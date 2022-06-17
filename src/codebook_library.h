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

#ifndef NAeP_codebook_library_h
#define NAeP_codebook_library_h

#include <ogg/ogg.h>

#include <brrtools/brrtypes.h>

#define CODEBOOK_ERROR (-1)
#define CODEBOOK_CORRUPT (-2)

typedef struct packed_codebook {
	unsigned char *data;
	unsigned char *unpacked_data;
	brru8 unpacked_bits;
	brru4 size;
	int did_unpack;
} packed_codebook_t;

/* -2 : decode error/corrupt data
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int
packed_codebook_unpack(packed_codebook_t *const pc);
/* -2 : decode error/corrupt data
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int
packed_codebook_unpack_raw(oggpack_buffer *const unpacker, oggpack_buffer *const packer);

void
packed_codebook_clear(packed_codebook_t *const pc);

typedef struct codebook_library {
	packed_codebook_t *codebooks;
	brru4 codebook_count;
} codebook_library_t;

void
codebook_library_clear(codebook_library_t *const cb);

/* -1 : Corrupt/bad input
 *  0 : Regular codebook
 *  1 : Alternate codebook
 * */
int
codebook_library_check_type(const void *const data, brru8 data_size);

/* Codebooks first, offsets last.
 *  0 : success
 * -1 : error (allocation/argument)
 * -2 : corruption
 * */
int
codebook_library_deserialize_alt(codebook_library_t *const library, const void *const input_data, brru8 data_size);

/* Offsets first, codebooks last.
 *  0 : success
 * -1 : error (allocation/argument)
 * -2 : corruption
 * */
int
codebook_library_deserialize(codebook_library_t *const library, const void *const input_data, brru8 data_size);

/* Codebooks first, offsets last.
 *  0 : success
 * -1 : error (allocation/argument)
 * */
int
codebook_library_serialize_alt(const codebook_library_t *const library, void **const output_data, brru8 *const data_size);

/* Offsets first, codebooks last.
 *  0 : success
 * -1 : error (allocation/argument)
 * */
int
codebook_library_serialize(const codebook_library_t *const library, void **const output_data, brru8 *const data_size);

#endif /* NAeP_codebook_library_h */
