/*
Copyright 2021-2022 BowToes (bow.toes@mailfence.com)

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

#include "rifflist.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrnum.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "errors.h"
#include "lib.h"
#include "print.h"
#include "wwise.h"

int
rifflist_scan(rifflist_t *const out_list, const unsigned char *const buffer, brrsz buffer_size)
{
	if (!out_list)
		return I_GENERIC_ERROR;
	if (!buffer || buffer_size < 4)
		return I_INSUFFICIENT_DATA;


	rifflist_t l = {0};
	brrsz offset = 0;
	while (offset < buffer_size - 8) {
		const unsigned char *data = buffer + offset;
		riffgeometry_t current = {.buffer_offset = offset};
		/* Scan for a recognized RIFF head chunk */
		if ((current.byteorder = riff_cc_byteorder(((brru4 *)data)[0])) == riff_byteorder_unrecognized) {
			offset++;
			continue;
		}

		/* RIFF size is next u32 */
		current.riff_size = ((brru4 *)data)[1];
		if (current.byteorder == riff_byteorder_RIFX || current.byteorder == riff_byteorder_FFIR) {
			brru4 tmp;
			riff_copier_data(current.byteorder)(&tmp, &current.riff_size, 4);
			current.riff_size = tmp;
		}

		current.riff_size += 8; /* 8 = byteorder_bytes (4) + size_bytes (4) */
		if (current.riff_size > buffer_size - current.buffer_offset) {
			BRRLOG_WAR("Corrupted/incomplete RIFF in data, #%zu, offset %zu + size %zu > data size %zu", l.n_riffs, current.buffer_offset, current.riff_size, buffer_size);
			break;
		}

		if (brrlib_alloc((void **)&l.riffs, (l.n_riffs + 1) * sizeof(*l.riffs), 0)) {
			rifflist_clear(&l);
			return I_BUFFER_ERROR;
		}
		BRRLOG_DEBUG("Found RIFF %zu at offset 0x%016X, %lu bytes", l.n_riffs, current.buffer_offset, current.riff_size);
		l.riffs[l.n_riffs++] = current;
		offset += current.riff_size;
	}
	NeExtraPrint(DEB, "Scanned list of %zu WwRIFFs...", l.n_riffs);

	*out_list = l;
	return I_SUCCESS;
}
void
rifflist_clear(rifflist_t *const list)
{
	if (list->riffs)
		free(list->riffs);
	memset(list, 0, sizeof(*list));
}

/* TODO for each embedded WWRIFF processed, log a symbol for success and one for failure;
 * e.g. :
 *   Processing file.wsp... . . . X X . . . . X . . . X . . X X . ... etc.
 * */

static char s_output_file[BRRPATH_MAX_PATH + 1] = {0};
#define OUTPUT_FORMAT "_%0*zu"
int
rifflist_convert(
    const rifflist_t *const list,
    const unsigned char *const buffer,
    nestate_t *const state,
    const neinput_t *const input,
    const codebook_library_t *const library,
    const char *const output_root
)
{
	if (!buffer)
		return I_INSUFFICIENT_DATA;
	if (!list || !state || !input || !output_root)
		return I_GENERIC_ERROR;

	int digits = brrnum_ndigits(list->n_riffs, 10, 1);
	NeExtraPrint(DEB, "Converting WwRIFF list...");
	for (brrsz i = 0; i < list->n_riffs; ++i) {
		if (input->filter.count) {
			int contained = neinput_filter_contains(&input->filter, i);
			if ((contained && input->filter.type) || (!contained && !input->filter.type)) {
				BRRLOG_DEBUG("WWRIFF %zu was filtered due to %slist", i, input->filter.type?"black":"white");
				continue;
			}
		}

		snprintf(s_output_file, sizeof(s_output_file), "%s"OUTPUT_FORMAT".ogg", output_root, digits, i);
		state->stats.wem_converts.assigned++;

		int err = 0;
		wwriff_t wwriff = {0};
		const riffgeometry_t *const geom = &list->riffs[i];
		if ((err = lib_parse_buffer_as_wwriff(&wwriff, buffer + geom->buffer_offset, geom->riff_size))) {
			BRRLOG_ERRN("Failed to parse WWRIFF ");
			LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
			BRRLOG_ERRP(" skipping : %s", lib_strerr(err));
		} else {
			if (input->flag.add_comments) {
				if ((err = wwriff_add_comment(&wwriff, "SourceFile=%s", input->path))) {
					BRRLOG_ERR("Failed to add comment to WWRIFF : %s (%d)", strerror(errno), errno);
				} else if ((err = wwriff_add_comment(&wwriff, "OutputFile=%s", s_output_file))) {
					BRRLOG_ERR("Failed to add comment to WWRIFF : %s (%d)", strerror(errno), errno);
				}
			}
			if (!err) {
				ogg_stream_state streamer;
				if (!(err = wwise_convert_wwriff(&wwriff, &streamer, library, input))) {
					if ((err = lib_write_ogg_out(&streamer, s_output_file))) {
						BRRLOG_ERRN("Failed to write converted WWRIFF ");
						LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
						BRRLOG_ERRP(", skipping.");
					}
					ogg_stream_clear(&streamer);
				} else {
					BRRLOG_ERRN("Failed to convert WWRIFF ");
					LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
					BRRLOG_ERRP(", skipping.");
				}
			}
			wwriff_clear(&wwriff);
		}

		if (!err) {
			NeExtraPrint(DEB, "Successfuly converted WwRIFF to '%s'", s_output_file);
			state->stats.wem_converts.succeeded++;
		} else {
			NeExtraPrint(DEB, "Failed to convert WwRIFF to '%s'", s_output_file);
			state->stats.wem_converts.failed++;
		}
	}
	return I_SUCCESS;
}
int
rifflist_extract(
    const rifflist_t *const list,
    const unsigned char *const buffer,
    nestate_t *const state,
    const neinput_t *const input,
    const char *const output_root
)
{
	if (!buffer)
		return I_INSUFFICIENT_DATA;
	if (!list || !state || !input || !output_root)
		return I_GENERIC_ERROR;

	int digits = brrnum_ndigits(list->n_riffs, 10, 0);
	NeExtraPrint(DEB, "Extracting WwRIFF list...");
	for (brrsz i = 0; i < list->n_riffs; ++i) {
		const riffgeometry_t *const wem = &list->riffs[i];
		brrsz wrote = 0;
		FILE *output = NULL;
		if (input->filter.count) {
			int contained = neinput_filter_contains(&input->filter, i);
			if ((contained && input->filter.type) || (!contained && !input->filter.type)) {
				BRRLOG_DEBUG("WWRIFF %zu was filtered due to %slist", i, input->filter.type?"black":"white");
				continue;
			}
		}

		snprintf(s_output_file, sizeof(s_output_file), "%s"OUTPUT_FORMAT".wem", output_root, digits, i);
		state->stats.wem_extracts.assigned++;

		if (!(output = fopen(s_output_file, "wb"))) {
			BRRLOG_ERRN("Failed to open output WEM ");
			LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
			BRRLOG_ERRP(" (%s), skipping", s_output_file);
			state->stats.wem_extracts.failed++;
		} else {
			if (wem->riff_size != (wrote = fwrite(buffer + wem->buffer_offset, 1, wem->riff_size, output))) {
				BRRLOG_ERRN("Failed to write to output WEM ");
				LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
				BRRLOG_ERRP(" (%s), skipping", s_output_file);
				NeExtraPrint(DEB, "Failed to extract WwRIFF to '%s'", s_output_file);
				state->stats.wem_extracts.failed++;
			} else {
				NeExtraPrint(DEB, "Successfuly extracted WwRIFF to '%s'", s_output_file);
				state->stats.wem_extracts.succeeded++;
			}
			fclose(output);
		}
	}
	return I_SUCCESS;
}
