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

#include "common_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "common_lib.h"
#include "errors.h"

static int BRRCALL
i_try_load_lib(input_libraryT *const library, const unsigned char *const buffer, brrsz buff_size)
{
	int err = 0;
	if (library->old)
		err = codebook_library_deserialize_deprecated(&library->library, buffer, buff_size);
	else
		err = codebook_library_deserialize(&library->library, buffer, buff_size);
	if (err == CODEBOOK_CORRUPT) {
		codebook_library_clear(&library->library);
		library->old = !library->old;
		if (library->old)
			err = codebook_library_deserialize_deprecated(&library->library, buffer, buff_size);
		else
			err = codebook_library_deserialize(&library->library, buffer, buff_size);
	}
	if (err == CODEBOOK_CORRUPT) {
		codebook_library_clear(&library->library);
		err = I_UNRECOGNIZED_DATA;
	} else if (err == CODEBOOK_ERROR) {
		err = I_BUFFER_ERROR;
	}
	return err;
}
static int BRRCALL
i_try_read_lib(input_libraryT *const library, unsigned char **const buffer, brrsz *file_size)
{
	brrpath_stat_resultT rs;
	int err = 0;
	FILE *file = NULL;
	if ((err = brrpath_stat(&rs, library->library_path.opaque))) {
		return I_IO_ERROR;
	} else if (!rs.exists || rs.type != brrpath_type_file) {
		return I_IO_ERROR;
	} else if ((brrlib_alloc((void **)buffer, rs.size, 1))) {
		return I_BUFFER_ERROR;
	} else if (!(file = fopen(library->library_path.opaque, "rb"))) {
		free(*buffer);
		*buffer = NULL;
		return I_IO_ERROR;
	} else if (rs.size > fread(*buffer, 1, rs.size, file)) {
		err = feof(file)?I_FILE_TRUNCATED:I_IO_ERROR;
		free(*buffer);
		*buffer = NULL;
	}
	fclose(file);
	if (!err)
		*file_size = rs.size;
	return err;
}
int BRRCALL
input_library_load(input_libraryT *const library)
{
	unsigned char *buffer = NULL;
	brrsz bufsize = 0;
	if (library->loaded) {
		return 0;
	} else if (library->load_error) {
		return library->load_error;
	} else if (!(library->load_error = i_try_read_lib(library, &buffer, &bufsize))) {
		library->load_error = i_try_load_lib(library, buffer, bufsize);
		free(buffer);
	}
	library->loaded = !library->load_error;
	return library->load_error;
}
void BRRCALL
input_library_clear(input_libraryT *const library)
{
	if (library) {
		codebook_library_clear(&library->library);
		brrstg_delete(&library->library_path);
		memset(library, 0, sizeof(*library));
	}
}

void BRRCALL
input_clear(inputT *const input)
{
	if (input) {
		brrstg_delete(&input->path);
		memset(input, 0, sizeof(*input));
	}
}
void BRRCALL
input_print(const inputT *const input, brrsz max_input_length,
    brrlog_priorityT priority, int newline)
{
	gbrrlog_level_last = gbrrlog_level_debug;
	gbrrlog_format_last = gbrrlog_format_normal;
	BRRLOG_LASTNP(" ");
	if (input->options.type == input_type_ogg)      { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_OGG, "OGG"); }
	else if (input->options.type == input_type_wem) { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_WEM, "WEM"); }
	else if (input->options.type == input_type_wsp) { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_WSP, "WSP"); }
	else if (input->options.type == input_type_bnk) { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_BNK, "BNK"); }
	else                                            { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_AUT, "AUT"); }

	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " cbl ");
	BRRLOG_STYLENP(LOG_COLOR_ENABLED, -1, brrlog_style_bold, "%zu", input->options.library_index);
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " log ");
	if (input->options.log_enabled) { BRRLOG_STYLENP(LOG_COLOR_ENABLED, -1, brrlog_style_bold, "ENB"); }
	else { BRRLOG_FORENP(LOG_COLOR_DISABLED, "DSB"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_color_enabled) { BRRLOG_STYLENP(LOG_COLOR_ENABLED, -1, brrlog_style_bold, "STY"); }
	else { BRRLOG_FORENP(LOG_COLOR_DISABLED, "SMP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_debug) { BRRLOG_FORENP(brrlog_color_cyan, "DBG"); }
	else { BRRLOG_FORENP(brrlog_color_yellow, "NRM"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	BRRLOG_FORENP(brrlog_color_normal + 1 + input->options.log_priority,
	    "%s", brrlog_priority_dbgstr(input->options.log_priority));
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " operation ");
	if (input->options.dry_run) { BRRLOG_FORENP(LOG_COLOR_DRY, "DRY"); }
	else { BRRLOG_FORENP(LOG_COLOR_WET, "WET"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " w2o ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(LOG_COLOR_AUT, "AUT"); }
	else { BRRLOG_FORENP(LOG_COLOR_MANUAL, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(LOG_COLOR_INPLACE, "INP"); }
	else { BRRLOG_FORENP(LOG_COLOR_SEPARATE, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " rvb ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(LOG_COLOR_AUT, "AUT"); }
	else { BRRLOG_FORENP(LOG_COLOR_MANUAL, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(LOG_COLOR_INPLACE, "INP"); }
	else { BRRLOG_FORENP(LOG_COLOR_MANUAL, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " bank recurse ");
	if (input->options.bank_recurse) { BRRLOG_FORENP(LOG_COLOR_ENABLED, "FUL"); }
	else { BRRLOG_FORENP(LOG_COLOR_DISABLED, "NON"); }
	if (newline)
		BRRLOG_LASTP("");
}

