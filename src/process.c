/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "process.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "neinput.h"
#include "nelog.h"
#include "riff.h"

const fcc_t fcc_OggS = fcc_str(,"OggS");
const fcc_t fcc_BKHD = fcc_str(,"BKHD");

static inline int
i_determine_input_type(neinput_t *const input)
{
	{
		int ext = 0;
		if (-1 != (ext = brrpath_match_extension(&input->path, 1, 0, NULL,
			    "ogg", "wem", "wsp", "bnk", "arc", "pck",
			NULL))) {
			switch (ext) {
				case 0: input->cfg.data_type = nedatatype_ogg; break;
				case 1: input->cfg.data_type = nedatatype_wem; break;
				case 2: input->cfg.data_type = nedatatype_wsp; break;
				case 3: input->cfg.data_type = nedatatype_bnk; break;
				case 4: input->cfg.data_type = nedatatype_arc; break;
				case 5: input->cfg.data_type = nedatatype_arc; break;
			}
			return 0;
		}
	}
	/* Can't determine type based on extension, resort to checking the fourcc */

	fcc_t cc = {.n=4};
	{
		FILE *fp = NULL;
		if (!(fp = fopen(input->path.full, "rb"))) {
			Err(,"Could not open (!f=y:'%s'!) to determine data type: %s ((!f=r:%d!))", input->path.full, strerror(errno), errno);
			return -1;
		}
		int e = 0;
		if (4 != fread(&cc.v, 1, 4, fp)) {
			if (ferror(fp)) {
				Err(,"Failed to read fourcc from (!f=y:'%s'!) to determine data type: %s ((!f=r:%d!))", input->path.full, strerror(errno), errno);
			} else {
				Err(,"!(f=y:'%s'!) is too small to contain valid data");
			}
			e = 1;
		}
		fclose(fp);
		if (e)
			return -1;
	}

	if (!fcccmp(cc, fcc_OggS)) {
		input->cfg.data_type = nedatatype_ogg;
	} else if (
	    !fcccmp(cc, fcc_RIFF) || !fcccmp(cc, fcc_RIFX) ||
	    !fcccmp(cc, fcc_XFIR) || !fcccmp(cc, fcc_FFIR)
	) {
		input->cfg.data_type = nedatatype_wem;
	} else if (!fcccmp(cc, fcc_BKHD)) {
		input->cfg.data_type = nedatatype_bnk;
	//} else if (!memcmp(&cc, &zframe_magic.integer, 4)) {
	//	input->data_type = nedatatype_arc;
	} else {
		Err(,"Unkown data in (!f=y:'%s'!)");
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
			if (state->cfg.log_style_enabled)
				brrlog_config.style_enabled = input->cfg.log_color_enabled;
		#if defined(Ne_debug)
			brrlog_config.debug = 1;
			brrlog_maxlabel(logpri_debug);
		#else
			brrlog_config.debug = input->cfg.log_debug;
			brrlog_maxlabel(input->log_label);
		#endif
		}

		if (!input->cfg.data_type) {
			if (i_determine_input_type(input))
				continue;
		}

		{ // Process input
			int err = 0;

			Nor(,"Processing input (!f=m:%*zu / %zu!) ", state->stats.n_input_digits, i + 1, state->n_inputs);

			#define _log_boiler(_type_) do { Nor(np, "(!" nest_filetype_##_type_ ":%-*s!) ", state->stats.input_path_max, input->path.full); } while(0)
			switch (input->cfg.data_type) {
				case nedatatype_ogg: _log_boiler(ogg); /* flush_log_queue(); */ err = neprocess_ogg(state, input); break;
				case nedatatype_wem: _log_boiler(wem); /* flush_log_queue(); */ err = neprocess_wem(state, input); break;
				case nedatatype_wsp: _log_boiler(wsp); /* flush_log_queue(); */ err = neprocess_wsp(state, input); break;
				case nedatatype_bnk: _log_boiler(bnk); /* flush_log_queue(); */ err = neprocess_bnk(state, input); break;
				default: break;
			}
			if (err)
				return err;
		}
	}
	return 0;
}
