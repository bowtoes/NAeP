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

#include <brrtools/brrlib.h>

#include "common_lib.h"
#include "errors.h"
#include "print.h"
#include "wspbnk.h"

int BRRCALL
extract_wsp(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input)
{
	int err = 0;
	state->wsps_to_process++;
	if (input->dry_run) {
		BRRLOG_FORENP(LOG_COLOR_DRY, " Extract WSP (dry)");
	} else {
		codebook_libraryT *cbl = NULL;
		BRRLOG_FORENP(LOG_COLOR_WET, " Extracting WSP...");

		if (library) {
			if ((err = input_library_load(library))) {
				BRRLOG_ERRN("Failed to load codebook library '%s' : %s",
				    (char *)library->library_path.opaque, lib_strerr(err));
				return err;
			}
			cbl = &library->library;
		}
		err = wspbnk_extract(state, input, input_length, options, cbl);
	}
	if (!err) {
		state->wsps_processed++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_SUCCESS, "Success!");
	} else {
		state->wsps_failed++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_FAILURE, "Failure! (%d)", err);
	}
	return err;
}
