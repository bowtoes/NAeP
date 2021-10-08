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

#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "lib.h"
#include "errors.h"
#include "print.h"
#include "wsp_meta.h"
#include "wwise.h"

static char goutput_root[BRRPATH_MAX_PATH + 1] = {0};

static int
i_extract_wsp(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input)
{
	int err = 0;
	wsp_metaT meta = {0};
	const codebook_libraryT *library = NULL;
	char *buffer = NULL;
	brrsz bufsize = 0;

	if ((err = lib_read_entire_file(input->path, (void **)&buffer, &bufsize)))
		return err;
	if (!(err = wsp_meta_init(&meta, buffer, bufsize))) {
		if (!(err = neinput_load_codebooks(libraries, &library, input->library_index))) {
			if (input->auto_ogg)
				err = wsp_meta_convert_wems(&meta, buffer, state, input, library, goutput_root);
			else
				err = wsp_meta_extract_wems(&meta, buffer, state, input, goutput_root);
		}
		wsp_meta_clear(&meta);
	}
	free(buffer);
	return err;
}

int
neextract_wsp(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input)
{
	int err = 0;
	state->wsps_to_process++;
	gbrrlog_level_last = gbrrlog_level_normal;
	if (input->dry_run) {
		LOG_FORMAT(LOG_PARAMS_DRY, "Extract WSP (dry) ");
	} else {
		LOG_FORMAT(LOG_PARAMS_WET, "Extracting WSP... ");
		lib_replace_ext(input->path, strlen(input->path) - 1, goutput_root, NULL, "");
		err = i_extract_wsp(state, libraries, input);
	}
	gbrrlog_level_last = gbrrlog_level_normal;
	if (!err) {
		state->wsps_processed++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, "Success!\n");
	} else {
		state->wsps_failed++;
		LOG_FORMAT(LOG_PARAMS_FAILURE, "Failure! (%d)\n", err);
	}
	return err;
}
