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

#include <stdio.h>
#include <string.h>
#include <strings.h>

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
long BRRCALL
lib_packer_transfer(oggpack_buffer *const unpacker, int in_bits,
    oggpack_buffer *const packer, int out_bits)
{
	long r = -1;
	r = oggpack_read(unpacker, in_bits);
	if (r == -1)
		return r;
	oggpack_write(packer, r, out_bits);
	return r;
}
long BRRCALL
lib_packer_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	long dwords = 0, left = 0, transferred = 0;
	if (unpacker->endbit) {
		int bits = 7 - unpacker->endbit;
		if (-1 == lib_packer_transfer(unpacker, bits, packer, bits))
			return -1;
		transferred += bits;
	}
	dwords = (unpacker->storage - unpacker->endbyte) / 4;
	for (long i = 0; i < dwords; ++i) {
		if (-1 == lib_packer_transfer(unpacker, 32, packer, 32))
			return -1;
		transferred += 32;
	}
	left = 8 * (unpacker->storage - unpacker->endbyte);
	if (left) {
		if (-1 == lib_packer_transfer(unpacker, left, packer, left))
			return -1;
		transferred += left;
	}
	return transferred;
}
long BRRCALL
lib_packer_write_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, long bits)
{
	long orig = bits;
	if (!packer)
		return -1;
	else if (!unpacker)
		return 0;
	while (bits > 32) {
		if (-1 == lib_packer_transfer(unpacker, 32, packer, 32))
			return -1;
		bits -= 32;
	}
	if (-1 == lib_packer_transfer(unpacker, bits, packer, bits))
		return -1;
	return orig;
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
