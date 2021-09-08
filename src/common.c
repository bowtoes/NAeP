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

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrmem.h>
#include <brrtools/brrpath.h>
#include <brrtools/brrplatform.h>
#include <brrtools/brrstg.h>
#if defined(BRRPLATFORMTYPE_UNIX)
# include <strings.h>
#endif

const fourccT goggfcc = FCC_INIT("OggS");
const fourccT gwemfcc = FCC_INIT("RIFF");
const fourccT gbnkfcc = FCC_INIT("BKHD");

static char i_strerr_str[256] = "";
#define i_strerr_max sizeof(i_strerr_str)
const char *BRRCALL
i_strerr(int err)
{
	switch (err) {
		case I_SUCCESS: return "Success";
		case I_BUFFER_ERROR: return "Buffer/memory error";
		case I_IO_ERROR: return "File I/O error";
		case I_FILE_TRUNCATED: return "File truncated";
		case I_INIT_ERROR: return "Initialization error";
		case I_NOT_VORBIS: return "Data is not Vorbis";
		case I_DESYNC: return "Desync while decoding stream";
		case I_CORRUPT: return "Corrupted headers/stream";
		case I_NOT_RIFF: return "Data is not RIFF";
		case I_UNRECOGNIZED_DATA: return "Data type is unrecognized";
		case I_INSUFFICIENT_DATA: return "Insufficient data to decode";
		case I_BAD_ERROR:
			snprintf(i_strerr_str, i_strerr_max, "I don't know what to do %d", err);
			return i_strerr_str;
		default:
			snprintf(i_strerr_str, i_strerr_max, "Unrecognized error code %d", err);
			return i_strerr_str;
	}
}

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
	} else if (library->failed) {
		return library->failed;
	} else if (!(library->failed = i_try_read_lib(library, &buffer, &bufsize))) {
		library->failed = i_try_load_lib(library, buffer, bufsize);
		free(buffer);
	}
	library->loaded = !library->failed;
	return library->failed;
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
processed_input_clear(processed_inputT *const input)
{
	if (input) {
		brrstg_delete(&input->path);
		memset(input, 0, sizeof(*input));
	}
}
void BRRCALL
processed_input_print(const processed_inputT *const input, brrsz max_input_length,
    brrlog_priorityT priority, int newline)
{
	gbrrlog_level_last = gbrrlog_level_debug;
	gbrrlog_format_last = gbrrlog_format_normal;
	BRRLOG_LASTNP(" ");
	if (input->options.type == INPUT_TYPE_OGG)      { BRRLOG_MESSAGETNP(gbrrlog_level_last, NeLOG_FORMAT_OGG, "OGG"); }
	else if (input->options.type == INPUT_TYPE_WEM) { BRRLOG_MESSAGETNP(gbrrlog_level_last, NeLOG_FORMAT_WEM, "WEM"); }
	else if (input->options.type == INPUT_TYPE_WSP) { BRRLOG_MESSAGETNP(gbrrlog_level_last, NeLOG_FORMAT_WSP, "WSP"); }
	else if (input->options.type == INPUT_TYPE_BNK) { BRRLOG_MESSAGETNP(gbrrlog_level_last, NeLOG_FORMAT_BNK, "BNK"); }
	else                                            { BRRLOG_MESSAGETNP(gbrrlog_level_last, NeLOG_FORMAT_AUT, "AUT"); }

	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " cbl ");
	BRRLOG_STYLENP(NeLOG_COLOR_ENABLED, -1, brrlog_style_bold, "%zu", input->options.library_index);
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " log ");
	if (input->options.log_enabled) { BRRLOG_STYLENP(NeLOG_COLOR_ENABLED, -1, brrlog_style_bold, "ENB"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_DISABLED, "DSB"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_color_enabled) { BRRLOG_STYLENP(NeLOG_COLOR_ENABLED, -1, brrlog_style_bold, "STY"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_DISABLED, "SMP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_debug) { BRRLOG_FORENP(brrlog_color_cyan, "DBG"); }
	else { BRRLOG_FORENP(brrlog_color_yellow, "NRM"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	BRRLOG_FORENP(brrlog_color_normal + 1 + input->options.log_priority,
	    "%s", brrlog_priority_dbgstr(input->options.log_priority));
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " operation ");
	if (input->options.dry_run) { BRRLOG_FORENP(NeLOG_COLOR_DRY, "DRY"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_WET, "WET"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " w2o ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(NeLOG_COLOR_AUT, "AUT"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_MANUAL, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(NeLOG_COLOR_INPLACE, "INP"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_SEPARATE, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " rvb ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(NeLOG_COLOR_AUT, "AUT"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_MANUAL, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(NeLOG_COLOR_INPLACE, "INP"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_MANUAL, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " bank recurse ");
	if (input->options.bank_recurse) { BRRLOG_FORENP(NeLOG_COLOR_ENABLED, "FUL"); }
	else { BRRLOG_FORENP(NeLOG_COLOR_DISABLED, "NON"); }
	if (newline)
		BRRLOG_LASTP("");
}

int BRRCALL
icount(unsigned long v)
{
	int r = 0;
	while (v) {
		r += v&1;
		v>>=1;
	}
	return r;
}
int BRRCALL
ilog(long number)
{
	int res = 0;
	while (number > 0) {
		res++;
		number >>= 1;
	}
	return res;
}
long BRRCALL
lookup1_values(long entries, long dimensions)
{
  /* totally ripped from tremor */
  /* get us a starting hint, we'll polish it below */
  int bits=ilog(entries);
  int vals=entries>>((bits-1)*(dimensions-1)/dimensions);

  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<dimensions;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=entries && acc1>entries){
      return(vals);
    }else{
      if(acc>entries){
        vals--;
      }else{
        vals++;
      }
    }
  }
}

int BRRCALL
replace_ext(const char *const input, brrsz inlen,
    char *const output, brrsz *const outlen,
	const char *const replacement)
{
	brrsz dot = 0, sep = 0;
	brrsz olen, nlen = 0;
	if (!input || !output)
		return 0;
	for (sep = inlen; sep > 0 && input[sep] != BRRPATH_SEP_CHR; --sep);
	for (dot = inlen; dot > sep && input[dot] != '.'; --dot);
	if (dot > sep + 1)
		nlen = dot;
	olen = snprintf(output, BRRPATH_MAX_PATH + 1, "%*.*s%s", nlen, nlen, input, replacement);
	if (outlen)
		*outlen = olen;
	return 0;
}
int BRRCALL
cstr_compare(const char *const a, const char *const b, brrsz max_length, int case_sensitive)
{
	if (!max_length && case_sensitive) {
		return strcmp(a, b);
	} else if (max_length && case_sensitive) {
		return strncmp(a, b, max_length);
	} else if (!max_length && !case_sensitive) {
#if defined(BRRPLATFORMTYPE_WINDOWS)
		return _stricmp(a, b);
#else
		return strcasecmp(a, b);
#endif
	} else {
#if defined(BRRPLATFORMTYPE_WINDOWS)
		return _strnicmp(a, b, max_length);
#else
		return strncasecmp(a, b, max_length);
#endif
	}
}
