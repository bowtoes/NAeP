/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "process.h"

#include <stdlib.h>
#include <string.h>
#include <brrtools/brrfile.h>

#include "neinput.h"
#include "nelog.h"
#include "neutil.h"
#include "rifflist.h"

static char s_output_root[brrpath_max_path + 1] = {0};

static int
i_extract_bnk(nestate_t *const state, const neinput_t *const input)
{
	int err = 0;
	rifflist_t meta = {0};
	const codebook_library_t *library = NULL;
	unsigned char *buffer = NULL;

	if (BRRSZ_MAX == (err = brrfile_read(input->path.full, &input->path.inf, (void **)&buffer))) {
		Err(,"%s", brrapi_error_message(nemessage, nemessage_len));
		return -1;
	}

	if (!(err = rifflist_scan(&meta, buffer, brrpath_get_size(input->path.size)))) {
		if (!(err = neinput_load_codebooks(state->libraries, &library, input->library_index))) {
			if (input->cfg.auto_ogg)
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
	if (input->cfg.dry_run) {
		Nor(p,"(!"nest_meta_dry":Extract BNK (dry)!) ");
	} else {
		Nor(p,"(!"nest_meta_wet":Extracting BNK...!) ");
		neutil_replace_extension(&input->path, NULL, 0, 1, s_output_root, sizeof(s_output_root) - 1);
		err = i_extract_bnk(state, input);
	}
	if (!err) {
		state->stats.bnks.succeeded++;
		Nor(,"(!"nest_meta_success":Success!!)");
	} else {
		state->stats.bnks.failed++;
		Nor(,"(!"nest_meta_failure":Failure! (%d)!)", err);
	}
	return err;
}
