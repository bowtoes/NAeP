/*
Copyright 2021-2022 BowToes (bow.toes@mailfence.com)

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

#include "lib.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <vorbis/vorbisenc.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "errors.h"
#include "print.h"

const lib_cmp_t lib_cmp = strcmp;
const lib_ncmp_t lib_ncmp = strncmp;
#if defined(BRRPLATFORMTYPE_WINDOWS)
const lib_cmp_t  lib_case_cmp = _stricmp;
const lib_ncmp_t lib_case_ncmp = _strnicmp;
#else
const lib_cmp_t  lib_case_cmp = strcasecmp;
const lib_ncmp_t lib_case_ncmp = strncasecmp;
#endif

static char s_strerr[256] = "";
#define s_max_strerr sizeof(s_strerr)

const char *
lib_strerr(int err)
{
	switch (err) {
		case I_SUCCESS           : return "Success";
		case I_BUFFER_ERROR      : return "Buffer/memory error";
		case I_IO_ERROR          : return "File I/O error";
		case I_FILE_TRUNCATED    : return "File truncated";
		case I_INIT_ERROR        : return "Initialization error";
		case I_NOT_VORBIS        : return "Data is not Vorbis";
		case I_DESYNC            : return "Desync while decoding stream";
		case I_CORRUPT           : return "Corrupted data";
		case I_NOT_RIFF          : return "Data is not RIFF";
		case I_UNRECOGNIZED_DATA : return "Data type is unrecognized";
		case I_INSUFFICIENT_DATA : return "Insufficient data to decode";
		case I_BAD_ERROR         : return "I don't know what to do";
	}
	snprintf(s_strerr, s_max_strerr, "Unrecognized error code %d", err);
	return s_strerr;
}

int
lib_read_entire_file(const char *const path, void **const buffer, brrsz *const buffer_size)
{
	if (!path || !buffer || !buffer_size)
		return I_GENERIC_ERROR;

	int err = 0;
	void *buff = NULL;
	brrsz size = 0;
	{
		const brrstringr_t path_str = brrstringr_cast(path);
		brrpath_stat_result_t st;
		if ((err = brrpath_stat(&st, &path_str)))
			return I_IO_ERROR;
		if (!st.exists || st.type != brrpath_type_file)
			return I_IO_ERROR;
		size = st.size;
		if (brrlib_alloc(&buff, size, 0))
			return I_BUFFER_ERROR;
	}

	{
		FILE *file;
		if (!(file = fopen(path, "rb")))
			return I_IO_ERROR;
		if (size > fread(buff, 1, size, file)) {
			err = feof(file) ? I_FILE_TRUNCATED : I_IO_ERROR;
			fclose(file);
			return err;
		}
	}

	*buffer = buff;
	*buffer_size = size;
	return err;
}

static inline int
i_consume_next_buffer_chunk(riff_t *const riff, riff_chunkstate_t *const chunkstate, riff_datasync_t *const datasync)
{
	int err = 0;
	while (RIFF_CHUNK_CONSUMED != (err = riff_consume_chunk(riff, chunkstate, datasync))) {
		if (err == RIFF_CONSUME_MORE) {
			continue;
		} else if (err == RIFF_CHUNK_UNRECOGNIZED) {
			riff_datasync_seek(datasync, 1);
			continue;
		} else if (err != RIFF_CHUNK_INCOMPLETE) {
			if (err == RIFF_ERROR)
				return I_BUFFER_ERROR;
			else if (err == RIFF_NOT_RIFF)
				return I_NOT_RIFF;
			else if (err == RIFF_CORRUPTED)
				return I_CORRUPT;
			else
				return I_BAD_ERROR - err;
		} else { /* RIFF_CHUNK_INCOMPLETE */
			return I_INSUFFICIENT_DATA;
		}
	}
	return I_SUCCESS;
}

int
lib_parse_buffer_as_riff(riff_t *const riff, const void *const buffer, brrsz buffer_size)
{
	int err;
	riff_datasync_t sync_data = {0};
	if ((err = riff_datasync_from_buffer(&sync_data, (void *)buffer, buffer_size))) {
		return I_INIT_ERROR;
	}

	riff_chunkstate_t sync_chunk = {0};
	while (I_SUCCESS == (err = i_consume_next_buffer_chunk(riff, &sync_chunk, &sync_data))) {
		NeExtraPrint(DEB, "Found chunk %s", FCC_GET_CODE(sync_chunk.chunkcc));
		riff_chunkstate_clear(&sync_chunk);
	}

	if (err == I_INSUFFICIENT_DATA)
		return I_SUCCESS;

	return err;
}

int
lib_write_ogg_out(ogg_stream_state *const streamer, const char *const destination)
{
	FILE *out = NULL;
	ogg_page pager;
	if (!(out = fopen(destination, "wb"))) {
		BRRLOG_ERRN("Failed to open output ogg file '%s' : %s", destination, strerror(errno));
		return I_IO_ERROR;
	}
	while (ogg_stream_pageout(streamer, &pager) || ogg_stream_flush(streamer, &pager)) {
		if (pager.header_len != fwrite(pager.header, 1, pager.header_len, out)) {
			fclose(out);
			BRRLOG_ERRN("Failed to write ogg page header to output '%s' : %s", destination, strerror(errno));
			return I_IO_ERROR;
		} else if (pager.body_len != fwrite(pager.body, 1, pager.body_len, out)) {
			fclose(out);
			BRRLOG_ERRN("Failed to write ogg page body to output '%s' : %s", destination, strerror(errno));
			return I_IO_ERROR;
		}
	}
	fclose(out);
	return I_SUCCESS;
}

/* -1 : Not found */
static inline int
i_find_ext(const char *const arg, int arglen)
{
	int dot = arglen;
	/* Search backwards because that's where the extensions are. */
	for (int i = arglen; i > 0; --i) {
		char c = arg[i - 1];
		if (c == '.')
			dot = i - 1;
		else if (c == BRRPATH_SEP_CHR)
			break;
	}
	if (dot == arglen) /* No dot found */
		return -1;
	return dot;
}

int
lib_cmp_ext(const char *const arg, int arglen, int case_sensitive, ...)
{
	char ext[BRRPATH_MAX_NAME + 1] = {0};
	{
		int idx = 0;
		if (-1 == (idx = i_find_ext(arg, arglen)))
			return -1;
		snprintf(ext, BRRPATH_MAX_NAME + 1, "%s", arg + idx);
	}

	brrbl did_match = 0;
	int idx = 0;
	{
		lib_cmp_t cmp = case_sensitive ? lib_cmp : lib_case_cmp;
		va_list lptr;
		va_start(lptr, case_sensitive);
		const char *a;
		while ((a = va_arg(lptr, const char *))) {
			if (0 == cmp(arg, a)) {
				did_match = 1;
				break;
			}
			idx++;
		}
		va_end(lptr);
	}
	return did_match ? idx : -1;
}

int
lib_replace_ext(const char *const input, brrsz inlen, char *const output, brrsz *const outlen, const char *const replacement)
{
	brrsz olen, nlen = 0;
	if (!input || !output)
		return 0;
	nlen = i_find_ext(input, inlen);
	olen = snprintf(output, BRRPATH_MAX_PATH + 1, "%*.*s%s", (int)nlen, (int)nlen, input, replacement);
	if (outlen)
		*outlen = olen;
	return 0;
}

int
lib_cstr_compare(const char *const a, const char *const b, brrsz max_length, int case_sensitive)
{
	if (!max_length && case_sensitive) {
		return lib_cmp(a, b);
	} else if (max_length && case_sensitive) {
		return lib_ncmp(a, b, max_length);
	} else if (!max_length && !case_sensitive) {
		return lib_case_cmp(a, b);
	} else {
		return lib_case_ncmp(a, b, max_length);
	}
}
