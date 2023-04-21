/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "process.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <vorbis/vorbisenc.h>
#include <brrtools/brrnum.h>

#include "neinput.h"
#include "nelog.h"
#include "neutil.h"
#include "wwise.h"

/* TODO remove ginput_name and all references to it */
static const char *s_input_name = NULL;
static char s_output_name[brrpath_max_path + 1] = {0};

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
	while (E_OGG_OUT_SUCCESS != (err = ogg_sync_pageout(&state->sync, &state->current_page)) && !feof(state->input)) {
		char *sync_buffer = NULL;
		if (!(sync_buffer = ogg_sync_buffer(&state->sync, SYNC_BUFFER_SIZE))) {
			XErr(,"Could not init sync buffer");
			return -1;
		}

		bytes_read = fread(sync_buffer, 1, SYNC_BUFFER_SIZE, state->input);
		if (ferror(state->input)) {
			XErr(,"Failed to read page from input : %s", strerror(errno));
			return -1;
		}

		if (E_OGG_SUCCESS != ogg_sync_wrote(&state->sync, bytes_read)) {
			XErr(,"Failed to apply input page buffer");
			return -1;
		}
	}

	if (feof(state->input) && bytes_read != 0 && err != E_OGG_OUT_SUCCESS) {
		XErr(,"Last page truncated");
		return -1;
	}
	return 0;
}

static inline int
i_inc_packet(i_state_t *const state)
{
	int err = 0;
	while (E_OGG_OUT_SUCCESS != (err = ogg_stream_packetout(&state->input_stream, &state->current_packet))) {
		if (err == E_OGG_OUT_INCOMPLETE) {
			if (i_inc_page(state)) {
				Err(,"Could not get next page from input");
				return -1;
			}
			if (E_OGG_SUCCESS != (err = ogg_stream_pagein(&state->input_stream, &state->current_page))) {
				Err(,"Could not insert page into stream");
				return -1; /* I think */
			}
		} else { /* Desync */
			return -1; /* Should desync be fatal? In the vorbis headers, probably */
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
		Err(,"Failed to open input ogg for regrain : %s (%d)", strerror(errno), errno);
		return -1;
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
	if (E_OGG_SUCCESS != ogg_stream_init(&s.input_stream, page_ser)) {
		i_state_clear(&s);
		XErr(,"Could not init input stream ");
		return -1;
	}
	if (E_OGG_SUCCESS != ogg_stream_pagein(&s.input_stream, &s.current_page)) {
		i_state_clear(&s);
		XErr(,"Could not read first input page");
		return -1;
	}
	if (E_OGG_SUCCESS != ogg_stream_init(&s.output_stream, page_ser)) {
		i_state_clear(&s);
		XErr(,"Could not init output stream");
		return -1;
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
	for (int current_header = vorbishdr_id; current_header < 3; ++current_header) {
		if (i_inc_packet(state)) {
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			XErr(,"Failed to get vorbis %s header", vorbishdr(current_header));
			return -1;
		}
		if (current_header == vorbishdr_id) {
			if (!vorbis_synthesis_idheader(&state->current_packet)) {
				vorbis_comment_clear(&vc);
				vorbis_info_clear(&vi);
				XErr(,"Bad vorbis ID header");
				return -1;
			}
		}
		if (E_VORBIS_HEADER_SUCCESS != (err = vorbis_synthesis_headerin(&vi, &vc, &state->current_packet))) {
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			switch (err) {
				case E_VORBIS_HEADER_FAULT:     XErr(,"Fault while synthesizing %s header from input", vorbishdr(current_header)); break;
				case E_VORBIS_HEADER_NOTVORBIS: XErr(,"Got invalid %s header from input", vorbishdr(current_header)); break;
				case E_VORBIS_HEADER_BADHEADER: XErr(,"Got bad/corrupt %s header from input", vorbishdr(current_header)); break;
				default: break;
			}
			return -1;
		}
		if (E_OGG_SUCCESS != (err = ogg_stream_packetin(&state->output_stream, &state->current_packet))) {
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			XErr(,"Failed to copy vorbis %s header packet", vorbishdr(current_header));
			return -1;
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
		XDeb(,"Granulepos: %llu | Block: %llu | Total block: %llu", state->current_packet.granulepos, current_block, total_block);
		if (E_OGG_SUCCESS != ogg_stream_packetin(&state->output_stream, &state->current_packet))
			return -1;
	}
	return 0;
}

static inline int
i_regrain(void)
{
	int err = 0;
	i_state_t state = {0};
	if (!i_state_init(&state, s_input_name)) {
		if (!i_state_process(&state)) {
			err = neutil_write_ogg(&state.output_stream, s_output_name);
		}
		i_state_clear(&state);
	}
	return err;
}

#define RVB_EXT "_rvb.ogg"
int
neprocess_ogg(nestate_t *const state, const neinput_t *const input)
{
	int err = 0;
	state->stats.oggs.assigned++;
	if (input->cfg.dry_run) {
		Nor(p,"(!"nest_meta_dry":Regranularize OGG (dry)!) ");
	} else {
		Nor(p,"(!"nest_meta_wet":Regranularizing OGG...!) ");
		s_input_name = input->path.full;
		if (input->cfg.inplace_regrain) {
			/* Overwrite input file */
			brrsz l = brrnum_umin(sizeof(s_output_name)-1, input->path.len);
			memcpy(s_output_name, input->path.full, l);
			s_output_name[l] = 0;
		} else {
			/* Output to [file_path/base_name]_rvb.ogg */
			neutil_replace_extension(&input->path, RVB_EXT, sizeof(RVB_EXT) - 1, 1, s_output_name, sizeof(s_output_name) - 1);
		}
		err = i_regrain();
	}

	if (!err) {
		state->stats.oggs.succeeded++;
		Nor(p,"(!"nest_meta_success":Success!!)");
	} else {
		state->stats.oggs.failed++;
		Nor(p,"(!"nest_meta_failure": Failure! (%d)!)", err);
	}
	return err;
}
