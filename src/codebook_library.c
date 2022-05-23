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
 * work with the (de)serializations, or if any modification is necessary */

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

int
packed_codebook_unpack(packed_codebook_t *const pc)
{
	if (!pc)
		return CODEBOOK_ERROR;

	if (pc->did_unpack)
		return CODEBOOK_SUCCESS;
	if (pc->unpacked_data)
		return CODEBOOK_CORRUPT;

	oggpack_buffer unpacker;
	oggpack_readinit(&unpacker, pc->data, pc->size);

	oggpack_buffer packer;
	oggpack_writeinit(&packer);

	int err = 0;
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
packed_codebook_unpack_raw(oggpack_buffer *const unpacker, oggpack_buffer *const packer)
{
	if (!unpacker)
		return -1;

	packer_pack(packer, 'B', 8); /* W Sync */
	packer_pack(packer, 'C', 8); /* W Sync */
	packer_pack(packer, 'V', 8); /* W Sync */

	int dimensions = packer_transfer(unpacker,  4, packer, 16); /* R/W Dimensions */
	int entries = packer_transfer(unpacker, 14, packer, 24);    /* R/W Entries */
	int ordered = packer_transfer(unpacker, 1, packer, 1);      /* R/W Ordered flag */
	if (ordered) {
		/* Ordered codeword decode identical to spec */
		int current_length = 1 + packer_transfer(unpacker, 5, packer, 5); /* R/W Start length */
		long current_entry = 0;
		while (current_entry < entries) {
			int number_bits = lib_count_bits(entries - current_entry);
			long number = packer_transfer(unpacker, number_bits, packer, number_bits); /* R/W Magic number */
			current_entry += number;
			current_length++;
		}
		if (current_entry > entries)
			return CODEBOOK_CORRUPT;

	} else {
		int codeword_length_bits = packer_unpack(unpacker, 3); /* R Codeword length bits */
		if (codeword_length_bits < 0 || codeword_length_bits > 5)
			return CODEBOOK_CORRUPT;

		int sparse = packer_transfer(unpacker, 1, packer, 1);   /* R/W Sparse flag */
		if (!sparse) {
			/* R/W Nonsparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				int length = packer_transfer(unpacker, codeword_length_bits, packer, 5);
			}

		} else {
			/* R/W Sparse codeword lengths */
			for (int i = 0; i < entries; ++i) {
				/* R/W Used flag */
				int used = packer_transfer(unpacker, 1, packer, 1);
				if (used) {
					int length = packer_transfer(unpacker, codeword_length_bits, packer, 5); /* R/W Codeword length */
				}
			}
		}
	}

	int lookup = packer_transfer(unpacker, 1, packer, 4); /* R/W Lookup type */
	if (lookup == 1) {
		/* Lookup 1 decode identical to spec */
		long minval_packed  =     packer_transfer(unpacker, 32, packer, 32); /* R/W Minimum value */
		long delval_packed  =     packer_transfer(unpacker, 32, packer, 32); /* R/W Delta value */
		int  value_bits     = 1 + packer_transfer(unpacker,  4, packer,  4); /* R/W Value bits */
		int  sequence_flag  =     packer_transfer(unpacker,  1, packer,  1); /* R/W Sequence flag */

		long lookup_values = lib_lookup1_values(entries, dimensions);
		for (long i = 0; i < lookup_values; ++i) { /* R/W Codebook multiplicands */
			long multiplicand = packer_transfer(unpacker, value_bits, packer, value_bits);
		}

	} else if (lookup) {
		/* Bad lookup value, can only be 0 or 1 */
		return CODEBOOK_CORRUPT;
	}

	return CODEBOOK_SUCCESS;
}

void
codebook_library_clear(codebook_library_t *const library)
{
	if (library) {
		if (library->codebooks) {
			for (brru4 i = 0; i < library->codebook_count; ++i) {
				packed_codebook_t *pc = &library->codebooks[i];
				packed_codebook_clear(pc);
			}
			free(library->codebooks);
		}
		memset(library, 0, sizeof(*library));
	}
}

int
codebook_library_deserialize_alt(codebook_library_t *const library, const void *const input_data, brru8 data_size)
{
	if (!input_data || !library || data_size < 4)
		return CODEBOOK_ERROR;

	const char *const data = (const char *)input_data;
	/* End of the codebook table (start of the index table) */
	brru4 last_end = *(brru4 *)(data + data_size - 4);
	if (last_end > data_size - 4)
		return CODEBOOK_CORRUPT;

	/* Every 4 bytes from the the end of the codebook table 'last_end' to the end of the data
	 * is a byte-offset to the end of a packed-codebook, all in sequential order. */
	const brru4 *offset_table = (brru4 *)(data + last_end);
	brru4 count = (data_size - last_end) / sizeof(brru4);

	codebook_library_t lib = {0};
	if (brrlib_alloc((void **)&lib.codebooks, count * sizeof(*lib.codebooks), 1)) {
		codebook_library_clear(&lib);
		return CODEBOOK_ERROR;
	}

	brru4 pc_start = 0;
	for (;lib.codebook_count < count; ++lib.codebook_count) {
		brru4 pc_end = offset_table[lib.codebook_count];

		if (pc_end < pc_start || pc_end > data_size) {
			codebook_library_clear(&lib);
			return CODEBOOK_CORRUPT;
		}

		packed_codebook_t pc = {.size = pc_end - pc_start};
		if (brrlib_alloc((void **)&pc.data, pc.size, 1)) {
			codebook_library_clear(&lib);
			return CODEBOOK_ERROR;
		}
		memcpy(pc.data, data + pc_start, pc.size);
		lib.codebooks[lib.codebook_count] = pc;

		pc_start = pc_end;
	}
	*library = lib;
	return CODEBOOK_SUCCESS;
}

int
codebook_library_deserialize(codebook_library_t *const library, const void *const input_data, brru8 data_size)
{
	if (!library || !input_data || data_size < 4)
		return CODEBOOK_ERROR;

	/* The offset table is at the start of the data this time, and each offset is the the start of a codebook,
	 * rather than the end. */
	const brru4 *offset_table = (const brru4*)input_data;
	brru4 count = offset_table[0] / 4;

	codebook_library_t lib = {0};
	if (brrlib_alloc((void **)&lib.codebooks, count * sizeof(*lib.codebooks), 1)) {
		codebook_library_clear(&lib);
		return CODEBOOK_ERROR;
	}

	/* Codebooks are loaded in reverse order so that I don't have to read the last one outside the loop. */
	const char *data = (const char *)input_data;
	brru4 pc_end = data_size;
	for (brru4 i = count; i > 0; --i, ++lib.codebook_count) {
		brru4 pc_start = offset_table[i - 1];
		if (pc_end < pc_start || pc_start > data_size) {
			codebook_library_clear(&lib);
			return CODEBOOK_CORRUPT;
		}

		packed_codebook_t pc = {.size = pc_end - pc_start};
		if (brrlib_alloc((void **)&pc.data, pc.size, 1)) {
			codebook_library_clear(&lib);
			return CODEBOOK_ERROR;
		}
		memcpy(pc.data, data + pc_start, pc.size);
		lib.codebooks[i - 1] = pc;

		pc_end = pc_start;
	}
	*library = lib;
	return CODEBOOK_SUCCESS;
}

int
codebook_library_serialize_alt(const codebook_library_t *const library, void **const output_data, brru8 *const data_size)
{
	if (!library || !output_data)
		return CODEBOOK_ERROR;

	const codebook_library_t lib = *library;

	brru8 total_size = 4 * lib.codebook_count;
	for (brru4 i = 0; i < lib.codebook_count; ++i)
		total_size += lib.codebooks[i].size;

	if (brrlib_alloc(output_data, total_size, 1))
		return CODEBOOK_ERROR;

	char *const data = *(char **)output_data;
	brru8 pc_start = 0;
	brru4 *offset_table = (brru4 *)(data + total_size - 4 * lib.codebook_count);
	for (brru4 i = 0; i < lib.codebook_count; ++i) {
		const packed_codebook_t pc = lib.codebooks[i];
		memcpy(data + pc_start, pc.data, pc.size);
		pc_start += pc.size;
		offset_table[i] = pc_start;
	}

	if (data_size)
		*data_size = total_size;
	return CODEBOOK_SUCCESS;
}

int
codebook_library_serialize(const codebook_library_t *const library, void **const output_data, brru8 *const data_size)
{
	if (!library || !output_data)
		return CODEBOOK_ERROR;

	const codebook_library_t lib = *library;

	brru8 total_size = 4 * lib.codebook_count;
	for (brru4 i = 0; i < lib.codebook_count; ++i)
		total_size += lib.codebooks[i].size;

	if (brrlib_alloc(output_data, total_size, 1))
		return CODEBOOK_ERROR;

	char *const data = *(char **)output_data;

	brru8 pc_start = 4 * lib.codebook_count;
	brru4 *offset_table = (brru4 *)data;
	for (brru4 i = 0; i < lib.codebook_count; ++i) {
		const packed_codebook_t pc = lib.codebooks[i];
		memcpy(data + pc_start, pc.data, pc.size);
		offset_table[i] = pc_start;
		pc_start += pc.size;
	}

	if (data_size)
		*data_size = total_size;
	return CODEBOOK_SUCCESS;
}
