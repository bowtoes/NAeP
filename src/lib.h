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

#ifndef LIB_H
#define LIB_H

#include <ogg/ogg.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

#include "riff.h"
#include "wwise.h"

typedef union fourcc {
	struct {
		brru1 _0;
		brru1 _1;
		brru1 _2;
		brru1 _3;
	} bytes;
	brru4 integer;
} fourcc_t;

#define _fcc_full(_a_, _b_, _c_, _d_) {.bytes={(_a_), (_b_), (_c_), (_d_)}}
#define _fcc_lit(_l_) _fcc_full((_l_)[0], (_l_)[1], (_l_)[2], (_l_)[3])
#define FCC_INIT(_l_) _fcc_lit(_l_)
#define FCC_TO_CODE(_f_) ((char[5]){(_f_).bytes._0, (_f_).bytes._1, (_f_).bytes._2, (_f_).bytes._3, 0})
/* Is this wrong on big-endian systems? */
#define FCC_GET_INT(_l_) (brru4)(\
	(brru4)((_l_)[0] <<  0) | \
	(brru4)((_l_)[1] <<  8) | \
	(brru4)((_l_)[2] << 16) | \
	(brru4)((_l_)[3] << 24)   \
)
#define FCC_GET_CODE(_i_) ((char[5]){\
	((char*)(&(_i_)))[0], \
	((char*)(&(_i_)))[1], \
	((char*)(&(_i_)))[2], \
	((char*)(&(_i_)))[3], \
	0, \
})

const char *lib_strerr(int err);

/* Counts number of set bits in number */
int lib_count_ones(unsigned long number);
/* Counts number of bits needed to store number (log base 2) */
int lib_count_bits(long number);
/* Ripped from tremor, don't understand what it does. */
long lib_lookup1_values(long entries, long dimensions);

typedef int (* lib_cmp_t)(const char *const, const char *const);
typedef int (* lib_ncmp_t)(const char *const, const char *const, size_t);
extern const lib_cmp_t lib_cmp;
extern const lib_cmp_t lib_case_cmp;
extern const lib_ncmp_t lib_ncmp;
extern const lib_ncmp_t lib_case_ncmp;

/* TODO for this and most other functions that can support it, errors should be logged as soon as possible,
 * and ideally there will only be one error code (-1). */
/* Reads the entire file 'path' into 'buffer', allocating as necessary, and returns the file size in 'buffer_size'.
 * Returns 0 on success.
 * If 'path' or 'buffer' or 'buffer_size' is NULL, nothing is done and -1 is returned.
 * If an error occurs, 'buffer' and 'buffer_size' are unaffacted and an appropriate error code is returned.
 * */
int lib_read_entire_file(const char *const path, void **const buffer, brrsz *const buffer_size);

int lib_parse_buffer_as_riff(riff_t *const rf, const void *const buffer, brrsz buffer_size);

int lib_parse_buffer_as_wwriff(wwriff_t *const rf, const void *const buffer, brrsz buffer_size);

/* Writes the ogg stream 'stream' to the file 'destination'.
 * Returns 0 on success, or I_IO_ERROR on failure.
 * */
int lib_write_ogg_out(ogg_stream_state *const streamer, const char *const destination);

/* Returns the index of the first extension that matches the last extension of 'arg' (everything after the dot),
 * or -1 if no extension matches.
 */
int lib_cmp_ext(const char *const arg, int arglen, int case_sensitive, ...);

/* Replaces everything at and after the last dot */
int lib_replace_ext(const char *const input, brrsz inlen, char *const output, brrsz *const outlen, const char *const replacement);

int lib_cstr_compare(const char *const a, const char *const b, brrsz max_length, int case_sensitive);

#endif /* LIB_H */
