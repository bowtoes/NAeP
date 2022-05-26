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

/* TODO remove ginput_name and all references to it */
static const char *s_input_name = NULL;
static char s_output_name[BRRPATH_MAX_PATH + 1] = {0};

// There should be extended error logging that I think should be optional
// I'm thinking a tiered error system, with '+E, +error' and '-E, -error' like with the quiet options

typedef struct i_state {
	FILE *input;
	ogg_stream_state input_stream;
	ogg_stream_state output_stream;
	ogg_sync_state sync;
	ogg_page current_page;
	ogg_packet current_packet;
	vorbis_info vi;
	vorbis_comment vc;
} i_state_t;

/* Generalize processing steps:
 *   Take pages from input file (ogg_sync_state)
 *   Put those pages into an input stream
 *   For each packet in that stream:
 *     Regrain, insert into output stream.
 *   Write the output stream to disk.
 * */

#define SYNC_BUFFER_SIZE 4096
static inline int
i_inc_page(i_state_t *const state)
{
	int err = 0;
	brrsz bytes_read = 0;
	while (SYNC_PAGEOUT_SUCCESS != (err = ogg_sync_pageout(&state->sync, &state->current_page)) && !feof(state->input)) {
		char *sync_buffer = NULL;
		if (!(sync_buffer = ogg_sync_buffer(&state->sync, SYNC_BUFFER_SIZE))) {
			NeExtraPrint(ERR, "Could not init sync buffer");
			return I_BUFFER_ERROR;
		}

		bytes_read = fread(sync_buffer, 1, SYNC_BUFFER_SIZE, state->input);
		if (ferror(state->input)) {
			NeExtraPrint(ERR, "Failed to read page from input : %s", strerror(errno));
			return I_IO_ERROR;
		}

		if (SYNC_WROTE_SUCCESS != ogg_sync_wrote(&state->sync, bytes_read)) {
			NeExtraPrint(ERR, "Failed to apply input page buffer");
			return I_BUFFER_ERROR;
		}
	}

	if (feof(state->input) && bytes_read != 0 && err != SYNC_PAGEOUT_SUCCESS) {
		NeExtraPrint(ERR, "Last page truncated");
		return I_FILE_TRUNCATED;
	}
	return 0;
}

static inline int
i_inc_packet(i_state_t *const state)
{
	int err = 0;
	while (STREAM_PACKETOUT_SUCCESS != (err = ogg_stream_packetout(&state->input_stream, &state->current_packet))) {
		if (err == STREAM_PACKETOUT_INCOMPLETE) {
			if ((err = i_inc_page(state))) {
				BRRLOG_ERR("Could not get next page from input");
				return err;
			}
			if (STREAM_PAGEIN_SUCCESS != (err = ogg_stream_pagein(&state->input_stream, &state->current_page))) {
				BRRLOG_ERR("Could not insert page into stream");
				return I_BUFFER_ERROR; /* I think */
			}
		} else {
			return I_DESYNC; /* Again, should desync be fatal? In headers, probably */
		}
	}
	return 0;
}

static inline void
i_state_clear(i_state_t *const state)
{
	if (state) {
		if (state->input)
			fclose(state->input);
		 /* libogg never actually uses this, and I don't think it's necessary for me to either;
		  * ogg_packet_clear doesn't even check if the packet actually has a valid pointer to free first
		  * (the others do). */
		// ogg_packet_clear(&state->current_packet);
		vorbis_comment_clear(&state->vc);
		vorbis_info_clear(&state->vi);
		ogg_sync_clear(&state->sync);
		ogg_stream_clear(&state->input_stream);
		ogg_stream_clear(&state->output_stream);
		memset(state, 0, sizeof(*state));
	}
}

static inline int
i_state_init(i_state_t *const state, const char *const input)
{
	i_state_t s = {0};
	if (!(s.input = fopen(input, "rb"))) {
		BRRLOG_ERR("Failed to open input ogg for regrain : %s (%d)", strerror(errno), errno);
		return I_IO_ERROR;
	}

	ogg_sync_init(&s.sync); /* cannot fail */

	{ /* Get first page of input. */
		int err = 0;
		if ((err = i_inc_page(&s))) {
			i_state_clear(&s);
			return err;
		}
	}

	long page_ser = ogg_page_serialno(&s.current_page);
	if (STREAM_INIT_SUCCESS != ogg_stream_init(&s.input_stream, page_ser)) {
		i_state_clear(&s);
		NeExtraPrint(ERR, "Could not init input stream ");
		return I_INIT_ERROR;
	}
	if (STREAM_PAGEIN_SUCCESS != ogg_stream_pagein(&s.input_stream, &s.current_page)) {
		i_state_clear(&s);
		NeExtraPrint(ERR, "Could not read first input page");
		return I_BUFFER_ERROR;
	}
	if (STREAM_INIT_SUCCESS != ogg_stream_init(&s.output_stream, page_ser)) {
		i_state_clear(&s);
		NeExtraPrint(ERR, "Could not init output stream ");
		return I_INIT_ERROR;
	}
	*state = s;
	return 0;
}

static inline int
i_state_process(i_state_t *const state)
{
	int err = 0;
	vorbis_info vi = {0};
	vorbis_comment vc = {0};
	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);
	/* Copy the headers */
	for (int current_header = vorbis_header_packet_id; current_header < 3; ++current_header) {
		if ((err = i_inc_packet(state))) {
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			NeExtraPrint(ERR, "Failed to get vorbis %s header.", vorbis_header(current_header));
			return err;
		}
		if (current_header == vorbis_header_packet_id) {
			if (!vorbis_synthesis_idheader(&state->current_packet)) {
				vorbis_comment_clear(&vc);
				vorbis_info_clear(&vi);
				NeExtraPrint(ERR, "Bad vorbis ID header.");
				return I_NOT_VORBIS;
			}
		}
		if (VORBIS_SYNTHESIS_HEADERIN_SUCCESS != (err = vorbis_synthesis_headerin(&vi, &vc, &state->current_packet))) {
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			switch (err) {
				case VORBIS_SYNTHESIS_HEADERIN_FAULT:
					NeExtraPrint(ERR, "Fault while synthesizing %s header from input.", vorbis_header(current_header));
					return I_BUFFER_ERROR;
				case VORBIS_SYNTHESIS_HEADERIN_NOTVORBIS:
					NeExtraPrint(ERR, "Got invalid %s header from input.", vorbis_header(current_header));
					return I_NOT_VORBIS;
				case VORBIS_SYNTHESIS_HEADERIN_BADHEADER:
					NeExtraPrint(ERR, "Got bad/corrupt %s header from input.", vorbis_header(current_header));
					return I_CORRUPT;
				default:
					return I_BAD_ERROR;
			}
		}
		if (STREAM_PACKETIN_SUCCESS != (err = ogg_stream_packetin(&state->output_stream, &state->current_packet))) {
			NeExtraPrint(ERR, "Failed to copy vorbis %s header packet.", vorbis_header(current_header));
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			return I_BUFFER_ERROR;
		}
	}
	state->vi = vi;
	state->vc = vc;

	/* Recompute granules */
	long last_block = 0;
	brru8 total_block = 0;
	brru8 total_packets = 0;
	while (!state->current_packet.e_o_s && !(err = i_inc_packet(state))) {
		long current_block = vorbis_packet_blocksize(&state->vi, &state->current_packet);
		state->current_packet.granulepos = total_block;
		state->current_packet.packetno = total_packets++;
		if (last_block)
			total_block += (last_block + current_block) / 4;
		last_block = current_block;
		NeExtraPrint(DEB, "Granulepos: %llu | Block: %llu | Total block: %llu", state->current_packet.granulepos, current_block, total_block);
		if (STREAM_PACKETIN_SUCCESS != ogg_stream_packetin(&state->output_stream, &state->current_packet))
			return I_BUFFER_ERROR;
	}
	return 0;
}

static inline int
i_regrain(void)
{
	int err = 0;
	i_state_t state = {0};
	if (!(err = i_state_init(&state, s_input_name))) {
		if (!(err = i_state_process(&state))) {
			err = lib_write_ogg_out(&state.output_stream, s_output_name);
		}
	}
	i_state_clear(&state);
	return err;
}

int
neregrain_ogg(nestate_t *const state, const neinput_t *const input)
{
	int err = 0;
	state->stats.oggs.assigned++;
	if (input->flag.dry_run) {
		LOG_FORMAT(LOG_PARAMS_DRY, "Regranularize OGG ");
	} else {
		LOG_FORMAT(LOG_PARAMS_WET, "Regranularizing OGG... ");
		s_input_name = input->path;
		if (input->flag.inplace_regrain)
			snprintf(s_output_name, sizeof(s_output_name), "%s", input->path);
		else
			lib_replace_ext(s_input_name, strlen(input->path), s_output_name, NULL, "_rvb.ogg");
		err = i_regrain();
	}

	if (!err) {
		state->stats.oggs.succeeded++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, "Success!\n");
	} else {
		state->stats.oggs.failed++;
		LOG_FORMAT(LOG_PARAMS_FAILURE, " Failure! (%d)\n", err);
	}
	return err;
}
