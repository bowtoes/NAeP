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
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

/* Ogg/Vorbis function return codes */
#define SYNC_PAGEOUT_SUCCESS 1
#define SYNC_PAGEOUT_INCOMPLETE 0
#define SYNC_PAGEOUT_DESYNC -1
#define SYNC_WROTE_SUCCESS 0
#define SYNC_WROTE_FAILURE -1
#define STREAM_INIT_SUCCESS 0
#define STREAM_PAGEIN_SUCCESS 0
#define STREAM_PACKETIN_SUCCESS 0
#define STREAM_PACKETOUT_SUCCESS 1
#define STREAM_PACKETOUT_INCOMPLETE 0
#define STREAM_PACKETOUT_DESYNC -1
#define VORBIS_SYNTHESIS_HEADERIN_SUCCESS 0
#define VORBIS_SYNTHESIS_HEADERIN_FAULT OV_EFAULT
#define VORBIS_SYNTHESIS_HEADERIN_NOTVORBIS OV_ENOTVORBIS
#define VORBIS_SYNTHESIS_HEADERIN_BADHEADER OV_EBADHEADER

#define SYNC_BUFFER_SIZE 4096
static int BRRCALL
i_get_first_page(FILE *const file, ogg_sync_state *const syncer, ogg_page *sync_page)
{
	int err = 0;
	brrsz bytes_read = 0;
	char *sync_buffer = NULL;
	while (SYNC_PAGEOUT_SUCCESS != (err = ogg_sync_pageout(syncer, sync_page)) && !feof(file)) {
		if (!(sync_buffer = ogg_sync_buffer(syncer, SYNC_BUFFER_SIZE))) {
			return I_BUFFER_ERROR;
		}
		bytes_read = fread(sync_buffer, 1, SYNC_BUFFER_SIZE, file);
		if (ferror(file)) {
			return I_IO_ERROR;
		} else if (SYNC_WROTE_SUCCESS != ogg_sync_wrote(syncer, bytes_read)) {
			return I_BUFFER_ERROR;
		}
	}
	if (feof(file) && bytes_read != 0 && err != SYNC_PAGEOUT_SUCCESS) {
		return I_FILE_TRUNCATED;
	}
	return I_SUCCESS;
}
static int BRRCALL
i_get_next_page(FILE *const file, ogg_sync_state *const syncer,
	ogg_page *const sync_page, ogg_stream_state *const stream)
{
	int err = 0;
	if ((err = i_get_first_page(file, syncer, sync_page))) {
		return err;
	} else if (STREAM_PAGEIN_SUCCESS != (err = ogg_stream_pagein(stream, sync_page))) {
		return I_BUFFER_ERROR; /* I think */
	}
	return I_SUCCESS;
}
static int BRRCALL
i_get_next_packet(FILE *const file, ogg_sync_state *const syncer,
    ogg_page *const sync_page, ogg_packet *const sync_packet,
    ogg_stream_state *const istream)
{
	int err = 0;
	while (STREAM_PACKETOUT_SUCCESS != (err = ogg_stream_packetout(istream, sync_packet))) {
		if (err == STREAM_PACKETOUT_INCOMPLETE) {
			if ((err = i_get_next_page(file, syncer, sync_page, istream))) {
				return err;
			}
		} else {
			BRRLOG_NOR("PACKET %zu", sync_packet->packetno);
			return I_DESYNC; /* Again, should desync be fatal? In headers, probably */
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_init_streams(FILE *const file, ogg_sync_state *const syncer, ogg_page *const sync_page,
    ogg_stream_state *const istream, ogg_stream_state *const ostream)
{
	int err = I_SUCCESS;
	if (!(err = i_get_first_page(file, syncer, sync_page))) {
		long page_ser = ogg_page_serialno(sync_page);
		if (STREAM_INIT_SUCCESS != ogg_stream_init(istream, page_ser)) {
			return I_INIT_ERROR;
		} else if (STREAM_INIT_SUCCESS != ogg_stream_init(ostream, page_ser)) {
			ogg_stream_clear(istream);
			return I_INIT_ERROR;
		}
	} else {
		return err;
	}
	if (STREAM_PAGEIN_SUCCESS != (err = ogg_stream_pagein(istream, sync_page))) {
		return I_BUFFER_ERROR;
	}
	return err;
}
static int BRRCALL
i_copy_header(ogg_packet *const sync_packet,
    ogg_stream_state *const ostream,
    vorbis_info *const info, vorbis_comment *const comment)
{
	int err = 0;
	if (VORBIS_SYNTHESIS_HEADERIN_SUCCESS != (err = vorbis_synthesis_headerin(info, comment, sync_packet))) {
		switch (err) {
			case VORBIS_SYNTHESIS_HEADERIN_FAULT: return I_BUFFER_ERROR;
			case VORBIS_SYNTHESIS_HEADERIN_NOTVORBIS: return I_NOT_VORBIS;
			case VORBIS_SYNTHESIS_HEADERIN_BADHEADER: return I_CORRUPT;
			default: return I_BAD_ERROR;
		}
	} else if (STREAM_PACKETIN_SUCCESS != (err = ogg_stream_packetin(ostream, sync_packet))) {
		return I_BUFFER_ERROR;
	}
	return I_SUCCESS;
}
static int BRRCALL
i_init_headers(FILE *const file, ogg_sync_state *const syncer,
    ogg_page *const sync_page, ogg_packet *const sync_packet,
    ogg_stream_state *const istream, ogg_stream_state *const ostream,
    vorbis_info *const info, vorbis_comment *const comment)
{
	int err = 0;

	vorbis_info_init(info);
	vorbis_comment_init(comment);
	for (int completed_headers = 0; completed_headers < 3; ++completed_headers) {
		if ((err = i_get_next_packet(file, syncer, sync_page, sync_packet, istream))) {
			vorbis_comment_clear(comment);
			vorbis_info_clear(info);
			return err;
		} else if (completed_headers == 0 && !vorbis_synthesis_idheader(sync_packet)) {
			vorbis_comment_clear(comment);
			vorbis_info_clear(info);
			return I_NOT_VORBIS;
		} else if ((err = i_copy_header(sync_packet, ostream, info, comment))) {
			vorbis_comment_clear(comment);
			vorbis_info_clear(info);
			return err;
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_recompute_grains(FILE *const file, ogg_sync_state *const syncer,
    ogg_page *const sync_page, ogg_packet *const sync_packet,
    ogg_stream_state *const istream, ogg_stream_state *const ostream,
    vorbis_info *const info, vorbis_comment *const comment)
{
	int err = I_SUCCESS;
	long last_blocksize = 0;
	brru8 total_grain = 0;
	brru8 total_packets = 0;
	while (I_SUCCESS == (err = i_get_next_packet(file, syncer, sync_page, sync_packet, istream))) {
		long current_blocksize = 0;
		current_blocksize = vorbis_packet_blocksize(info, sync_packet);
		if (last_blocksize)
			total_grain += (last_blocksize + current_blocksize) / 4;
		last_blocksize = current_blocksize;
		sync_packet->granulepos = total_grain;
		sync_packet->packetno = total_packets++;
		if (STREAM_PACKETIN_SUCCESS != ogg_stream_packetin(ostream, sync_packet)) {
			return I_BUFFER_ERROR;
		} else if (ogg_page_eos(sync_page)) {
			break;
		}
	}
	return err;
}
static void BRRCALL
i_clear(FILE **const in, FILE **const out, ogg_sync_state *const syncer,
    ogg_stream_state *const istream, ogg_stream_state *ostream,
    vorbis_info *const info, vorbis_comment *const comment)
{
	if (in && *in) {
		fclose(*in);
		*in = NULL;
	}
	if (out && *out) {
		fclose(*out);
		*out = NULL;
	}
	if (syncer)
		ogg_sync_clear(syncer);
	if (istream)
		ogg_stream_clear(istream);
	if (ostream)
		ogg_stream_clear(ostream);
	if (info)
		vorbis_info_clear(info);
	if (comment)
		vorbis_comment_clear(comment);
}
static int BRRCALL
i_regrain(const char *const input, const char *const output)
{
	int err = 0;
	FILE *in, *out;
	ogg_sync_state syncer;
	ogg_page sync_page;
	ogg_packet sync_packet;
	ogg_stream_state istream, ostream;
	vorbis_info info;
	vorbis_comment comment;
	if (!(in = fopen(input, "rb"))) {
		BRRLOG_ERRN("Failed to open ogg for regrain input '%s' : %s", input, strerror(errno));
		return I_IO_ERROR;
	}
	ogg_sync_init(&syncer);
	if ((err = i_init_streams(in, &syncer, &sync_page, &istream, &ostream))) {
		i_clear(&in, NULL, &syncer, NULL, NULL, NULL, NULL);
		BRRLOG_ERRN("Failed to initialize streams for regrain of '%s' : %s", input, i_strerr(err));
		return err;
	} else if ((err = i_init_headers(in, &syncer, &sync_page, &sync_packet, &istream, &ostream, &info, &comment))) {
		i_clear(&in, NULL, &syncer, &istream, &ostream, NULL, NULL);
		BRRLOG_ERRN("Failed to initialize vorbis headers from '%s' : %s", input, i_strerr(err));
		return err;
	} else if ((err = i_recompute_grains(in, &syncer, &sync_page, &sync_packet, &istream, &ostream, &info, &comment))) {
		i_clear(&in, NULL, &syncer, &istream, &ostream, &info, &comment);
		BRRLOG_ERRN("Failed to recompute granules of '%s' : %s", input, i_strerr(err));
		return err;
	}

	fclose(in);
	if (!(out = fopen(output, "wb"))) {
		i_clear(NULL, NULL, &syncer, &istream, &ostream, &info, &comment);
		BRRLOG_ERRN("Failed to open file for regrain output '%s' : %s", output, strerror(errno));
		return I_IO_ERROR;
	}

	while (ogg_stream_pageout(&ostream, &sync_page) || ogg_stream_flush(&ostream, &sync_page)) {
		if (sync_page.header_len != fwrite(sync_page.header, 1, sync_page.header_len, out)) {
			i_clear(NULL, &out, &syncer, &istream, &ostream, &info, &comment);
			BRRLOG_ERRN("Failed to write page header to output '%s' : %s", output, strerror(errno));
			return I_IO_ERROR;
		} else if (sync_page.body_len != fwrite(sync_page.body, 1, sync_page.body_len, out)) {
			i_clear(NULL, &out, &syncer, &istream, &ostream, &info, &comment);
			BRRLOG_ERRN("Failed to write page body to output '%s' : %s", output, strerror(errno));
			return I_IO_ERROR;
		}
	}
	i_clear(NULL, &out, &syncer, &istream, &ostream, &info, &comment);
	return I_SUCCESS;
}

int BRRCALL
regranularize_ogg(numbersT *const numbers, int dry_run, const char *const path,
    int inplace_regranularize)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	numbers->oggs_to_regranularize++;
	if (dry_run) {
		BRRLOG_FOREP(DRY_COLOR, " Regranularize OGG ");
	} else {
		brrsz inlen = 0, outlen = 0;
		BRRLOG_FORENP(WET_COLOR, " Regranularizing OGG... ");
		replace_ext(path, &inlen, output, &outlen, "_rvb.ogg");
		err = i_regrain(path, output);
		if (!err) {
			if (inplace_regranularize) {
				NeTODO("ANTIGRAIN IN-PLACE");
				/* remove 'path' and rename 'output' to 'path' */
			}
		}
	}
	if (!err) {
		numbers->oggs_regranularized++;
		BRRLOG_MESSAGETP(gbrrlog_level_last, SUCCESS_FORMAT, "Success!");
	} else {
		/* remove 'output' */
		BRRLOG_MESSAGETP(gbrrlog_level_last, FAILURE_FORMAT, " Failure! (%d)", err);
	}
	return err;
}
