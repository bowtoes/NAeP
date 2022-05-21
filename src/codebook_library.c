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

#include "codebook_library.h"

#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>

#include "lib.h"
#include "packer.h"

/* TODO Same issue as elsewhere, I can't verify how big-endian systems will
 * work with this, or if any modification is necessary */

void
packed_codebook_clear(packed_codebook_t *const pc)
{
	if (pc) {
		if (pc->data)
			free(pc->data);
		if (pc->unpacked_data)
			free(pc->unpacked_data);
		memset(pc, 0, sizeof(*pc));
	}
}
void
packed_codebook_clear_unpacked(packed_codebook_t *const pc)
{
	if (pc) {
		if (pc->unpacked_data)
			free(pc->unpacked_data);
		pc->unpacked_data = 0;
		pc->unpacked_bits = 0;
		pc->did_unpack = 0;
	}
}
int
packed_codebook_unpack_raw(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	int dimensions, entries, ordered, lookup;

	if (!unpacker)
		return -1;

	packer_pack(packer, 'B', 8); /* OUT Sync */
	packer_pack(packer, 'C', 8); /* OUT Sync */
	packer_pack(packer, 'V', 8); /* OUT Sync */

	dimensions = packer_transfer(unpacker,  4, packer, 16); /* IN/OUT Dimensions */
	entries = packer_transfer(unpacker, 14, packer, 24);    /* IN/OUT Entries */
	ordered = packer_transfer(unpacker, 1, packer, 1);      /* IN/OUT Ordered flag */
	if (ordered) { /* Ordered codeword decode identical to spec */
		int current_length = 1 + packer_transfer(unpacker, 5, packer, 5); /* IN/OUT Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = lib_count_bits(entries - current_entry);
			long number = packer_transfer(unpacker, number_bits, packer, number_bits); /* IN/OUT Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries)
			return CODEBOOK_CORRUPT;
	} else {
		int codeword_length_bits, sparse;
		codeword_length_bits = packer_unpack(unpacker, 3);     /* IN Codeword length bits */
		if (codeword_length_bits < 0 || codeword_length_bits > 5)
			return CODEBOOK_CORRUPT;
		sparse = packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sparse flag */
		if (!sparse) { /* IN/OUT Nonsparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int length = packer_transfer(unpacker, codeword_length_bits, packer, 5);
			}
		} else { /* IN/OUT Sparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int used = packer_transfer(unpacker, 1, packer, 1); /* IN/OUT Used flag */
				if (used) {
					int length = packer_transfer(unpacker, codeword_length_bits, packer, 5); /* IN/OUT Codeword length */
				}
			}
		}
	}

	lookup = packer_transfer(unpacker, 1, packer, 4); /* IN/OUT Lookup type */
	if (lookup == 1) { /* Lookup 1 decode identical to spec */
		long minval_packed = packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Minimum value */
		long delval_packed = packer_transfer(unpacker, 32, packer, 32); /* IN/OUT Delta value */
		int value_bits = 1 + packer_transfer(unpacker, 4, packer, 4);   /* IN/OUT Value bits */
		int sequence_flag  = packer_transfer(unpacker, 1, packer, 1);   /* IN/OUT Sequence flag */

		long lookup_values = lib_lookup1_values(entries, dimensions);

		for (long i = 0; i < lookup_values; ++i) { /* IN/OUT Codebook multiplicands */
			long multiplicand = packer_transfer(unpacker, value_bits, packer, value_bits);
		}
	} else if (lookup) {
		BRRLOG_ERR("LOOKUP FAILED");
		return CODEBOOK_CORRUPT;
	}

	return CODEBOOK_SUCCESS;
}
int
packed_codebook_unpack(packed_codebook_t *const pc)
{
	int err = CODEBOOK_SUCCESS;
	oggpack_buffer unpacker, packer;
	if (!pc)
		return CODEBOOK_ERROR;
	else if (pc->did_unpack)
		return CODEBOOK_SUCCESS;
	else if (pc->unpacked_data)
		return CODEBOOK_CORRUPT;

	oggpack_readinit(&unpacker, pc->data, pc->size);
	oggpack_writeinit(&packer);

	if ((err = packed_codebook_unpack_raw(&unpacker, &packer))) {
		oggpack_writeclear(&packer);
	} else {
#if defined(NeWEMDEBUG)
		BRRLOG_DEBUG("%sRead %4lld == %3lld of %lld",
		    (oggpack_bits(unpacker)/8)+1!=pc->size?"!! ":"   ",
		    oggpack_bits(unpacker), oggpack_bytes(unpacker), pc->size);
#endif
		pc->unpacked_data = packer.buffer;
		pc->unpacked_bits = oggpack_bits(&packer);
		pc->did_unpack = 1;
	}
	return err;
}

int
codebook_library_deserialize_old(codebook_library_t *const cb,
    const void *const data, brru8 data_size)
{
	const unsigned char *dt = data;
	const brru4 *offset_table = NULL;
	brru4 count = 0, last_end = 0;
	if (!data || !cb || data_size < 4)
		return CODEBOOK_ERROR;
	last_end = *(brru4 *)(dt + data_size - 4);
	if (last_end > data_size - 4)
		return CODEBOOK_CORRUPT;
	count = (data_size - last_end) / 4;
	offset_table = (brru4 *)(dt + last_end);
	if (brrlib_alloc((void **)&cb->codebooks, count * sizeof(*cb->codebooks), 1)) {
		codebook_library_clear(cb);
		return CODEBOOK_ERROR;
	}
	for (brru4 i = 0, start = 0; i < count; ++i, ++cb->codebook_count) {
		brru4 end = offset_table[i];
		packed_codebook_t *pc = &cb->codebooks[i];

		if (end < start || end > data_size)
			return CODEBOOK_CORRUPT;
		pc->size = end - start;
		if (brrlib_alloc((void **)&pc->data, pc->size, 1)) {
			codebook_library_clear(cb);
			return CODEBOOK_ERROR;
		}
		memcpy(pc->data, dt + start, pc->size);

		start = end;
	}
	return CODEBOOK_SUCCESS;
}
int
codebook_library_deserialize(codebook_library_t *const cb,
    const void *const data, brru8 data_size)
{
	const unsigned char *dt = data;
	const brru4 *offset_table = data;
	brru4 count = 0;
	if (!cb || !data || data_size < 4)
		return CODEBOOK_ERROR;
	count = offset_table[0] / 4;
	if (offset_table[0] > data_size || count * 4 > data_size)
		return CODEBOOK_CORRUPT;
	if (brrlib_alloc((void **)&cb->codebooks, count * sizeof(*cb->codebooks), 1)) {
		codebook_library_clear(cb);
		return CODEBOOK_ERROR;
	}
	cb->codebook_count = count;
	for (brru4 i = count, end = data_size; i > 0; --i) {
		brru4 start = offset_table[i - 1];
		packed_codebook_t *pc = &cb->codebooks[i - 1];
		if (end < start || start > data_size)
			return CODEBOOK_CORRUPT;
		pc->size = end - start;
		if (brrlib_alloc((void **)&pc->data, pc->size, 1)) {
			codebook_library_clear(cb);
			return CODEBOOK_ERROR;
		}
		memcpy(pc->data, dt + start, pc->size);
		end = start;
	}
	return CODEBOOK_SUCCESS;
}
int
codebook_library_serialize_old(const codebook_library_t *const cb,
    void **const data, brru8 *const data_size)
{
	brru4 *offset_table = NULL;
	brru8 ofs = 0;
	brru8 ds = 0;
	if (!cb || !data)
		return CODEBOOK_ERROR;
	for (brru4 i = 0; i < cb->codebook_count; ++i)
		ds += cb->codebooks[i].size;
	ds += 4 * cb->codebook_count;
	if (brrlib_alloc(data, ds, 1))
		return CODEBOOK_ERROR;
	offset_table = (brru4 *)((unsigned char *)(*data) + ds - 4 * cb->codebook_count);
	for (brru4 i = 0; i < cb->codebook_count; ++i) {
		packed_codebook_t *pc = &cb->codebooks[i];
		memcpy((unsigned char *)*data + ofs, pc->data, pc->size);
		ofs += pc->size;
		offset_table[i] = ofs;
	}
	if (data_size)
		*data_size = ds;
	return CODEBOOK_SUCCESS;
}
int
codebook_library_serialize(const codebook_library_t *const cb,
    void **const data, brru8 *const data_size)
{
	brru4 *offset_table = NULL;
	brru8 ds = 0, ofs = 0;
	if (!cb || !data)
		return CODEBOOK_ERROR;
	ofs = 4 * cb->codebook_count;
	ds += ofs;
	for (brru4 i = 0; i < cb->codebook_count; ++i)
		ds += cb->codebooks[i].size;
	if (brrlib_alloc(data, ds, 1))
		return CODEBOOK_ERROR;
	offset_table = (brru4 *)*data;
	for (brru4 i = 0; i < cb->codebook_count; ++i) {
		packed_codebook_t *pc = &cb->codebooks[i];
		offset_table[i] = ofs;
		memcpy((unsigned char *)*data + ofs, pc->data, pc->size);
		ofs += pc->size;
	}
	if (data_size)
		*data_size = ds;
	return CODEBOOK_SUCCESS;
}
void
codebook_library_clear(codebook_library_t *const cb)
{
	if (cb) {
		if (cb->codebooks) {
			for (brru4 i = 0; i < cb->codebook_count; ++i) {
				packed_codebook_t *pc = &cb->codebooks[i];
				packed_codebook_clear(pc);
			}
			free(cb->codebooks);
		}
		memset(cb, 0, sizeof(*cb));
	}
}
