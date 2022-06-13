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
#include <stdio.h>

#include "neinput.h"
#include "wwise.h"

static char s_output_name[BRRPATH_MAX_PATH + 1] = {0};

static int
i_convert_wem(neinput_library_t *const libraries, const neinput_t *const input)
{
	int err = 0;
	wwriff_t wwriff = {0};
	{
		unsigned char *buffer = NULL;

		if ((err = nepath_read(&input->path, (void **)&buffer)))
			return err;

		err = neutil_buffer_to_wwriff(&wwriff, buffer, input->path.st.size);
		free(buffer);
		if (err)
			return err;
	}
	if (input->flag.add_comments) {
		if ((err = wwriff_add_comment(&wwriff, "SourceFile=%s", input->path))) {
			BRRLOG_ERR("Failed to add comment to WWRIFF : %s (%d)", strerror(errno), errno);
		} else if ((err = wwriff_add_comment(&wwriff, "OutputFile=%s", s_output_name))) {
			BRRLOG_ERR("Failed to add comment to WWRIFF : %s (%d)", strerror(errno), errno);
		}
	}
	if (!err) {
		const codebook_library_t *library = NULL; /* NULL library means inline library */
		if (!(err = neinput_load_codebooks(libraries, &library, input->library_index))) {
			ogg_stream_state streamer;
			if (!(err = wwise_convert_wwriff(&wwriff, &streamer, library, input)))
				err = neutil_write_ogg(&streamer, s_output_name);
			ogg_stream_clear(&streamer);
		}
	}
	wwriff_clear(&wwriff);

	return err;
}

#define W2O_EXT ".ogg"

int
neprocess_wem(nestate_t *const state, const neinput_t *const input)
{
	int err = 0;
	state->stats.wems.assigned++;
	if (input->flag.dry_run) {
		Style(np,meta_dry, "Convert WEM (dry) ");
	} else {
		Style(np,meta_wet, "Converting WEM... ");
		if (input->flag.inplace_ogg) {
			/* Overwrite input file */
			snprintf(s_output_name, sizeof(s_output_name), "%s", input->path.cstr);
		} else {
			/* Output to [file_path/base_name].ogg */
			nepath_extension_replace(&input->path, W2O_EXT, sizeof(W2O_EXT) - 1, s_output_name);
		}
		err = i_convert_wem(state->libraries, input);
	}
	if (!err) {
		state->stats.wems.succeeded++;
		Style(p,meta_success, "Success!");
	} else {
		state->stats.wems.failed++;
		Style(p,meta_failure, "Failure! (%d)", err);
	}
	return err;
}
