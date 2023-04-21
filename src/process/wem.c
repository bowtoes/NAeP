/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "process.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <brrtools/brrfile.h>

#include "neinput.h"
#include "nelog.h"
#include "neutil.h"
#include "wwise.h"

static char s_output_name[brrpath_max_path + 1] = {0};

static int
i_convert_wem(neinput_library_t *const libraries, const neinput_t *const input)
{
	int err = 0;
	wwriff_t wwriff = {0};
	{
		unsigned char *buffer = NULL;

		if (BRRSZ_MAX == (err = brrfile_read(input->path.full, &input->path.inf, (void **)&buffer))) {
			Err(,"%s", brrapi_error_message(nemessage, nemessage_len));
			return -1;
		}

		err = neutil_buffer_to_wwriff(&wwriff, buffer, brrpath_get_size(input->path.size));

		free(buffer);
		if (err)
			return err;
	}
	if (input->cfg.add_comments) {
		if ((err = wwriff_add_comment(&wwriff, "SourceFile=%s", input->path.full))) {
			Err(,"Failed to add comment to WWRIFF : %s (%d)", strerror(errno), errno);
		} else if ((err = wwriff_add_comment(&wwriff, "OutputFile=%s", s_output_name))) {
			Err(,"Failed to add comment to WWRIFF : %s (%d)", strerror(errno), errno);
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
	if (input->cfg.dry_run) {
		Nor(np, "(!" nest_meta_dry ":Convert WEM (dry)!) ");
	} else {
		Nor(np,"(!"nest_meta_wet":Converting WEM...!) ");
		if (input->cfg.inplace_ogg) {
			/* Overwrite input file */
			snprintf(s_output_name, sizeof(s_output_name), "%s", input->path.full);
		} else {
			/* Output to [file_path/base_name].ogg */
			neutil_replace_extension(&input->path, W2O_EXT, sizeof(W2O_EXT) - 1, 1, s_output_name, sizeof(s_output_name) - 1);
		}
		err = i_convert_wem(state->libraries, input);
	}
	if (!err) {
		state->stats.wems.succeeded++;
		Nor(p,"(!"nest_meta_success":Success!!)");
	} else {
		state->stats.wems.failed++;
		Nor(p,"(!"nest_meta_failure":Failure! (%d)!)", err);
	}
	return err;
}
