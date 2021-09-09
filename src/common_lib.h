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

#ifndef COMMON_LIB_H
#define COMMON_LIB_H

#include <ogg/ogg.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

BRRCPPSTART

typedef union fourcc {
	struct {
		brru1 _0;
		brru1 _1;
		brru1 _2;
		brru1 _3;
	} bytes;
	brru4 integer;
} fourccT;

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

const char *BRRCALL lib_strerr(int err);

/* Counts number of set bits in number */
int BRRCALL lib_count_set(unsigned long number);
/* Counts number of bits needed to store number (log base 2) */
int BRRCALL lib_count_bits(long number);
/* definitely not ripped from tremor */
long BRRCALL lib_lookup1_values(long entries, long dimensions);

long BRRCALL lib_packer_transfer(oggpack_buffer *const unpacker, int in_bits,
    oggpack_buffer *const packer, int out_bits);
long BRRCALL lib_packer_transfer_remaining(oggpack_buffer *const unpacker, oggpack_buffer *const packer);
long BRRCALL lib_packer_write_lots(oggpack_buffer *const unpacker, oggpack_buffer *const packer, long data_bits);

int BRRCALL lib_replace_ext(const char *const input, brrsz inlen,
    char *const output, brrsz *const outlen,
	const char *const replacement);

int BRRCALL
lib_cstr_compare(const char *const a, const char *const b,
    brrsz max_length, int case_sensitive);

BRRCPPEND

#endif /* COMMON_LIB_H */
