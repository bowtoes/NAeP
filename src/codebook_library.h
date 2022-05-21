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

#ifndef CODEBOOK_LIBRARY_H
#define CODEBOOK_LIBRARY_H

#include <ogg/ogg.h>

#include <brrtools/brrtypes.h>

#define CODEBOOK_SUCCESS 0
#define CODEBOOK_ERROR 1
#define CODEBOOK_CORRUPT 2

typedef struct packed_codebook {
	unsigned char *data;
	unsigned char *unpacked_data;
	brru8 unpacked_bits;
	brru4 size;
	int did_unpack;
} packed_codebook_t;
typedef struct codebook_library {
	packed_codebook_t *codebooks;
	brru4 codebook_count;
} codebook_library_t;

void packed_codebook_clear(packed_codebook_t *const pc);
void packed_codebook_clear_unpacked(packed_codebook_t *const pc);
/* -2 : decode error/corrupt data
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int packed_codebook_unpack_raw(oggpack_buffer *const unpacker, oggpack_buffer *const packer);
/* -2 : decode error/corrupt data
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int packed_codebook_unpack(packed_codebook_t *const pc);

void codebook_library_clear(codebook_library_t *const cb);
/* -2 : corruption
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int codebook_library_deserialize_old(codebook_library_t *const cb,
    const void *const data, brru8 data_size);
/* -2 : corruption
 * -1 : error (allocation/argument)
 *  0 : success
 * */
int codebook_library_deserialize(codebook_library_t *const cb,
    const void *const data, brru8 data_size);
/* -1 : error (allocation/argument)
 *  0 : success
 * */
int codebook_library_serialize_old(const codebook_library_t *const cb,
    void **const data, brru8 *const data_size);
/* -1 : error (allocation/argument)
 *  0 : success
 * */
int codebook_library_serialize(const codebook_library_t *const cb,
    void **const data, brru8 *const data_size);

#endif /* CODEBOOK_LIBRARY_H */
