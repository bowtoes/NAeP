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
i_extract_bnk(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input)
{
	int err = 0;
	wsp_metaT meta = {0};
	const codebook_libraryT *library = NULL;
	char *buffer = NULL;
	brrsz bufsize = 0;

	if ((err = lib_read_entire_file(input->path, (void **)&buffer, &bufsize)))
		return err;
	if (!(err = wsp_meta_init(&meta, buffer, bufsize)))
		err = neinput_load_index(libraries, &library, input->library_index);
	if (err) {
		free(buffer);
		return err;
	}
	if (input->auto_ogg)
		err = wsp_meta_convert_wems(&meta, buffer, state, input, library, goutput_root);
	else
		err = wsp_meta_extract_wems(&meta, buffer, state, input, goutput_root);
	free(buffer);
	return err;
}

int BRRCALL
neextract_bnk(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input)
{
	int err = 0;
	state->bnks_to_process++;
	if (input->dry_run) {
		BRRLOG_FORENP(LOG_COLOR_DRY, "Extract BNK (dry) ");
	} else {
		codebook_libraryT *cbl = NULL;
		BRRLOG_FORENP(LOG_COLOR_WET, "Extracting WSP...");
		lib_replace_ext(input->path, strlen(input->path) - 1, goutput_root, NULL, "");
		err = i_extract_bnk(state, libraries, input);
	}
	if (!err) {
		state->bnks_processed++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, "Success!\n");
	} else {
		state->bnks_failed++;
		LOG_FORMAT(LOG_PARAMS_FAILURE, "Failure! (%d)\n", err);
	}
	return err;
}
