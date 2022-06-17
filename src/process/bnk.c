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

#include "neinput.h"
#include "nelog.h"
#include "rifflist.h"

static char s_output_root[BRRPATH_MAX_PATH + 1] = {0};

static int
i_extract_bnk(nestate_t *const state, const neinput_t *const input)
{
	int err = 0;
	rifflist_t meta = {0};
	const codebook_library_t *library = NULL;
	unsigned char *buffer = NULL;

	if ((err = nepath_read(&input->path, (void **)&buffer)))
		return err;

	if (!(err = rifflist_scan(&meta, buffer, input->path.st.size))) {
		if (!(err = neinput_load_codebooks(state->libraries, &library, input->library_index))) {
			if (input->flag.auto_ogg)
				err = rifflist_convert(&meta, buffer, state, input, library, s_output_root);
			else
				err = rifflist_extract(&meta, buffer, state, input, s_output_root);
		}
		rifflist_clear(&meta);
	}
	free(buffer);
	return err;
}

int
neprocess_bnk(nestate_t *const state, const neinput_t *const input)
{
	int err = 0;
	state->stats.bnks.assigned++;
	if (input->flag.dry_run) {
		SNor(p,meta_dry, "Extract BNK (dry) ");
	} else {
		SNor(p,meta_wet, "Extracting BNK... ");
		nepath_extension_replace(&input->path, NULL, 0, s_output_root);
		err = i_extract_bnk(state, input);
	}
	if (!err) {
		state->stats.bnks.succeeded++;
		SNor(,meta_success, "Success!");
	} else {
		state->stats.bnks.failed++;
		SNor(,meta_failure, "Failure! (%d)", err);
	}
	return err;
}
