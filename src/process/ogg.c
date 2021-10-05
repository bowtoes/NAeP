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

#include "process.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <vorbis/vorbisenc.h>

#include <brrtools/brrpath.h>

#include "errors.h"
#include "lib.h"
#include "print.h"

static const char *ginput_name = NULL;
static char goutput_name[BRRPATH_MAX_PATH + 1] = {0};

// There should be extended error logging that I think should be optional
// I'm thinking a tiered error system, with '+E, +error' and '-E, -error' like with the quiet options

#define SYNC_BUFFER_SIZE 4096
static int BRRCALL
i_get_first_page(FILE *const file, ogg_sync_state *const syncer, ogg_page *sync_page)
{
	int err = 0;
	brrsz bytes_read = 0;
	char *sync_buffer = NULL;
	while (SYNC_PAGEOUT_SUCCESS != (err = ogg_sync_pageout(syncer, sync_page)) && !feof(file)) {
		if (!(sync_buffer = ogg_sync_buffer(syncer, SYNC_BUFFER_SIZE))) {
			// ("Could not init sync buffer");
			return I_BUFFER_ERROR;
		}
		bytes_read = fread(sync_buffer, 1, SYNC_BUFFER_SIZE, file);
		if (ferror(file)) {
			// ("Failed to read page from input : %s", strerror(errno));
			return I_IO_ERROR;
		} else if (SYNC_WROTE_SUCCESS != ogg_sync_wrote(syncer, bytes_read)) {
			// ("Failed to apply input page buffer");
			return I_BUFFER_ERROR;
		}
	}
	if (feof(file) && bytes_read != 0 && err != SYNC_PAGEOUT_SUCCESS) {
		// ("Last page truncated");
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
		BRRLOG_ERRN("Could not get next page from input");
		return err;
	} else if (STREAM_PAGEIN_SUCCESS != (err = ogg_stream_pagein(stream, sync_page))) {
		BRRLOG_ERRN("Could not insert page into stream");
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
			// ("Could not init input stream ");
			return I_INIT_ERROR;
		} else if (STREAM_INIT_SUCCESS != ogg_stream_init(ostream, page_ser)) {
			ogg_stream_clear(istream);
			// ("Could not init output stream ");
			return I_INIT_ERROR;
		}
	} else {
		return err;
	}
	if (STREAM_PAGEIN_SUCCESS != (err = ogg_stream_pagein(istream, sync_page))) {
		// ("Could not read first input page");
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
			case VORBIS_SYNTHESIS_HEADERIN_FAULT:
				// ("Fault while synthesizing header from input ");
				return I_BUFFER_ERROR;
			case VORBIS_SYNTHESIS_HEADERIN_NOTVORBIS:
				// ("Input stream failed to pass as vorbis ");
				return I_NOT_VORBIS;
			case VORBIS_SYNTHESIS_HEADERIN_BADHEADER:
				// ("Input had bad header ");
				return I_CORRUPT;
			default: return I_BAD_ERROR;
		}
	} else if (STREAM_PACKETIN_SUCCESS != (err = ogg_stream_packetin(ostream, sync_packet))) {
		// ("Could not copy header packet ");
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
			// ("Could not copy header %d", completed_headers + 1);
			return err;
		} else if (completed_headers == 0 && !vorbis_synthesis_idheader(sync_packet)) {
			vorbis_comment_clear(comment);
			vorbis_info_clear(info);
			// ("Could not copy header %d", completed_headers + 1);
			return I_NOT_VORBIS;
		} else if ((err = i_copy_header(sync_packet, ostream, info, comment))) {
			vorbis_comment_clear(comment);
			vorbis_info_clear(info);
			// ("Could not copy header %d", completed_headers + 1);
			return err;
		}
	}
	return I_SUCCESS;
}
static int BRRCALL
i_recompute_grains(FILE *const file, ogg_sync_state *const syncer,
    ogg_page *const sync_page, ogg_packet *const sync_packet,
    ogg_stream_state *const istream, ogg_stream_state *const ostream,
    vorbis_info *const info)
{
	int err = I_SUCCESS;
	long last_blocksize = 0;
	brru8 total_grain = 0;
	brru8 total_packets = 0;
	while (I_SUCCESS == (err = i_get_next_packet(file, syncer, sync_page, sync_packet, istream))) {
		long current_blocksize = current_blocksize = vorbis_packet_blocksize(info, sync_packet);
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
i_clear(FILE **const in, ogg_sync_state *const syncer,
    ogg_stream_state *const istream, ogg_stream_state *ostream,
    vorbis_info *const info, vorbis_comment *const comment)
{
	if (in && *in) {
		fclose(*in);
		*in = NULL;
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
i_regrain(void)
{
	int err = 0;
	FILE *in;
	ogg_sync_state syncer;
	ogg_page sync_page;
	ogg_packet sync_packet;
	ogg_stream_state istream, ostream;
	vorbis_info info;
	vorbis_comment comment;
	if (!(in = fopen(ginput_name, "rb"))) {
		BRRLOG_ERRN("Failed to open ogg for regrain input '%s' : %s", ginput_name, strerror(errno));
		return I_IO_ERROR;
	}
	ogg_sync_init(&syncer);
	if ((err = i_init_streams(in, &syncer, &sync_page, &istream, &ostream))) {
		i_clear(&in, &syncer, NULL, NULL, NULL, NULL);
		BRRLOG_ERRN("Failed to initialize streams for regrain of '%s' : %s", ginput_name, lib_strerr(err));
		return err;
	} else if ((err = i_init_headers(in, &syncer, &sync_page, &sync_packet, &istream, &ostream, &info, &comment))) {
		i_clear(&in, &syncer, &istream, &ostream, NULL, NULL);
		BRRLOG_ERRN("Failed to initialize vorbis headers from '%s' : %s", ginput_name, lib_strerr(err));
		return err;
	} else if ((err = i_recompute_grains(in, &syncer, &sync_page, &sync_packet, &istream, &ostream, &info))) {
		i_clear(&in, &syncer, &istream, &ostream, &info, &comment);
		BRRLOG_ERRN("Failed to recompute granules of '%s' : %s", ginput_name, lib_strerr(err));
		return err;
	}
	i_clear(&in, &syncer, &istream, NULL, &info, &comment);
	err = lib_write_ogg_out(&ostream, goutput_name);
	i_clear(NULL, NULL, NULL, &ostream, NULL, NULL);
	return err;
}

int BRRCALL
neregrain_ogg(nestateT *const state, const neinputT *const input)
{
	int err = 0;
	state->oggs_to_regrain++;
	if (input->dry_run) {
		LOG_FORMAT(LOG_PARAMS_DRY, "Regranularize OGG ");
	} else {
		LOG_FORMAT(LOG_PARAMS_WET, "Regranularizing OGG... ");
		ginput_name = input->path;
		if (input->inplace_regrain) {
			snprintf(goutput_name, sizeof(goutput_name), "%s", input->path);
		} else {
			lib_replace_ext(ginput_name, strlen(input->path), goutput_name, NULL, "_rvb.ogg");
		}
		err = i_regrain();
	}
	if (!err) {
		state->oggs_regrained++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, "Success!");
	} else {
		state->oggs_failed++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, " Failure! (%d)", err);
	}
	return err;
}
