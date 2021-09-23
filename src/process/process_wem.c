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
#include <string.h>

#include <ogg/ogg.h>

#include <brrtools/brrpath.h>

#include "common_lib.h"
#include "errors.h"
#include "print.h"

int BRRCALL
convert_wem(nestateT *const state, neinput_libraryT *const libraries, const neinputT *const input)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	state->wems_to_convert++;
	if (input->dry_run) {
		LOG_FORMAT(LOG_PARAMS_DRY, "Convert WEM (dry) ");
	} else {
		LOG_FORMAT(LOG_PARAMS_WET, "Converting WEM... ");
		if (input->inplace_ogg) {
			snprintf(output, sizeof(output), "%s", input->path);
		} else {
			lib_replace_ext(input->path, strlen(input->path), output, NULL, ".ogg");
		}
		err = I_BAD_ERROR;
	}
	if (!err) {
		state->wems_converted++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, "Success!");
	} else {
		state->wems_failed++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, " Failure! (%d)", err);
	}
	return err;
}
