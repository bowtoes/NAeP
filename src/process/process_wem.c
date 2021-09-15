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

#include "process_files.h"

#include <errno.h>
#include <string.h>

#include <ogg/ogg.h>

#include "common_lib.h"

int BRRCALL
convert_wem(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, input_libraryT *const library)
{
	static char output[BRRPATH_MAX_PATH + 1] = {0};
	int err = 0;
	numbers->wems_to_convert++;
	if (options->dry_run) {
		BRRLOG_FORENP(LOG_COLOR_DRY, "Convert WEM (dry) ");
	} else {
		BRRLOG_FORENP(LOG_COLOR_WET, "Converting WEM... ");
		if (options->inplace_ogg) {
			snprintf(output, sizeof(output), "%s", input);
		} else {
			lib_replace_ext(input, input_length, output, NULL, ".ogg");
		}
	}
	if (!err) {
		numbers->wems_converted++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_SUCCESS, "Success!");
	} else {
		numbers->wems_failed++;
		BRRLOG_MESSAGETP(gbrrlog_level_normal, LOG_FORMAT_FAILURE, " Failure! (%d)", err);
	}
	return err;
}
