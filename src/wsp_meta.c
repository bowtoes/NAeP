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

#include "wsp_meta.h"

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
wsp_meta_init(wsp_metaT *const wsp, const char *const buffer, brrsz buffer_size)
{
	wem_geometryT current = {0};
	const char *cur = buffer;
	if (!wsp)
		return I_GENERIC_ERROR;
	if (!buffer || buffer_size < 4)
		return I_INSUFFICIENT_DATA;
	while (current.offset < buffer_size - 4) {
		if (!(current.byteorder = riff_cc_byteorder(((brru4 *)cur)[0]))) {
			current.offset++;
			cur++;
			continue;
		}
		/* Found RIFF */
		current.size = ((brru4 *)cur)[1];
		if (current.byteorder == riff_byteorder_RIFX || current.byteorder == riff_byteorder_FFIR) {
			brru4 tmp;
			riff_copier_data(current.byteorder)(&tmp, &current.size, 4);
			current.size = tmp;
		}
		current.size += 8;
		if (current.offset + current.size > buffer_size) {
			BRRLOG_WAR("Miscounted/corrupted WSP, WEM #%zu, offset %zu + size %zu > WSP size %zu",
			    wsp->wem_count, current.offset, current.size, buffer_size);
			break;
		}
		if (brrlib_alloc((void **)&wsp->wems, (wsp->wem_count + 1) * sizeof(*wsp->wems), 0)) {
			wsp_meta_clear(wsp);
			return I_BUFFER_ERROR;
		}
		BRRLOG_DEBUG("Found WEM %zu at offset 0x%016X, %lu bytes", wsp->wem_count, current.offset, current.size);
		wsp->wems[wsp->wem_count++] = current;
		current.offset += current.size; /* I have no idea why I need to add 8 here */
		cur += current.size;
	}
	return I_SUCCESS;
}
void
wsp_meta_clear(wsp_metaT *const wsp)
{
	if (wsp->wems)
		free(wsp->wems);
	memset(wsp, 0, sizeof(*wsp));
}

static char s_output_file[BRRPATH_MAX_PATH + 1] = {0};
#define OUTPUT_FORMAT "_%0*zu"
int
wsp_meta_convert_wems(const wsp_metaT *const wsp, const char *const buffer,
    nestate_t *const state, const neinput_t *const input, const codebook_library_t *const library,
    const char *const output_root)
{
	if (!buffer)
		return I_INSUFFICIENT_DATA;
	if (!wsp || !state || !input || !output_root)
		return I_GENERIC_ERROR;

	int digits = brrnum_ndigits(wsp->wem_count, 0, 10);
	for (brrsz i = 0; i < wsp->wem_count; ++i) {
		const wem_geometryT *const wem = &wsp->wems[i];
		if (input->filter.count) {
			int contained = neinput_filter_contains(&input->filter, i);
			if ((contained && input->filter.type) || (!contained && !input->filter.type)) {
				BRRLOG_DEBUG("WEM %zu was filtered due to %slist", i, input->filter.type?"black":"white");
				continue;
			}
		}

		int err = 0;
		state->stats.wem_converts.assigned++;
		riff_t rf = {0};
		if ((err = lib_parse_buffer_as_riff(&rf, buffer + wem->offset, wem->size))) {
			BRRLOG_ERRN("Failed to parse wem ");
			LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
			BRRLOG_ERRP(" skipping : %s", lib_strerr(err));
		} else {
			ogg_stream_state streamer;
			if (!(err = wwise_convert_wwriff(&rf, &streamer, library, input))) {
				snprintf(s_output_file, sizeof(s_output_file), "%s"OUTPUT_FORMAT".ogg", output_root, digits, i);
				if ((err = lib_write_ogg_out(&streamer, s_output_file))) {
					BRRLOG_ERRN("Failed to write converted wem ");
					LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
					BRRLOG_ERRP(" skipping : %s", lib_strerr(err));
				}
				ogg_stream_clear(&streamer);
			} else {
				BRRLOG_ERRN("Failed to convert wem ");
				LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
				BRRLOG_ERRP(" skipping : %s", lib_strerr(err));
			}
			riff_clear(&rf);
		}

		if (!err)
			state->stats.wem_converts.succeeded++;
		else
			state->stats.wem_converts.failed++;
	}
	return I_SUCCESS;
}
int
wsp_meta_extract_wems(const wsp_metaT *const wsp, const char *const buffer,
    nestate_t *const state, const neinput_t *const input,
    const char *const output_root)
{
	if (!buffer)
		return I_INSUFFICIENT_DATA;
	if (!wsp || !state || !input || !output_root)
		return I_GENERIC_ERROR;

	int digits = brrnum_ndigits(wsp->wem_count, 0, 10);
	for (brrsz i = 0; i < wsp->wem_count; ++i) {
		const wem_geometryT *const wem = &wsp->wems[i];
		brrsz wrote = 0;
		FILE *output = NULL;
		if (input->filter.count) {
			int contained = neinput_filter_contains(&input->filter, i);
			if ((contained && input->filter.type) || (!contained && !input->filter.type)) {
				BRRLOG_DEBUG("WEM %zu was filtered due to %slist", i, input->filter.type?"black":"white");
				continue;
			}
		}

		BRRLOG_DEBUG("Extracting WEM %zu, %lu bytes", i, wem->size);

		state->stats.wem_extracts.assigned++;
		snprintf(s_output_file, sizeof(s_output_file), "%s"OUTPUT_FORMAT".wem", output_root, digits, i);
		if (!(output = fopen(s_output_file, "wb"))) {
			BRRLOG_ERRN("Failed to open output wem ");
			LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
			BRRLOG_ERRP(" skipping : %s", strerror(errno));
			state->stats.wem_extracts.failed++;
		} else {
			if (wem->size != (wrote = fwrite(buffer + wem->offset, 1, wem->size, output))) {
				BRRLOG_ERRN("Failed to write to output wem ");
				LOG_FORMAT(LOG_PARAMS_INFO, "#%*zu", digits, i);
				BRRLOG_ERRP(" (%s) skipping : %s", s_output_file, strerror(errno));
				state->stats.wem_extracts.failed++;
			} else {
				state->stats.wem_extracts.succeeded++;
			}
			fclose(output);
		}
	}
	return I_SUCCESS;
}
