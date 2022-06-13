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

#include "process.h"

#include <errno.h>
#include <stdio.h>

#include <brrtools/brrlog.h>

#include "neinput.h"

static inline int
i_determine_input_type(neinput_t *const input)
{
	{
		brru4 ext = 0;
		if (-1 != (ext = nepath_extension_cmp(&input->path, NULL,
			    "ogg", "wem", "wsp", "bnk", "arc", "pck",
			NULL))) {
			switch (ext) {
				case 0: input->data_type = nedatatype_ogg; break;
				case 1: input->data_type = nedatatype_wem; break;
				case 2: input->data_type = nedatatype_wsp; break;
				case 3: input->data_type = nedatatype_bnk; break;
				case 4: input->data_type = nedatatype_arc; break;
				case 5: input->data_type = nedatatype_arc; break;
			}
			return 0;
		}
	}

	fcc_t cc = {.n=4};
	{
		FILE *fp = NULL;
		if (!(fp = fopen(input->path.cstr, "rb"))) {
			Err(,"Could not open '%s' to determine data type: %s (%d)", input->path.cstr, strerror(errno), errno);
			return -1;
		}
		if (4 != fread(&cc.v, 1, 4, fp)) {
			fclose(fp);
			if (ferror(fp)) {
				Err(,"Failed to read fourcc from '%s' to determine data type: %s (%d)", input->path.cstr, strerror(errno), errno);
				return -1;
			} else {
				Err(,"'%s' is too small to contain valid data");
				return -1;
			}
		}
		fclose(fp);
	}

	if (!fcccmp(cc, fcc_OggS)) {
		input->data_type = nedatatype_ogg;
	} else if (
	    !fcccmp(cc, fcc_RIFF) || !fcccmp(cc, fcc_RIFX) ||
	    !fcccmp(cc, fcc_XFIR) || !fcccmp(cc, fcc_FFIR)
	) {
		input->data_type = nedatatype_wem;
	} else if (!fcccmp(cc, fcc_BKHD)) {
		input->data_type = nedatatype_bnk;
	//} else if (!memcmp(&cc, &zframe_magic.integer, 4)) {
	//	input->data_type = nedatatype_arc;
	} else {
		Err(,"Unkown data in '%s'");
		return -1;
	}
	return 0;
}

int
neprocess_inputs(nestate_t *const state)
{
	for (brrsz i = 0; i < state->n_inputs; ++i) {
		neinput_t *const input = &state->inputs[i];
		{ // Set log state
			if (state->settings.log_style_enabled)
				gbrrlogctl.style_disabled = !input->flag.log_color_enabled;
		#if defined(Ne_debug)
			gbrrlogctl.debug_enabled = 1;
			brrlog_set_max_priority(brrlog_priority_debug);
		#else
			gbrrlogctl.debug_enabled = input->flag.log_debug;
			brrlog_set_max_priority(input->log_priority);
		#endif
		}

		if (!input->data_type) {
			if (i_determine_input_type(input))
				continue;
		}

		{ // Process input
			Nor(n,"Processing input ");
			Style(np, extra_info, "%*zu / %zu", state->stats.n_input_digits, i+1, state->n_inputs);

			#define _log_boiler(_type_) Style(np,_type_, "%-*s", state->stats.input_path_max, input->path)
			switch (input->data_type) {
				case nedatatype_ogg: _log_boiler(ft_ogg); return neprocess_ogg(state, input);
				case nedatatype_wem: _log_boiler(ft_wem); return neprocess_wem(state, input);
				case nedatatype_wsp: _log_boiler(ft_wsp); return neprocess_wsp(state, input);
				case nedatatype_bnk: _log_boiler(ft_bnk); return neprocess_bnk(state, input);
				default: break;
			}
		}
	}
	return 0;
}
