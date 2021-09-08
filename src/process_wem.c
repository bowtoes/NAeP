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
#include <brrtools/brrtil.h>

#include "riff.h"
#include "wwise.h"

static const input_optionsT *goptions = NULL;
static const char *ginput_name = NULL;
static char goutput_name[BRRPATH_MAX_PATH + 1] = {0};
static codebook_libraryT *glibrary = NULL;

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
	BRRLOG_NOR("vorb>         sample_count : %lu", wem->vorb.sample_count);
	BRRLOG_NOR("vorb>           mod_signal : %lu 0x%08X", wem->vorb.mod_signal, wem->vorb.mod_signal);
	BRRLOG_NOR("vorb>header_packets_offset : 0x%08X", wem->vorb.header_packets_offset);
	BRRLOG_NOR("vorb>   audio_start_offset : 0x%08X", wem->vorb.audio_start_offset);
	BRRLOG_NOR("vorb>                  uid : 0x%08X", wem->vorb.uid);
	BRRLOG_NOR("vorb>          blocksize_0 : %u", wem->vorb.blocksize[0]);
	BRRLOG_NOR("vorb>          blocksize_1 : %u", wem->vorb.blocksize[1]);
}

/* State init/input reading */
static int BRRCALL
i_init_state(ogg_stream_state *const streamer, const wwise_wemT *const wem,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int serialno = 0;
	serialno = wem->vorb.uid;
	if (STREAM_INIT_SUCCESS != ogg_stream_init(streamer, serialno)) {
		return I_INIT_ERROR;
	}
	vorbis_info_init(vi);
	vorbis_comment_init(vc);
	return I_SUCCESS;
}
static void BRRCALL
i_clear_state(ogg_stream_state *const streamer, vorbis_info *const vi, vorbis_comment *const vc)
{
	if (streamer)
		ogg_stream_clear(streamer);
	if (vi)
		vorbis_info_clear(vi);
	if (vc)
		vorbis_comment_clear(vc);
}
static int BRRCALL
i_consume_next_chunk(FILE *const file, riffT *const rf, riff_chunk_infoT *const sc, riff_data_syncT *const ds)
{
	#define RIFF_BUFFER_INCREMENT 4096
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
	#undef RIFF_BUFFER_INCREMENT
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
	riff_data_sync_clear(&sync_data);
	return err;
}

static long BRRCALL
i_packer_transfer(oggpack_buffer *const unpacker, int in_bits,
    oggpack_buffer *const packer, int out_bits)
{
	long r = oggpack_read(unpacker, in_bits);
	if (r == -1)
		return r;
	oggpack_write(packer, r, out_bits);
	return r;
}
static long BRRCALL
i_packer_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	long dwords = 0, left = 0, transferred = 0;
	if (unpacker->endbit) {
		int bits = 7 - unpacker->endbit;
		i_packer_transfer(unpacker, bits, packer, bits);
		transferred += bits;
	}
	dwords = (unpacker->storage - unpacker->endbyte) / 4;
	for (long i = 0; i < dwords; ++i) {
		i_packer_transfer(unpacker, 32, packer, 32);
		transferred += 32;
	}
	left = 8 * (unpacker->storage - unpacker->endbyte);
	if (left) {
		i_packer_transfer(unpacker, left, packer, left);
		transferred += 8;
	}
	return I_SUCCESS;
}
/* Header processing */
static int BRRCALL
i_build_packet(ogg_packet *const packet, oggpack_buffer *const packer,
	brru8 packetno, brru8 granule, int end_of_stream)
{
	packet->packet = oggpack_get_buffer(packer);
	packet->bytes = oggpack_bytes(packer);
	packet->b_o_s = packetno == 0;
	packet->e_o_s = end_of_stream != 0;
	packet->granulepos = granule;
	packet->packetno = packetno;
	return 0;
}
static int BRRCALL
i_insert_packet(ogg_stream_state *const streamer, ogg_packet *const packet,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	if (ogg_stream_packetin(streamer, packet)) {
		return I_BUFFER_ERROR;
	} else if (vi || vc) {
		int err = 0;
		if (!vi || !vc) {
			return I_BAD_ERROR;
		} else if ((err = vorbis_synthesis_headerin(vi, vc, packet))) {
			BRRLOG_ERRN("Failed to synthesize header %d : ", packet->packetno);
			if (err == OV_ENOTVORBIS)
				BRRLOG_ERR("NOT VORBIS");
			else if (err == OV_EBADHEADER)
				BRRLOG_ERR("BAD HEADER");
			else
				BRRLOG_ERR("INTERNAL ERROR");
			return I_CORRUPT;
		}
	}
	return I_SUCCESS;
}
/* COPY */
static int BRRCALL
i_copy_id_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	brrs4 packet_type = 0, vorbis[6] = {0}, audio_channels, blocksize_0, blocksize_1, frame_flag;
	brrs8 version = 0, sample_rate, bitrate_max, bitrate_nom, bitrate_min;

	packet_type = oggpack_read(unpacker, 8);        /* IN Packet type, should be 1 */
	for (int i = 0; i < 6; ++i)
		vorbis[i] = oggpack_read(unpacker, 8);      /* IN Vorbis str, should read 'vorbis' */
	version = oggpack_read(unpacker, 32);           /* IN Version, should be 0 */
	audio_channels = oggpack_read(unpacker, 8);     /* IN Audio channels */
	sample_rate = oggpack_read(unpacker, 32);       /* IN Sample rate */
	bitrate_max = oggpack_read(unpacker, 32);       /* IN Bitrate maximum */
	bitrate_nom = oggpack_read(unpacker, 32);       /* IN Bitrate nominal */
	bitrate_min = oggpack_read(unpacker, 32);       /* IN Bitrate minimum */
	blocksize_0 = oggpack_read(unpacker, 4);        /* IN Blocksize 0 */
	blocksize_1 = oggpack_read(unpacker, 4);        /* IN Blocksize 1 */
	frame_flag = oggpack_read(unpacker, 1);         /* IN Frame flag, should be 1 */

	oggpack_write(packer, 1, 8);                    /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                     /* OUT Vorbis str */
		oggpack_write(packer, VORBIS_STR[i], 8);
	oggpack_write(packer, 0, 32);                   /* OUT Version */
	oggpack_write(packer, audio_channels, 8);       /* OUT Audio channels */
	oggpack_write(packer, sample_rate, 32);         /* OUT Sample rate */
	oggpack_write(packer, bitrate_max, 32);         /* OUT Bitrate maximum */
	oggpack_write(packer, bitrate_nom, 32);         /* OUT Bitrate nominal */
	oggpack_write(packer, bitrate_min, 32);         /* OUT Bitrate minimum */
	oggpack_write(packer, blocksize_0, 4);          /* OUT Blocksize 0 */
	oggpack_write(packer, blocksize_1, 4);          /* OUT Blocksize 1 */
	oggpack_write(packer, 1, 1);                    /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_copy_comment_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	brrs4 packet_type = 0, vorbis[6] = {0}, frame_flag = 0;
	brrs8 vendor_length = 0, comments_count = 0;

	packet_type = oggpack_read(unpacker, 8);        /* IN Packet type, should be 3 */
	oggpack_write(packer, 3, 8);                    /* OUT Packet type */

	for (int i = 0; i < 6; ++i)                     /* IN Vorbis str, should read 'vorbis' */
		vorbis[i] = oggpack_read(unpacker, 8);
	for (int i = 0; i < 6; ++i)                     /* OUT Vorbis str */
		oggpack_write(packer, VORBIS_STR[i], 8);

	vendor_length = i_packer_transfer(unpacker, 32, packer, 32);      /* IN/OUT Vendor length */
	for (brrs8 i = 0; i < vendor_length; ++i) {                       /* IN/OUT Vendor string */
		char vendor_str = i_packer_transfer(unpacker, 32, packer, 8);
	}
	comments_count = i_packer_transfer(unpacker, 32, packer, 32);     /* IN/OUT Comment list length */
	for (brrs8 i = 0; i < comments_count; ++i) {
		brrs8 comment_length = i_packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Comment length */
		for (brrs8 j = 0; j < comment_length; ++j) {                        /* IN/OUT Comment string */
			char comment_str = i_packer_transfer(unpacker, 8, packer, 8);
		}
	}
	frame_flag = oggpack_read(unpacker, 1);         /* IN Frame flag, should be 1 */
	oggpack_write(packer, 1, 1);                    /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_copy_next_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	long dimensions, entries;
	int ordered, lookup;
	if ('B' != i_packer_transfer(unpacker, 8, packer, 8) ||   /* IN/OUT Codebook sync */
	    'C' != i_packer_transfer(unpacker, 8, packer, 8) ||   /* IN/OUT Codebook sync */
	    'V' != i_packer_transfer(unpacker, 8, packer, 8)) {   /* IN/OUT Codebook sync */
		return I_CORRUPT;
	}
	dimensions = i_packer_transfer(unpacker, 16, packer, 16); /* IN/OUT Codebook dimensions */
	entries = i_packer_transfer(unpacker, 24, packer, 24);    /* IN/OUT Codebook entries */
	ordered = i_packer_transfer(unpacker, 1, packer, 1);      /* IN/OUT Ordered flag */
	if (ordered) {
		int current_length = 1 + i_packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = ilog(entries - current_entry);
			long number = i_packer_transfer(unpacker, number_bits, packer, number_bits); /* IN/OUT Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries)
			return I_CORRUPT;
	} else {
		int sparse = i_packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Sparse flag */
		for (long i = 0; i < entries; ++i) {
			if (!sparse) {
				int length = 1 + i_packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Codeword length */
			} else {
				int used = i_packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Used flag */
				if (used) {
					int length = 1 + i_packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Codeword length */
				}
			}
		}
	}
	lookup = i_packer_transfer(unpacker, 4, packer, 4); /* IN/OUT Lookup type */
	if (lookup) {
		long minval_packed = 0, delval_packed = 0;
		int value_bits = 0, sequence_flag = 0;
		long lookup_values = 0;
		if (lookup > 2)
			return I_CORRUPT;
		minval_packed = i_packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Minimum value */
		delval_packed = i_packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Delta value */
		value_bits = 1 + i_packer_transfer(unpacker, 4, packer, 4);  /* IN/OUT Value bits */
		sequence_flag = i_packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sequence flag */
		if (lookup == 1)
			lookup_values = lookup1_values(entries, dimensions);
		else
			lookup_values = entries * dimensions;

		for (long i = 0; i < lookup_values; ++i) { /* IN/OUT Codebook multiplicands */
			long multiplicand = i_packer_transfer(unpacker, value_bits, packer, value_bits);
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_copy_setup_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	int packet_type = 0, vorbis[6] = {0}, codebook_count = 0;

	packet_type = oggpack_read(unpacker, 8);        /* IN Packet type, should be 5 */
	oggpack_write(packer, 5, 8);                    /* OUT Packet type */

	for (int i = 0; i < 6; ++i)                     /* IN Vorbis str, should read 'vorbis' */
		vorbis[i] = oggpack_read(unpacker, 8);
	for (int i = 0; i < 6; ++i)                     /* OUT Vorbis str */
		oggpack_write(packer, VORBIS_STR[i], 8);

	codebook_count = 1 + i_packer_transfer(unpacker, 8, packer, 8); /* IN/OUT Codebooks counts */
	if (!glibrary || 1) { /* Inline codebooks, copy verbatim */
		// For now, copy verbatim
		for (int err = 0, i = 0; i < codebook_count; ++i) {
			if ((err = i_copy_next_codebook(unpacker, packer)))
				return err;
		}
	} else { /* Optionally external codebooks, copy from those instead */
		return I_BAD_ERROR;
	}
	/* Now copy the rest of it (naive shallow copy) */
	i_packer_transfer_remaining(unpacker, packer);
	return I_SUCCESS;
}
static int BRRCALL
i_copy_headers(ogg_stream_state *const streamer, wwise_wemT *const wem,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	unsigned char *packets_start = wem->data + wem->vorb.header_packets_offset;
	brru4 packets_size = wem->vorb.audio_start_offset - wem->vorb.header_packets_offset;
	wwise_packetT packeteer = {0};

	/* Should check for WWISE_INCOMPLETE? */
	for (int current_header = 0; current_header < 3; ++current_header) {
		ogg_packet packet;
		oggpack_buffer unpacker, packer;
		if (WWISE_SUCCESS != wwise_packet_init(&packeteer, wem, packets_start, packets_size))
			return I_INSUFFICIENT_DATA;
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		oggpack_writeinit(&packer);
		if (current_header == 0) {
			err = i_copy_id_header(&unpacker, &packer);
		} else if (current_header == 1) {
			err = i_copy_comment_header(&unpacker, &packer);
		} else {
			err = i_copy_setup_header(&unpacker, &packer);
		}
		if (err) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_build_packet(&packet, &packer, current_header, 0, 0))) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_insert_packet(streamer, &packet, vi, vc))) {
			oggpack_writeclear(&packer);
			return err;
		}
		oggpack_writeclear(&packer);
		packets_start += packeteer.header_length + packeteer.payload_size;
		packets_size -= packeteer.header_length + packeteer.payload_size;
	}
	return I_SUCCESS;
}
/* BUILD */
static int BRRCALL
i_build_id_header(oggpack_buffer *const packer, const wwise_wemT *const wem)
{
	oggpack_write(packer, 1, 8);                           /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                            /* OUT Vorbis string */
		oggpack_write(packer, VORBIS_STR[i], 8);
	oggpack_write(packer, 0, 32);                          /* OUT Vorbis version */
	oggpack_write(packer, wem->fmt.n_channels, 8);         /* OUT Audio channels */
	oggpack_write(packer, wem->fmt.samples_per_sec, 32);   /* OUT Sample rate */
	oggpack_write(packer, 0, 32);                          /* OUT Bitrate maximum */
	oggpack_write(packer, wem->fmt.avg_byte_rate * 8, 32); /* OUT Bitrate nominal */
	oggpack_write(packer, 0, 32);                          /* OUT Bitrate minimum */
	oggpack_write(packer, wem->vorb.blocksize[0], 4);      /* OUT Blocksize 0 */
	oggpack_write(packer, wem->vorb.blocksize[1], 4);      /* OUT Blocksize 1 */
	oggpack_write(packer, 1, 1);                           /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_build_comments_header(oggpack_buffer *const packer)
{
	static const char vendor_string[] = "NieR:Automata extraction Preceptv"NeVERSION;
	static const char comment_format[] = "COMMENT=Converted from %s";
	char comment[513] = {0};
	brru4 comment_length = 0;
	comment_length = snprintf(comment, 513, comment_format, ginput_name);

	oggpack_write(packer, 3, 8);                            /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                             /* OUT Vorbis string */
		oggpack_write(packer, VORBIS_STR[i], 8);

	oggpack_write(packer, (sizeof(vendor_string) - 1), 32); /* OUT Vendor string length */
	for (long i = 0; i < sizeof(vendor_string) - 1; ++i)    /* OUT Vendor string */
		oggpack_write(packer, vendor_string[i], 8);
	oggpack_write(packer, 1, 32);                           /* OUT Comment list length */
	for (long i = 0; i < 1; ++i) {
		oggpack_write(packer, comment_length, 32);          /* OUT Comment length */
		for (long i = 0; i < comment_length; ++i)           /* OUT Comment string */
			oggpack_write(packer, comment[i], 8);
	}

	oggpack_write(packer, 1, 1);                            /* OUT Frame flag */
	return I_SUCCESS;
}
static long codebook_bytes;
static int BRRCALL
i_build_codebook(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	int dimensions, entries, ordered, lookup;

	oggpack_write(packer, 'B', 8); /* OUT Sync */
	oggpack_write(packer, 'C', 8); /* OUT Sync */
	oggpack_write(packer, 'V', 8); /* OUT Sync */

	dimensions = i_packer_transfer(unpacker,  4, packer, 16); /* IN/OUT Dimensions */
	entries = i_packer_transfer(unpacker, 14, packer, 24);    /* IN/OUT Entries */
	ordered = i_packer_transfer(unpacker, 1, packer, 1);      /* IN/OUT Ordered flag */
	if (ordered) { /* Ordered codeword decode identical to spec */
		int current_length = 1 + i_packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Start length */
		long current_entry = 0;
#if defined(NeWEMDEBUG)
		BRRLOG_DEBUGNP("Ordered  ");
#endif
		while (current_entry < entries) {
			int number_bits = ilog(entries - current_entry);
			long number = i_packer_transfer(unpacker, number_bits, packer, number_bits); /* IN/OUT Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries)
			return I_CORRUPT;
	} else {
		int codeword_length_bits, sparse;
#if defined(NeWEMDEBUG)
		BRRLOG_DEBUGNP("Unordered");
#endif
		codeword_length_bits = oggpack_read(unpacker, 3);     /* IN Codeword length bits */
		if (codeword_length_bits < 0 || codeword_length_bits > 5)
			return I_CORRUPT;
		sparse = i_packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sparse flag */
		if (!sparse) { /* IN/OUT Nonsparse codeword lengths */
#if defined(NeWEMDEBUG)
			BRRLOG_DEBUGNP(", Nonsparse");
#endif
			for (int i = 0; i < entries; ++i) {
				int length = i_packer_transfer(unpacker, codeword_length_bits, packer, 5);
			}
		} else { /* IN/OUT Sparse codeword lengths */
#if defined(NeWEMDEBUG)
			BRRLOG_DEBUGNP(", Sparse   ");
#endif
			for (int i = 0; i < entries; ++i) {
				int used = i_packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Used flag */
				if (used) {
					int length = i_packer_transfer(unpacker, codeword_length_bits, packer, 5); /* IN/OUT Codeword length */
				}
			}
		}
	}

	lookup = i_packer_transfer(unpacker, 1, packer, 4); /* IN/OUT Lookup type */
	if (lookup == 1) { /* Lookup 1 decode identical to spec */
		long minval_packed = i_packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Minimum value */
		long delval_packed = i_packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Delta value */
		int value_bits = 1 + i_packer_transfer(unpacker, 4, packer, 4);  /* IN/OUT Value bits */
		int sequence_flag = i_packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sequence flag */

		long lookup_values = lookup1_values(entries, dimensions);
#if defined(NeWEMDEBUG)
		BRRLOG_DEBUGNP(", Lookup 1 values %3lld", lookup_values);
#endif

		for (long i = 0; i < lookup_values; ++i) { /* IN/OUT Codebook multiplicands */
			long multiplicand = i_packer_transfer(unpacker, value_bits, packer, value_bits);
		}
	} else if (lookup == 0) {
#if defined(NeWEMDEBUG)
		BRRLOG_DEBUGNP(", Lookup 0 no values ");
#endif
	} else {
		BRRLOG_ERR("LOOKUP FAILED");
	}
#if defined(NeWEMDEBUG)
	if (codebook_bytes)
		BRRLOG_DEBUGP(", %sRead %4lld == %3lld of %lld",
		    (oggpack_bits(unpacker)/8)+1!=codebook_bytes?"!! ":"   ",
		    oggpack_bits(unpacker), oggpack_bytes(unpacker), codebook_bytes);
#endif
	return I_SUCCESS;
}
static int BRRCALL
i_build_floors(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* Floor 1 decode mostly identical to spec, except floor type is absent from
	 * each floor (because there is only a single floor type) */
	int floor_count = 1 + i_packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Floor count */
	for (int i = 0; i < floor_count; ++i) {
		int partitions, partition_classes[31], max_class = -1;
		int class_dims[16], class_subs[16], class_books[16], sub_books[16][16];
		int multiplier, rangebits;
		oggpack_write(packer, 1, 16);                            /* OUT Floor type, always 1 */
		partitions = i_packer_transfer(unpacker, 5, packer, 5);  /* IN/OUT Floor partitions */
		for (int j = 0; j < partitions; ++j) {                   /* IN/OUT Partition classes */
			int class = partition_classes[j] = i_packer_transfer(unpacker, 4, packer, 4);
			if (class > max_class)
				max_class = class;
		}
		for (int j = 0; j <= max_class; ++j) {
			int dim = class_dims[j] = 1 + i_packer_transfer(unpacker, 3, packer, 3); /* IN/OUT Class dimensions */
			int sub = class_subs[j] = i_packer_transfer(unpacker, 2, packer, 2);     /* IN/OUT Class subclasses */
			int limit_break = 1 << sub;
			if (sub) {                                                               /* IN/OUT Class books */
				int master = class_books[j] = i_packer_transfer(unpacker, 8, packer, 8);
			}
			for (int k = 0; k < limit_break; ++k) {                                  /* IN/OUT Subclass books */
				sub_books[j][k] = -1 + i_packer_transfer(unpacker, 8, packer, 8);
			}
		}
		multiplier = 1 + i_packer_transfer(unpacker, 2, packer, 2); /* IN/OUT Floor multiplier */
		rangebits = i_packer_transfer(unpacker, 4, packer, 4);      /* IN/OUT Floor rangebits */
		for (int j = 0; j < partitions; ++j) {
			int dims = class_dims[partition_classes[j]];
			for (int k = 0; k < dims; ++k) {           /* IN/OUT Floor X list */
				long X = i_packer_transfer(unpacker, rangebits, packer, rangebits);
			}
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_residues(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	/* As far as I can tell, residue decode is identical to spec */
	int residue_count = 1 + i_packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Residue count */
	for (int i = 0; i < residue_count; ++i) {
		long start, end, partition_size;
		int classes, classbook;
		int type;
		int cascades[64];
		int acc = 0; /* ??????? */
		type = i_packer_transfer(unpacker, 2, packer, 16);  /* IN/OUT Residue type */
		if (type > 2)
			return I_CORRUPT;

		start = i_packer_transfer(unpacker, 24, packer, 24);              /* IN/OUT Residue begin */
		end = i_packer_transfer(unpacker, 24, packer, 24);                /* IN/OUT Residue end */
		partition_size = 1 + i_packer_transfer(unpacker, 24, packer, 24); /* IN/OUT Partition size */
		classes = 1 + i_packer_transfer(unpacker, 6, packer, 6);          /* IN/OUT Residue classes */
		classbook = i_packer_transfer(unpacker, 8, packer, 8);            /* IN/OUT Residue classbook */

		for (int j = 0; j < classes; ++j) {                               /* IN/OUT Residue cascades */
			int bitflag;
			cascades[j] = i_packer_transfer(unpacker, 3, packer, 3);          /* IN/OUT Cascade low-bits */
			bitflag = i_packer_transfer(unpacker, 1, packer, 1);              /* IN/OUT Cascade bitflag */
			if (bitflag)
				cascades[j] += 8 * i_packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Cascade high-bits */

			acc += icount(cascades[j]);
		}
		for (int j = 0; j < acc; ++j) {/* IN/OUT Residue books */
			int residue_book_index_jb = i_packer_transfer(unpacker, 8, packer, 8);
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_mappings(oggpack_buffer *const unpacker, oggpack_buffer *const packer, int n_channels)
{
	int mapping_count = 1 + i_packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Mapping count */
	for (int i = 0; i < mapping_count; ++i) {
		int n_channel_bits = ilog(n_channels - 1);
		long mapping_type = 0; /* Mapping type always 0 */
		int submaps_flag, submaps = 1;
		int square_mapping, coupling_steps = 0;
		int reserved;
		oggpack_write(packer, mapping_type, 16); /* OUT Mapping type */
		submaps_flag = i_packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Submaps flag */

		if (submaps_flag)
			submaps = 1 + i_packer_transfer(unpacker, 4, packer, 4); /* IN/OUT Submaps */

		square_mapping = i_packer_transfer(unpacker, 1, packer, 1);  /* IN/OUT Square mapping flag */
		if (square_mapping) {
			coupling_steps = 1 + i_packer_transfer(unpacker, 8, packer, 8); /* IN/OUT Coupling steps */
			for (int j = 0; j < coupling_steps; ++j) { /* IN/OUT Mapping vectors */
				long mapping_magnitude = i_packer_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
				long mapping_angle = i_packer_transfer(unpacker, n_channel_bits, packer, n_channel_bits);
			}
		}

		reserved = oggpack_read(unpacker, 2); /* IN Reserved */
		oggpack_write(packer, 0, 2);          /* OUT Reserved */

		if (submaps > 1) {                    /* IN/OUT Mapping channel multiplexes */
			for (int j = 0; j < n_channels; ++j) {
				int mapping_mux = i_packer_transfer(unpacker, 4, packer, 4);
			}
		}
		for (int i = 0; i < submaps; ++i) { /* IN/OUT Submap configurations */
			int discarded = i_packer_transfer(unpacker, 8, packer, 8);
			int floor = i_packer_transfer(unpacker, 8, packer, 8);
			int residue = i_packer_transfer(unpacker, 8, packer, 8);
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_modes(oggpack_buffer *const unpacker, oggpack_buffer *const packer,
    brru1 *const mode_blockflags, int *const mode_count)
{
	int md_count = 1 + i_packer_transfer(unpacker, 6, packer, 6); /* IN/OUT Mode count */
	*mode_count = md_count;
	for (int i = 0; i < md_count; ++i) {
		int blockflag, mapping;
		long window = 0, transform = 0;
		blockflag = i_packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Blockflag */
		oggpack_write(packer, window, 16);                     /* OUT Window type */
		oggpack_write(packer, transform, 16);                  /* OUT Transform type */
		mapping = i_packer_transfer(unpacker, 8, packer, 8);   /* IN/OUT Mode mapping */

		mode_blockflags[i] = blockflag;
	}
	return I_SUCCESS;
}
static int BRRCALL
i_build_setup_header(oggpack_buffer *const unpacker, oggpack_buffer *const packer,
    wwise_wemT *const wem, int stripped)
{
	int codebook_count, err = 0;
	oggpack_write(packer, 5, 8);                            /* OUT Packet type */
	for (int i = 0; i < 6; ++i)                             /* OUT Vorbis string */
		oggpack_write(packer, VORBIS_STR[i], 8);

	codebook_count = 1 + i_packer_transfer(unpacker, 8, packer, 8); /* IN/OUT Codebook count */
	if (!glibrary) { /* Internal codebooks */
		if (!stripped) { /* Full codebooks, can be copied directly */
			for (int err = 0, i = 0; i < codebook_count; ++i) {
#if defined(NeWEMDEBUG)
				BRRLOG_DEBUG("Copying internal codebook %d", i);
#endif
				if ((err = i_copy_next_codebook(unpacker, packer))) {
					BRRLOG_ERR("Failed to copy codebook %d", i);
					return err;
				}
			}
		} else {
			for (int err = 0, i = 0; i < codebook_count; ++i) {
#if defined(NeWEMDEBUG)
				BRRLOG_DEBUG("Rebuilding internal codebook %d", i);
#endif
				if ((err = i_build_codebook(unpacker, packer))) {
					BRRLOG_ERR("Failed to build codebook %d", i);
					return err;
				}
			}
		}
	} else { /* External codebooks */
		for (int i = 0, err = 0; i < codebook_count; ++i) {
			oggpack_buffer cb_unpacker;
			int cbidx = 1 + oggpack_read(unpacker, 10);      /* IN Codebook index */
			/* I don't know why it's off by 1; ww2ogg just sorta rolls with it
			 * without too much checking (specifically in get_codebook_size) and
			 * I can't figure out why it works there */
			if (cbidx > glibrary->codebook_count) {
				/* This bit ripped from ww2ogg, no idea what it means */
				if (cbidx == 0x342) {
					cbidx = oggpack_read(unpacker, 14);      /* IN Codebook id */
					if (cbidx == 0x1590) {
						/* ??? */
					}
				}
				BRRLOG_ERR("Codebook index too large %d", cbidx);
				return I_CORRUPT;
			}
#if defined(NeWEMDEBUG)
			BRRLOG_DEBUGN("Building external codebook %3d: ", cbidx);
#endif
			codebook_bytes = glibrary->codebooks[cbidx].size;
			oggpack_readinit(&cb_unpacker, glibrary->codebooks[cbidx].data, glibrary->codebooks[cbidx].size);
			if ((err = i_build_codebook(&cb_unpacker, packer))) { /* Copy from external */
				BRRLOG_ERR("Failed to copy external codebook %d", cbidx);
				return err;
			}
		}
	}

	oggpack_write(packer, 0, 6);  /* OUT Time count - 1 */
	oggpack_write(packer, 0, 16); /* OUT Vorbis time-domain stuff */

	if (!stripped) { /* Rest of the header in-spec, copy verbatim */
		if ((err = i_packer_transfer_remaining(unpacker, packer))) {
			BRRLOG_ERR("Failed to copy setup packet");
			return err;
		}
	} else { /* Need to rebuild the information */
		if ((err = i_build_floors(unpacker, packer))) {
			BRRLOG_ERR("Failed building floors");
			return err;
		} else if ((err = i_build_residues(unpacker, packer))) {
			BRRLOG_ERR("Failed building residues");
			return err;
		} else if ((err = i_build_mappings(unpacker, packer, wem->fmt.n_channels))) {
			BRRLOG_ERR("Failed building mappings");
			return err;
		} else if ((err = i_build_modes(unpacker, packer, wem->mode_blockflags, &wem->mode_count))) {
			BRRLOG_ERR("Failed building modes");
			return err;
		}
	}

	oggpack_write(packer, 1, 1);                           /* OUT Frame flag */
	return I_SUCCESS;
}
static int BRRCALL
i_build_headers(ogg_stream_state *const streamer, wwise_wemT *const wem,
    vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	for (int current_header = 0; current_header < 3; ++current_header) {
		ogg_packet packet;
		oggpack_buffer packer;
		oggpack_writeinit(&packer);
		if (current_header == 0) {
			err = i_build_id_header(&packer, wem);
		} else if (current_header == 1) {
			err = i_build_comments_header(&packer);
		} else {
			oggpack_buffer unpacker;
			unsigned char *packets_start = wem->data + wem->vorb.header_packets_offset;
			brru4 packets_size = wem->vorb.audio_start_offset - wem->vorb.header_packets_offset;
			wwise_packetT packeteer = {0};
#if defined (NeWEMDEBUG)
			BRRLOG_DEBUG("Building setup header");
#endif
			if (WWISE_SUCCESS != (err = wwise_packet_init(&packeteer, wem, packets_start, packets_size))) {
				BRRLOG_ERRN("Failed to init setup header packet (%d)", err);
				err = I_INSUFFICIENT_DATA;
			} else {
				oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
				err = i_build_setup_header(&unpacker, &packer, wem, goptions->stripped_headers);
			}
		}
		if (err) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_build_packet(&packet, &packer, current_header, 0, 0))) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_insert_packet(streamer, &packet, vi, vc))) {
			oggpack_writeclear(&packer);
			return I_CORRUPT;
		}
		oggpack_writeclear(&packer);
	}
	return I_SUCCESS;
}
/* PROCESS */
static int BRRCALL
i_process_headers(ogg_stream_state *const streamer, wwise_wemT *const wem,
	vorbis_info *const vi, vorbis_comment *const vc)
{
	if (wem->all_headers_present) {
		return i_copy_headers(streamer, wem, vi, vc);
	} else {
		return i_build_headers(streamer, wem, vi, vc);
	}
}
static int BRRCALL
i_process_audio(ogg_stream_state *const streamer, wwise_wemT *const wem,
	vorbis_info *const vi, vorbis_comment *const vc)
{
	int err = 0;
	oggpack_buffer blockflag_unpacker;
	brru4 packets_start = wem->vorb.audio_start_offset,
	      packets_size = wem->data_size - wem->vorb.audio_start_offset;
	wwise_packetT packeteer = {0};
	int prev_blockflag = 0,  mode_count_bits = ilog(wem->mode_count - 1);
	brru8 packetno = 0, last_block = 0, total_block = 0;
	for (; packets_start < wem->data_size; ++packetno) {
		int eos = 0;
		ogg_packet packet;
		oggpack_buffer unpacker, packer;
		brru8 packet_granule = 1;
		if (WWISE_SUCCESS != wwise_packet_init(&packeteer, wem, wem->data + packets_start, packets_size)) {
			return I_INSUFFICIENT_DATA;
		}
		oggpack_readinit(&unpacker, packeteer.payload, packeteer.payload_size);
		oggpack_writeinit(&packer);

		if (wem->mod_packets) {
			int packet_type = 0;
			int mode_number, remainder;
			oggpack_write(&packer, packet_type, 1); /* OUT Packet type */
			mode_number = i_packer_transfer(&unpacker, mode_count_bits, &packer, mode_count_bits); /* IN/OUT Mode number */
			remainder = oggpack_read(&unpacker, 8 - mode_count_bits); /* IN Remainder bits */

			if (wem->mode_blockflags[mode_number]) {
				/* Long window */
				wwise_packetT next_packeteer;
				brru4 next_start = packets_start + packeteer.header_length + packeteer.payload_size,
				      next_size = packets_size + packeteer.header_length + packeteer.payload_size;
				int next_blockflag = 0;
				if (WWISE_SUCCESS != wwise_packet_init(&next_packeteer, wem, wem->data + next_start, next_size)) {
					eos = 1;
				} else if (next_packeteer.payload_size) {
					int next_number;
					oggpack_buffer next_unpacker;
					oggpack_readinit(&next_unpacker, next_packeteer.payload, next_packeteer.payload_size);
					next_number = oggpack_read(&next_unpacker, mode_count_bits); /* IN Next number */
					next_blockflag = wem->mode_blockflags[next_number];
				}
				oggpack_write(&packer, prev_blockflag, 1); /* OUT Previous window type */
				oggpack_write(&packer, next_blockflag, 1); /* OUT Next window type */
			}

			oggpack_write(&packer, remainder, 8 - mode_count_bits); /* OUT Remainder of read-in first byte */
			prev_blockflag = wem->mode_blockflags[mode_number];
		} else {
			int transferred = i_packer_transfer(&unpacker, 8, &packer, 8); /* Unmodified first byte */
		}
		i_packer_transfer_remaining(&unpacker, &packer);
		if (err) {
			oggpack_writeclear(&packer);
			return err;
		} else if ((err = i_build_packet(&packet, &packer, packetno + 3, 0, eos))) {
			oggpack_writeclear(&packer);
			return err;
		} else {
			/* This granule calculation from revorb, not sure its source though
			 * Probably somewhere in vorbis docs, haven't found it */
			long current_block = vorbis_packet_blocksize(vi, &packet);
			if (last_block)
				total_block += (last_block + current_block) / 4;
			last_block = current_block;
			packet.granulepos = total_block;
			if ((err = i_insert_packet(streamer, &packet, NULL, NULL))) {
				oggpack_writeclear(&packer);
				return err;
			}
		}
		oggpack_writeclear(&packer);
		packets_start += packeteer.header_length + packeteer.payload_size;
		packets_size -= packeteer.header_length + packeteer.payload_size;
	}
#if defined(NeWEMDEBUG)
	BRRLOG_DEBUG("Packetno : %lld", 3 + packetno);
#endif
	return I_SUCCESS;
}

/* Output writing */
static int BRRCALL
i_write_out(ogg_stream_state *const streamer)
{
	FILE *out = NULL;
	ogg_page pager;
	if (!(out = fopen(goutput_name, "wb"))) {
		BRRLOG_ERRN("Failed to open wem conversion output '%s' : %s", goutput_name, strerror(errno));
		return I_IO_ERROR;
	}
	while (ogg_stream_pageout(streamer, &pager) || ogg_stream_flush(streamer, &pager)) {
		if (pager.header_len != fwrite(pager.header, 1, pager.header_len, out)) {
			fclose(out);
			BRRLOG_ERRN("Failed to write ogg page header to output '%s' : %s", goutput_name, strerror(errno));
			return I_IO_ERROR;
		} else if (pager.body_len != fwrite(pager.body, 1, pager.body_len, out)) {
			fclose(out);
			BRRLOG_ERRN("Failed to write ogg page body to output '%s' : %s", goutput_name, strerror(errno));
			return I_IO_ERROR;
		}
	}
	fclose(out);
	return I_SUCCESS;
}
static int BRRCALL
int_convert_wem(void)
{
	int err = 0;
	FILE *in;
	riffT rf;
	wwise_wemT wem;
	ogg_stream_state streamer;
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
	if (WWISE_SUCCESS != (err = wwise_wem_init(&wem, &rf))) {
		riff_clear(&rf);
		if (err == WWISE_INCOMPLETE) {
			BRRLOG_ERRN("WEM missing");
			if (!wem.fmt_initialized)
				BRRLOG_ERRNP(" 'fmt'");
			if (!wem.vorb_initialized) {
				if (!wem.fmt_initialized)
					BRRLOG_ERRNP(",");
				BRRLOG_ERRNP(" 'vorb'");
			}
			if (!wem.data_initialized) {
				if (!wem.fmt_initialized || !wem.vorb_initialized)
					BRRLOG_ERRNP(",");
				BRRLOG_ERRNP(" 'data'");
			}
			BRRLOG_ERRNP(" chunks");
		} else if (err == WWISE_DUPLICATE) {
			BRRLOG_ERRN("WEM has duplicate 'fmt', 'data', or 'vorb' chunk(s)");
		} else if (err == WWISE_CORRUPT) {
			BRRLOG_ERRN("WEM is corrupted or does not contain vorbis data");
			return I_CORRUPT;
		}
		return I_INIT_ERROR;
	}
	if (I_SUCCESS != (err = i_init_state(&streamer, &wem, &vi, &vc))) {
		riff_clear(&rf);
		return err;
	} else if (I_SUCCESS != (err = i_process_headers(&streamer, &wem, &vi, &vc))) {
		i_clear_state(&streamer, &vi, &vc);
		riff_clear(&rf);
		return err;
	} else if (I_SUCCESS != (err = i_process_audio(&streamer, &wem, &vi, &vc))) {
		i_clear_state(&streamer, &vi, &vc);
		riff_clear(&rf);
		return err;
	}

	err = i_write_out(&streamer);

	i_clear_state(&streamer, &vi, &vc);
	riff_clear(&rf);
	return err;
}

int BRRCALL
convert_wem(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library)
{
	int err = 0;
	numbers->wems_to_convert++;
	if (options->dry_run) {
		BRRLOG_FORENP(NeLOG_COLOR_DRY, "Convert WEM (dry) ");
	} else {
		BRRLOG_FORENP(NeLOG_COLOR_WET, "Converting WEM... ");
		goptions = options;
		ginput_name = input;
		if (options->inplace_ogg) {
			snprintf(goutput_name, sizeof(goutput_name), "%s", input);
		} else {
			replace_ext(ginput_name, input_length, goutput_name, NULL, ".ogg");
		}
		if (!library) {
			glibrary = NULL;
		} else if ((err = input_library_load(library))) {
			BRRLOG_ERRN("Failed to load codebook library '%s' : %s", (char *)library->library_path.opaque, i_strerr(err));
		} else {
			glibrary = &library->library;
			err = int_convert_wem();
		}
	}
	if (!err) {
		numbers->wems_converted++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, NeLOG_FORMAT_SUCCESS, "Success!");
	} else {
		numbers->wems_failed++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, NeLOG_FORMAT_FAILURE, " Failure! (%d)", err);
	}
	return err;
}
