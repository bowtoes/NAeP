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

#include "common_lib.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <brrtools/brrlog.h>
#include <brrtools/brrmem.h>
#include <brrtools/brrpath.h>

#include "errors.h"

static char i_strerr_str[256] = "";
#define i_strerr_max sizeof(i_strerr_str)
const char *BRRCALL
lib_strerr(int err)
{
	switch (err) {
		case I_SUCCESS: return "Success";
		case I_BUFFER_ERROR: return "Buffer/memory error";
		case I_IO_ERROR: return "File I/O error";
		case I_FILE_TRUNCATED: return "File truncated";
		case I_INIT_ERROR: return "Initialization error";
		case I_NOT_VORBIS: return "Data is not Vorbis";
		case I_DESYNC: return "Desync while decoding stream";
		case I_CORRUPT: return "Corrupted data";
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

int BRRCALL
lib_count_set(unsigned long v)
{
	int r = 0;
	while (v) {
		r += v&1;
		v>>=1;
	}
	return r;
}
int BRRCALL
lib_count_bits(long number)
{
	int res = 0;
	while (number > 0) {
		res++;
		number >>= 1;
	}
	return res;
}
long BRRCALL
lib_lookup1_values(long entries, long dimensions)
{
  /* totally ripped from tremor */
  int bits=lib_count_bits(entries);
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
lib_write_ogg_out(ogg_stream_state *const streamer, const char *const destination)
{
	FILE *out = NULL;
	ogg_page pager;
	if (!(out = fopen(destination, "wb"))) {
		BRRLOG_ERRN("Failed to open wem conversion output '%s' : %s", destination, strerror(errno));
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
static int BRRCALL
i_consume_next_chunk(FILE *const file, riffT *const rf, riff_chunk_infoT *const sc, riff_data_syncT *const ds)
{
	#define RIFF_BUFFER_INCREMENT 4096
	int err = 0;
	char *buffer = NULL;
	brrsz increment = RIFF_BUFFER_INCREMENT;
	brrsz bytes_read = 0;
	while (RIFF_CHUNK_CONSUMED != (err = riff_consume_chunk(rf, sc, ds))) {
		if (err == RIFF_CONSUME_MORE) {
			continue;
		} else if (err == RIFF_CHUNK_UNRECOGNIZED) {
			riff_data_sync_seek(ds, 1);
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
		} else if (feof(file)) {
			if (sc->chunksize)
				return I_FILE_TRUNCATED;
			break;
		} else {
			increment = sc->chunksize?sc->chunksize:RIFF_BUFFER_INCREMENT;
			if (!(buffer = riff_data_sync_buffer(ds, increment))) {
				return I_BUFFER_ERROR;
			}
		}
		bytes_read = fread(buffer, 1, increment, file);
		if (ferror(file)) {
			return I_IO_ERROR;
		} else if (RIFF_BUFFER_APPLY_SUCCESS != riff_data_sync_apply(ds, bytes_read)) {
			return I_BUFFER_ERROR;
		}
	}
	return I_SUCCESS;
	#undef RIFF_BUFFER_INCREMENT
}
static int BRRCALL
i_consume_next_buffer_chunk(riffT *const rf, riff_chunk_infoT *const sc, riff_data_syncT *const ds)
{
	int err = 0;
	while (RIFF_CHUNK_CONSUMED != (err = riff_consume_chunk(rf, sc, ds))) {
		if (err == RIFF_CONSUME_MORE) {
			continue;
		} else if (err == RIFF_CHUNK_UNRECOGNIZED) {
			riff_data_sync_seek(ds, 1);
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
int BRRCALL
lib_read_riff_from_buffer(riffT *const rf, unsigned char *const buffer, brrsz buffer_size)
{
	int err = I_SUCCESS;
	riff_chunk_infoT sync_chunk = {0};
	riff_data_syncT sync_data = {0};
	if ((err = riff_data_sync_from_buffer(&sync_data, buffer, buffer_size))) {
		return I_INIT_ERROR;
	}
	while (I_SUCCESS == (err = i_consume_next_buffer_chunk(rf, &sync_chunk, &sync_data))) {
		riff_chunk_info_clear(&sync_chunk);
	}
	if (err == I_INSUFFICIENT_DATA)
		return I_SUCCESS;
	return err;
}
int BRRCALL
lib_read_riff_chunks(FILE *const file, riffT *const rf)
{
	int err = I_SUCCESS;
	riff_chunk_infoT sync_chunk = {0};
	riff_data_syncT sync_data = {0};
	while (I_SUCCESS == (err = i_consume_next_chunk(file, rf, &sync_chunk, &sync_data)) && (sync_chunk.is_basic || sync_chunk.is_list)) {
		riff_chunk_info_clear(&sync_chunk);
	}
	riff_data_sync_clear(&sync_data);
	return err;
}

int BRRCALL
lib_replace_ext(const char *const input, brrsz inlen,
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
	olen = snprintf(output, BRRPATH_MAX_PATH + 1, "%*.*s%s", (int)nlen, (int)nlen, input, replacement);
	if (outlen)
		*outlen = olen;
	return 0;
}
int BRRCALL
lib_cstr_compare(const char *const a, const char *const b,
    brrsz max_length, int case_sensitive)
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

