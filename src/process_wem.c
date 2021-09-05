/*
Copyright 2021 BowToes (bow.toes@mailfence.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "process_files.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "riff.h"
#include "wwise.h"
#include "bitstream.h"

#define RIFF_BUFFER_APPLY_SUCCESS 0
#define RIFF_BUFFER_APPLY_FAILURE -1

#define RIFF_BUFFER_INCREMENT 4096

static const char *ginput_name;
static const char *goutput_name;
static codebook_libraryT *glibrary;

static void BRRCALL
i_print_wem(const wwise_wemT *const wem)
{
	if (!wem)
		return;
	BRRLOG_NOR("fmt data:");
	BRRLOG_NOR("fmt>     format_tag : 0x%04X", wem->fmt.format_tag);
	BRRLOG_NOR("fmt>     n_channels : %u", wem->fmt.n_channels);
	BRRLOG_NOR("fmt>samples_per_sec : %lu", wem->fmt.samples_per_sec);
	BRRLOG_NOR("fmt>  avg_byte_rate : %lu", wem->fmt.avg_byte_rate);
	BRRLOG_NOR("fmt>    block_align : %u", wem->fmt.block_align);
	BRRLOG_NOR("fmt>bits_per_sample : %u", wem->fmt.bits_per_sample);
	BRRLOG_NOR("fmt>     extra_size : %u", wem->fmt.extra_size);
	if (wem->fmt.extra_size) {
		BRRLOG_NOR("fmt>extra>    reserved : %u", wem->fmt.reserved);
		BRRLOG_NOR("fmt>extra>channel_mask : %lu", wem->fmt.channel_mask);
		if (wem->fmt.extra_size == 22) {
			BRRLOG_NOR("fmt>extra>        guid>data1 : %lu", wem->fmt.guid.data1);
			BRRLOG_NOR("fmt>extra>        guid>data2 : %u", wem->fmt.guid.data2);
			BRRLOG_NOR("fmt>extra>        guid>data3 : %u", wem->fmt.guid.data3);
			BRRLOG_NOR("fmt>extra>        guid>data4 : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
			    wem->fmt.guid.data4[0], wem->fmt.guid.data4[1], wem->fmt.guid.data4[2], wem->fmt.guid.data4[3],
			    wem->fmt.guid.data4[4], wem->fmt.guid.data4[5], wem->fmt.guid.data4[6], wem->fmt.guid.data4[7]);
		} else if (wem->fmt.extra_size > 22) {
			BRRLOG_NOR("fmt>extra == vorb");
		}
	}
	BRRLOG_NOR("vorb data:");
	BRRLOG_NOR("vorb>       sample_count : %lu", wem->vorb.sample_count);
	BRRLOG_NOR("vorb>         mod_signal : %lu 0x%08X", wem->vorb.mod_signal, wem->vorb.mod_signal);
	BRRLOG_NOR("vorb>setup_packet_offset : 0x%08X", wem->vorb.setup_packet_offset);
	BRRLOG_NOR("vorb> audio_start_offset : 0x%08X", wem->vorb.audio_start_offset);
	BRRLOG_NOR("vorb>                uid : 0x%08X", wem->vorb.uid);
	BRRLOG_NOR("vorb>        blocksize_0 : %u", wem->vorb.blocksize[0]);
	BRRLOG_NOR("vorb>        blocksize_1 : %u", wem->vorb.blocksize[1]);
}
static int BRRCALL
i_consume_next_chunk(FILE *const file, riffT *const rf, riff_chunk_infoT *const sc, riff_data_syncT *const ds)
{
	int err = 0;
	brrsz bytes_read = 0;
	char *buffer = NULL;
	while (RIFF_CHUNK_CONSUMED != (err = riff_consume_chunk(rf, sc, ds))) {
		if (err == RIFF_CONSUME_MORE) {
			continue;
		} else if (err == RIFF_CHUNK_UNRECOGNIZED) {
			//BRRLOG_WAR("Skipping unrecognized chunk '%s' (%zu)", FCC_INT_CODE(sc->chunkcc), sc->chunksize);
			continue;
		} else if (err != RIFF_CHUNK_INCOMPLETE) {
			if (err == RIFF_ERROR)
				return I_BUFFER_ERROR;
			else if (err == RIFF_NOT_RIFF)
				return I_NOT_RIFF;
			else if (err == RIFF_CORRUPTED)
				return I_CORRUPT;
			else
				return I_BAD_ERROR - err;
		} else if (feof(file)) {
			if (sc->chunksize)
				return I_FILE_TRUNCATED;
			break;
		} else if (!(buffer = riff_data_sync_buffer(ds, RIFF_BUFFER_INCREMENT))) {
			return I_BUFFER_ERROR;
		}
		bytes_read = fread(buffer, 1, RIFF_BUFFER_INCREMENT, file);
		if (ferror(file)) {
			return I_IO_ERROR;
		} else if (RIFF_BUFFER_APPLY_SUCCESS != riff_data_sync_apply(ds, bytes_read)) {
			return I_BUFFER_ERROR;
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_read_riff_chunks(FILE *const file, riffT *const rf)
{
	riff_chunk_infoT sync_chunk = {0};
	riff_data_syncT sync_data = {0};
	int err = I_SUCCESS;
	while (I_SUCCESS == (err = i_consume_next_chunk(file, rf, &sync_chunk, &sync_data)) && (sync_chunk.is_basic || sync_chunk.is_list)) {
		riff_chunk_info_clear(&sync_chunk);
	}
	if (err != I_SUCCESS) {
		riff_data_sync_clear(&sync_data);
		return err;
	}
	return err;
}
static void BRRCALL
i_clear_state(ogg_sync_state *const syncer,
    ogg_stream_state *restrict const istreamer, ogg_stream_state *restrict const ostreamer,
    oggpack_buffer *restrict const ipacker, oggpack_buffer *restrict const opacker,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	if (syncer)
		ogg_sync_clear(syncer);
	if (istreamer)
		ogg_stream_clear(istreamer);
	if (ipacker)
		oggpack_reset(ipacker);
	if (ostreamer)
		ogg_stream_clear(ostreamer);
	if (opacker)
		oggpack_writeclear(opacker);
	if (vi)
		vorbis_info_clear(vi);
	if (vc)
		vorbis_comment_clear(vc);
}
static int BRRCALL
i_init_state(const wwise_wemT *const wem, ogg_sync_state *const syncer,
    ogg_stream_state *const istreamer, ogg_stream_state *const ostreamer,
    oggpack_buffer *const ipacker, oggpack_buffer *const opacker,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int serialno = 0;
	serialno = wem->vorb.uid;
	if (STREAM_INIT_SUCCESS != ogg_stream_init(istreamer, 0)) {
		return I_INIT_ERROR;
	} else if (STREAM_INIT_SUCCESS != ogg_stream_init(ostreamer, serialno)) {
		ogg_stream_clear(istreamer);
		return I_INIT_ERROR;
	}
	ogg_sync_init(syncer);
	oggpack_readinit(ipacker, wem->data, wem->data_size);
	oggpack_writeinit(opacker);
	vorbis_info_init(vi);
	vorbis_comment_init(vc);
	return I_SUCCESS;
}
static void BRRCALL
i_init_header_packet(int type, oggpack_buffer *const opacker)
{
	oggpack_writeclear(opacker);
	oggpack_writeinit(opacker);

	oggpack_write(opacker, type, 8);                        /* Packet type */
	oggpack_write(opacker, 'v', 8);                         /* Vorbis identification string */
	oggpack_write(opacker, 'o', 8);                         /* Vorbis identification string */
	oggpack_write(opacker, 'r', 8);                         /* Vorbis identification string */
	oggpack_write(opacker, 'b', 8);                         /* Vorbis identification string */
	oggpack_write(opacker, 'i', 8);                         /* Vorbis identification string */
	oggpack_write(opacker, 's', 8);                         /* Vorbis identification string */
}
static int BRRCALL
i_finish_header_packet(int packetno, ogg_packet *const sync_packet,
    ogg_stream_state *const ostreamer, oggpack_buffer *const opacker,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	sync_packet->packet = oggpack_get_buffer(opacker);
	sync_packet->bytes = oggpack_bytes(opacker);
	sync_packet->b_o_s = packetno == 0;
	sync_packet->e_o_s = 0;
	sync_packet->granulepos = 0;
	sync_packet->packetno = packetno;
	if (VORBIS_SYNTHESIS_HEADERIN_SUCCESS != (err = vorbis_synthesis_headerin(vi, vc, sync_packet))) {
		switch (err) {
			case VORBIS_SYNTHESIS_HEADERIN_FAULT: return I_BUFFER_ERROR;
			case VORBIS_SYNTHESIS_HEADERIN_NOTVORBIS: return I_NOT_VORBIS;
			case VORBIS_SYNTHESIS_HEADERIN_BADHEADER: return I_CORRUPT;
			default: return I_BAD_ERROR;
		}
	} else if (STREAM_PACKETIN_SUCCESS != (err = ogg_stream_packetin(ostreamer, sync_packet))) {
		return I_BUFFER_ERROR;
	}
	return I_SUCCESS;
}
static int BRRCALL
i_init_id(const wwise_wemT *const wem, ogg_packet *const sync_packet,
    ogg_stream_state *const ostreamer, oggpack_buffer *const opacker,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	i_init_header_packet(1, opacker);
	oggpack_write(opacker, 0, 32);                          /* Vorbis version */
	oggpack_write(opacker, wem->fmt.n_channels, 8);         /* Audio channels */
	oggpack_write(opacker, wem->fmt.samples_per_sec, 32);   /* Audio sample rate */
	oggpack_write(opacker, 0, 32);                          /* Bitrate maximum */
	oggpack_write(opacker, 8 * wem->fmt.avg_byte_rate, 32); /* Bitrate nominal */
	oggpack_write(opacker, 0, 32);                          /* Bitrate minimum */
	oggpack_write(opacker, wem->vorb.blocksize[0], 4);      /* Blocksize 0 */
	oggpack_write(opacker, wem->vorb.blocksize[1], 4);      /* Blocksize 1 */
	oggpack_write(opacker, 1, 8);                           /* Framing flag */
	return i_finish_header_packet(0, sync_packet, ostreamer, opacker, vi, vc);
}
static int BRRCALL
i_init_comments(ogg_packet *const sync_packet,
    ogg_stream_state *const ostreamer, oggpack_buffer *const opacker,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	static const char vendor_string[] = "(N)ieR: (A)utomata (e)xtraction (P)rotocol";
	static const brru4 vendor_length = sizeof(vendor_string) - 1;
	static const char comment_format[] = "COMMENT=Converted from %s";
	static char comment[513] = {0};
	static brru4 comment_length = 0;
	i_init_header_packet(3, opacker);

	comment_length = snprintf(comment, 513, comment_format, ginput_name);
	oggpack_write(opacker, vendor_length, 32);              /* Vendor string length */
	for (brru4 i = 0; i < vendor_length; ++i)
		oggpack_write(opacker, vendor_string[i], 8);        /* Vendor string */
	oggpack_write(opacker, 1, 32);                          /* Number of comment strings */
	oggpack_write(opacker, comment_length, 32);             /* Comment length */
	for (brru4 i = 0; i < comment_length; ++i)
		oggpack_write(opacker, comment[i], 8);              /* Comment string */
	oggpack_write(opacker, 1, 8);                           /* Framing bit */

	return i_finish_header_packet(1, sync_packet, ostreamer, opacker, vi, vc);
}
static int BRRCALL
i_init_headers(const wwise_wemT *const wem,
    ogg_sync_state *const syncer, ogg_page *const sync_page, ogg_packet *const sync_packet,
    ogg_stream_state *restrict const istreamer, ogg_stream_state *restrict const ostreamer,
    oggpack_buffer *restrict const ipacker, oggpack_buffer *restrict const opacker,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int err;
	if (I_SUCCESS != (err = i_init_id(wem, sync_packet, ostreamer, opacker, vi, vc)))
		return err;
	if (I_SUCCESS != (err = i_init_comments(sync_packet, ostreamer, opacker, vi, vc)))
		return err;
	return I_SUCCESS;
}
static int BRRCALL
int_convert_wem(void)
{
	int err = 0;
	FILE *in, *out;
	riffT rf;
	wwise_wemT wem;
	ogg_page sync_page;
	ogg_packet sync_packet;
	ogg_sync_state syncer;
	ogg_stream_state istreamer, ostreamer;
	oggpack_buffer ipacker, opacker;
	vorbis_info vi;
	vorbis_comment vc;

	if (!(in = fopen(ginput_name, "rb"))) {
		BRRLOG_ERRN("Failed to open wem for conversion input '%s' : %s ", ginput_name, strerror(errno));
		return I_IO_ERROR;
	}

	riff_init(&rf);
	if (I_SUCCESS != (err = i_read_riff_chunks(in, &rf))) {
		BRRLOG_ERRN("Failed to consume RIFF chunk : %s ", i_strerr(err));
		fclose(in);
		riff_clear(&rf);
		return err;
	}
	fclose(in);
	if (WEM_SUCCESS != (err = wwise_wem_init(&wem, &rf))) {
		riff_clear(&rf);
		if (err == WEM_INCOMPLETE) {
			BRRLOG_ERRN("WEM missing 'fmt', 'data' or 'vorb' chunk(s)");
		} else if (err == WEM_DUPLICATE) {
			BRRLOG_ERRN("WEM has duplicate 'fmt', 'data', or 'vorb' chunk(s)");
		} else if (err == WEM_NOT_VORBIS) {
			BRRLOG_ERRN("WEM does not contain vorbis data");
			return I_NOT_VORBIS;
		}
		return I_INIT_ERROR;
	}
	if (I_SUCCESS != (err = i_init_state(&wem, &syncer, &istreamer, &ostreamer, &ipacker, &opacker, &vi, &vc))) {
		riff_clear(&rf);
		return err;
	} else if (I_SUCCESS != (err = i_init_headers(&wem,
	    &syncer, &sync_page, &sync_packet,
	    &istreamer, &ostreamer, &ipacker, &opacker, &vi, &vc))) {
		riff_clear(&rf);
		i_clear_state(&syncer, &istreamer, &ostreamer, &ipacker, &opacker, &vi, &vc);
		return err;
	}
	/* Then, decode/copy packets from input data to output oggstream */

	riff_clear(&rf);
	return err;
}

int BRRCALL
convert_wem(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_ogg, input_libraryT *const library)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	numbers->wems_to_convert++;
	if (dry_run) {
		BRRLOG_FORENP(DRY_COLOR, " Convert WEM (dry)");
	} else {
		brrsz outlen = 0, inlen = 0;
		BRRLOG_FORENP(WET_COLOR, " Converting WEM...");
		replace_ext(path, &inlen, output, &outlen, ".txt");
		if ((err = input_library_load(library))) {
			BRRLOG_ERRN(" Failed to load codebook library '%s' : %s", (char *)library->library_path.opaque, i_strerr(err));
		} else {
			ginput_name = path;
			goutput_name = output;
			glibrary = &library->library;
			err = int_convert_wem();
			if (!err) {
				if (inplace_ogg) {
					NeTODO("WEM CONVERT IN-PLACE");
					/* remove 'path' and rename 'output' to 'path' */
				}
			}
		}
	}
	if (!err) {
		numbers->wems_converted++;
		BRRLOG_MESSAGETP(gbrrlog_level_last, SUCCESS_FORMAT, " Success!");
	} else {
		/* remove 'output' */
		BRRLOG_MESSAGETP(gbrrlog_level_last, FAILURE_FORMAT, " Failure! (%d)", err);
	}
	return err;
}
