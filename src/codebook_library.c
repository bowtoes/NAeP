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

#include "codebook_library.h"

#include <stdlib.h>
#include <string.h>
#include <brrtools/brrlib.h>

/* TODO Same issue as elsewhere, I can't verify how big-endian systems will
 * work with this, or if any modification is necessary */

int BRRCALL
codebook_library_deserialize_deprecated(codebook_libraryT *const cb,
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
		packed_codebookT *pc = &cb->codebooks[i];

		if (end < start || end > data_size)
			return CODEBOOK_CORRUPT;
		pc->codebook_size = end - start;
		if (brrlib_alloc((void **)&pc->codebook_data, pc->codebook_size, 1)) {
			codebook_library_clear(cb);
			return CODEBOOK_ERROR;
		}
		memcpy(pc->codebook_data, dt + start, pc->codebook_size);

		start = end;
	}
	return CODEBOOK_SUCCESS;
}
int BRRCALL
codebook_library_deserialize(codebook_libraryT *const cb,
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
		packed_codebookT *pc = &cb->codebooks[i - 1];
		if (end < start || start > data_size)
			return CODEBOOK_CORRUPT;
		pc->codebook_size = end - start;
		if (brrlib_alloc((void **)&pc->codebook_data, pc->codebook_size, 1)) {
			codebook_library_clear(cb);
			return CODEBOOK_ERROR;
		}
		memcpy(pc->codebook_data, dt + start, pc->codebook_size);
		end = start;
	}
	return CODEBOOK_SUCCESS;
}
int BRRCALL
codebook_library_serialize_deprecated(const codebook_libraryT *const cb,
    void **const data, brru8 *const data_size)
{
	brru4 *offset_table = NULL;
	brru8 ofs = 0;
	brru8 ds = 0;
	if (!cb || !data)
		return CODEBOOK_ERROR;
	for (brru4 i = 0; i < cb->codebook_count; ++i)
		ds += cb->codebooks[i].codebook_size;
	ds += 4 * cb->codebook_count;
	if (brrlib_alloc(data, ds, 1))
		return CODEBOOK_ERROR;
	offset_table = (brru4 *)((unsigned char *)(*data) + ds - 4 * cb->codebook_count);
	for (brru4 i = 0; i < cb->codebook_count; ++i) {
		packed_codebookT *pc = &cb->codebooks[i];
		memcpy((unsigned char *)*data + ofs, pc->codebook_data, pc->codebook_size);
		ofs += pc->codebook_size;
		offset_table[i] = ofs;
	}
	if (data_size)
		*data_size = ds;
	return CODEBOOK_SUCCESS;
}
int BRRCALL
codebook_library_serialize(const codebook_libraryT *const cb,
    void **const data, brru8 *const data_size)
{
	brru4 *offset_table = NULL;
	brru8 ds = 0, ofs = 0;
	if (!cb || !data)
		return CODEBOOK_ERROR;
	ofs = 4 * cb->codebook_count;
	ds += ofs;
	for (brru4 i = 0; i < cb->codebook_count; ++i)
		ds += cb->codebooks[i].codebook_size;
	if (brrlib_alloc(data, ds, 1))
		return CODEBOOK_ERROR;
	offset_table = (brru4 *)*data;
	for (brru4 i = 0; i < cb->codebook_count; ++i) {
		packed_codebookT *pc = &cb->codebooks[i];
		offset_table[i] = ofs;
		memcpy((unsigned char *)*data + ofs, pc->codebook_data, pc->codebook_size);
		ofs += pc->codebook_size;
	}
	if (data_size)
		*data_size = ds;
	return CODEBOOK_SUCCESS;
}
void BRRCALL
codebook_library_clear(codebook_libraryT *const cb)
{
	if (cb) {
		if (cb->codebooks) {
			for (brru4 i = 0; i < cb->codebook_count; ++i) {
				packed_codebookT *pc = &cb->codebooks[i];
				if (pc->codebook_data)
					free(pc->codebook_data);
			}
			free(cb->codebooks);
		}
		memset(cb, 0, sizeof(*cb));
	}
}
