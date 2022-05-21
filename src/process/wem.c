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
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>

#include <brrtools/brrpath.h>

#include "lib.h"
#include "errors.h"
#include "wwise.h"
#include "print.h"

static char goutput_name[BRRPATH_MAX_PATH + 1] = {0};

static int
i_convert_wem(neinput_library_t *const libraries, const neinput_t *const input)
{
	int err = 0;
	ogg_stream_state streamer;
	riff_t rf = {0};
	char *buffer = NULL;
	brrsz bufsize = 0;
	const codebook_library_t *library = NULL; /* NULL library means inline library */

	if ((err = lib_read_entire_file(input->path, (void **)&buffer, &bufsize)))
		return err;
	err = lib_parse_buffer_as_riff(&rf, buffer, bufsize);
	free(buffer);
	if (err || (err = neinput_load_codebooks(libraries, &library, input->library_index))) {
		riff_clear(&rf);
		return err;
	}
	if (!(err = wwise_convert_wwriff(&rf, &streamer, library, input)))
		err = lib_write_ogg_out(&streamer, goutput_name);
	riff_clear(&rf);
	ogg_stream_clear(&streamer);
	return err;
}

int
neconvert_wem(nestate_t *const state, neinput_library_t *const libraries, const neinput_t *const input)
{
	int err = 0;
	state->stats.wems.assigned++;
	if (input->flag.dry_run) {
		LOG_FORMAT(LOG_PARAMS_DRY, "Convert WEM (dry) ");
	} else {
		LOG_FORMAT(LOG_PARAMS_WET, "Converting WEM... ");
		if (input->flag.inplace_ogg)
			snprintf(goutput_name, sizeof(goutput_name), "%s", input->path);
		else
			lib_replace_ext(input->path, strlen(input->path), goutput_name, NULL, ".ogg");
		err = i_convert_wem(libraries, input);
	}
	if (!err) {
		state->stats.wems.succeeded++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, "Success!");
	} else {
		state->stats.wems.failed++;
		LOG_FORMAT(LOG_PARAMS_FAILURE, "Failure! (%d)", err);
	}
	return err;
}
