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

#include <brrtools/brrpath.h>
#include <brrtools/brrplatform.h>
#include <brrtools/brrstg.h>
#if defined(BRRPLATFORMTYPE_UNIX)
# include <strings.h>
#endif

const char *BRRCALL
i_strerr(int err)
{
	switch (err) {
		case I_SUCCESS: return "Success";
		case I_BUFFER_ERROR: return "Buffer/memory error";
		case I_IO_ERROR: return "File I/O error";
		case I_FILE_TRUNCATED: return "File truncated";
		case I_INIT_ERROR: return "Initialization error";
		case I_NOT_VORBIS: return "Stream is not vorbis";
		case I_DESYNC: return "Desync while decoding stream";
		case I_CORRUPT: return "Corrupted headers/stream";
		case I_BAD_ERROR: return "I don't know what to do";
		case I_NOT_RIFF: return "Stream is not RIFF";
		default: return "Unrecognized error code";
	}
}

int BRRCALL
replace_ext(const char *const input, brrsz *const inlen,
    char *const output, brrsz *const outlen, const char *const replacement)
{
	brrsz dot = 0, sep = 0;
	brrsz ilen, olen, nlen = 0;
	if (!input || !output)
		return 0;
	ilen = brrstg_strlen(input, BRRPATH_MAX_PATH);
	for (sep = ilen; sep > 0 && input[sep] != BRRPATH_SEP_CHR; --sep);
	for (dot = ilen; dot > sep && input[dot] != '.'; --dot);
	if (dot > sep + 1)
		nlen = dot;
	olen = snprintf(output, BRRPATH_MAX_PATH + 1, "%*.*s%s", (int)nlen, (int)nlen, input, replacement);
	if (inlen)
		*inlen = ilen;
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
brrsz BRRCALL
read_to_offset(void *const data, brrsz offset, brrsz to_read, FILE *const file)
{
	if (!data || !file || !to_read)
		return 0;
	return fread((char *)data + offset, 1, to_read, file);
}
brrsz BRRCALL
read_to_offsetr(void *const data, brrsz offset, brrsz to_read, FILE *const file)
{
	char *d = NULL;
	brrsz read = 0;
	if (!data || !file || !to_read)
		return 0;
	d = (char *)data;
	if (to_read != (read = fread(d + offset, 1, to_read, file)))
		return read;
	for (brrsz i = 0; i < offset / 2; ++i)
		d[i] = d[offset - i];
	return read;
}
