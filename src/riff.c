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

#define _riff_keep_types_defines
#include "riff.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrmem.h>
#include <brrtools/brrplatform.h>

#define RIFF_BUFF_EXTRA 4096
#define RIFF_BUFF_MAX INT_MAX

#define _int_defs(_l_, _d_) const brru4 riff_##_l_##_int_##_d_ = FCC_CODE_INT(#_d_"    ");
#define _fcc_defs(_l_, _d_) const fourccT riff_##_l_##_fcc_##_d_ = FCC_INIT(#_d_"    ");
_riff_basictypes(_int_defs)
_riff_basictypes(_fcc_defs)
_riff_listtypes(_int_defs)
_riff_listtypes(_fcc_defs)
_riff_subtypes(_int_defs)
_riff_subtypes(_fcc_defs)
_riff_datatypes(_int_defs)
_riff_datatypes(_fcc_defs)
#undef _fcc_defs
#undef _int_defs

#define _arr_defs(_l_, _d_) riff_##_l_##_int_##_d_,
const brru4 riff_basictypes[riff_basictype_count - 1] = { _riff_basictypes(_arr_defs) };
const brru4 riff_listtypes[riff_listtype_count - 1] = { _riff_listtypes(_arr_defs) };
const brru4 riff_subtypes[riff_subtype_count - 1] = { _riff_subtypes(_arr_defs) };
const brru4 riff_datatypes[riff_datatype_count - 1] = { _riff_datatypes(_arr_defs) };
#undef _arr_defs

static void* BRRCALL
cpy_rvs(void *restrict const dst, const void *restrict const src, size_t size)
{
	memcpy(dst, src, size);
	_brrmem_reverse(dst, size);
	return dst;
}
static void* BRRCALL
move_rvs(void *const dst, const void *const src, size_t size)
{
	memmove(dst, src, size);
	_brrmem_reverse(dst, size);
	return dst;
}
static void *(*const BRRCALL memfuntable[])(void *const, const void *const, size_t) = {
	memcpy, memcpy,  cpy_rvs, cpy_rvs, memmove, memmove,  move_rvs, move_rvs,
	memcpy, cpy_rvs, memcpy,  cpy_rvs, memmove, move_rvs, memmove,  move_rvs,
};

static riff_byteorderT BRRCALL
i_determine_byteorder(brru4 fcc)
{
	if (fcc == riff_listtype_int_RIFF) {
		return riff_byteorder_RIFF;
	} else if (fcc == riff_listtype_int_RIFX) {
		return riff_byteorder_RIFX;
	} else if (fcc == riff_listtype_int_XFIR) {
		return riff_byteorder_XFIR;
	} else if (fcc == riff_listtype_int_FFIR) {
		return riff_byteorder_FFIR;
	} else {
		return riff_byteorder_unrecognized;
	}
}
static riff_basictypeT BRRCALL
i_get_basictype(brru4 fcc)
{
	for (riff_basictypeT i = 1; i < riff_basictype_count; ++i) {
		if (fcc == riff_basictypes[i - 1])
			return i;
	}
	return riff_basictype_unrecognized;
}
static riff_listtypeT BRRCALL
i_get_listtype(brru4 fcc)
{
	for (riff_listtypeT i = 1; i < riff_listtype_count; ++i) {
		if (fcc == riff_listtypes[i - 1])
			return i;
	}
	return riff_listtype_unrecognized;
}
static riff_subtypeT BRRCALL
i_get_subtype(brru4 fcc)
{
	for (riff_subtypeT i = 1; i < riff_subtype_count; ++i) {
		if (fcc == riff_subtypes[i - 1])
			return i;
	}
	return riff_subtype_unrecognized;
}
static riff_datatypeT BRRCALL
i_get_datatype(brru4 fcc)
{
	for (riff_datatypeT i = 1; i < riff_datatype_count; ++i) {
		if (fcc == riff_datatypes[i - 1])
			return i;
	}
	return riff_datatype_unrecognized;
}

void BRRCALL
riff_chunkinfo_clear(riff_chunkinfoT *const fo)
{
	if (fo) {
		memset(fo, 0, sizeof(*fo));
	}
}

int BRRCALL
riff_init(riffT *const rf)
{
	if (rf) {
		memset(rf, 0, sizeof(*rf));
	}
	return 0;
}
void BRRCALL
riff_clear(riffT *const rf)
{
	if (rf) {
		if (rf->basics) {
			for (brru4 i = 0; i < rf->n_basics; ++i) {
				brrlib_alloc((void **)&rf->basics[i].data, 0, 0);
			}
			brrlib_alloc((void **)&rf->basics, 0, 0);
		}
		if (rf->lists) { /* Lists store pointers back into rf, no need to free them */
			brrlib_alloc((void **)&rf->lists, 0, 0);
		}
		memset(rf, 0, sizeof(*rf));
	}
}
int BRRCALL
riff_check(riffT *const rf)
{
	if (!rf)
		return RIFF_ERROR;
	return 0;
}
char *BRRCALL
riff_buffer(riffT *const rf, brru4 size)
{
	if (riff_check(rf))
		return NULL;

	if (rf->consumed) {
		rf->stored -= rf->consumed;
		if (rf->stored)
			memmove(rf->data, rf->data + rf->consumed, rf->stored);
		rf->consumed = 0;
	}

	if (size > rf->storage - rf->stored) {
		brru4 new_size;
		if (size > RIFF_BUFF_MAX - RIFF_BUFF_EXTRA - rf->stored) {
			riff_clear(rf);
			return NULL;
		}
		new_size = rf->stored + size + RIFF_BUFF_EXTRA;
		if (brrlib_alloc((void **)&rf->data, new_size, 0)) {
			riff_clear(rf);
			return NULL;
		}
		rf->storage = new_size;
	}
	return (char *)rf->data + rf->stored;
}
int BRRCALL
riff_apply_buffer(riffT *const rf, brru4 size)
{
	if (riff_check(rf))
		return RIFF_ERROR;
	if (size + rf->stored > rf->storage)
		return RIFF_ERROR;

	rf->stored += size;
	return 0;
}

static int BRRCALL
i_setup_riff(riffT *const rf, brru4 stor)
{
	unsigned char *ckdata = rf->data + rf->consumed;
	brru4 datacc, typecc;
	int idx = 0;
	if (stor < 4) { /* Not enough DATA */
		return RIFF_CHUNK_INCOMPLETE;
	}
	memcpy(&typecc, ckdata, 4);
	if (!(rf->byteorder = i_determine_byteorder(typecc))) {
		riff_clear(rf);
		return RIFF_NOT_RIFF;
	}
#if BRRENDIAN_SYSTE == BRRENDIAN_BIG
	idx = 3 - (rf->byteorder - 1);
#else
	idx = rf->byteorder - 1;
#endif
	/* Function pointers for interpreting data from the given riff. */
	rf->cpy_cc    = memfuntable[ 0 + idx];
	rf->move_cc   = memfuntable[ 4 + idx];
	rf->cpy_data  = memfuntable[ 8 + idx];
	rf->move_data = memfuntable[12 + idx];

	rf->cpy_data(&rf->total_size, ckdata + 4, 4);
	rf->cpy_cc(&datacc, ckdata + 8, 4);
	rf->datatype = i_get_datatype(datacc);
	rf->consumed += 12;
	return RIFF_CONSUME_MORE;
}
static int BRRCALL
i_add_basictype(riffT *const rf, riff_basictypeT cktype, brru4 cksize)
{
	unsigned char *ckdata = rf->data + rf->consumed;
	riff_basic_chunkinfoT basic = {0};
	basic.type = cktype;
	basic.size = cksize;
	if (brrlib_alloc((void **)&basic.data, basic.size, 1)) {
		riff_clear(rf);
		return RIFF_ERROR;
	}
	rf->cpy_data(basic.data, ckdata, basic.size);
	rf->consumed += basic.size;
	if (brrlib_alloc((void **)&rf->basics, (rf->n_basics + 1) * sizeof(*rf->basics), 0)) {
		riff_clear(rf);
		return RIFF_ERROR;
	}
	rf->basics[rf->n_basics++] = basic;
	if (rf->list_end > 0) { /* Again, assuming lists can't have lists as subelements */
		riff_list_chunkinfoT *list = &rf->lists[rf->n_lists - 1];
		if (list->n_basics == 0)
			list->first_basic_index = rf->n_basics - 1;
		list->n_basics++;
		rf->list_end -= basic.size + 8;
	} else if (rf->list_end < 0) {
		/* List was corrupted, stream is too */
		riff_clear(rf);
		return RIFF_CORRUPTED;
	}
	return RIFF_CHUNK_CONSUMED;
}
static int BRRCALL
i_add_listtype(riffT *const rf, riff_listtypeT cktype, brru4 cksize)
{
	unsigned char *ckdata = rf->data + rf->consumed;
	riff_list_chunkinfoT list = {0};
	brru4 subcc;
	rf->cpy_cc(&subcc, ckdata, 4);
	rf->consumed += 4;
	list.size = cksize;
	list.type = cktype;
	list.subtype = i_get_subtype(subcc);
	list.first_basic_index = rf->n_basics;
	list.n_basics = 0;
	rf->list_end = list.size - 4; /* list size includes subtype */
	if (brrlib_alloc((void **)&rf->lists, (rf->n_lists + 1) * sizeof(*rf->lists), 0)) {
		riff_clear(rf);
		return RIFF_ERROR;
	}
	rf->lists[rf->n_lists++] = list;
	return RIFF_CHUNK_CONSUMED;
}
int BRRCALL
riff_consume_chunk(riffT *const rf, riff_chunkinfoT *const fo)
{
	unsigned char *ckdata = rf->data + rf->consumed;
	brrs8 stor = rf->stored - rf->consumed;
	if (riff_check(rf) || !fo)
		return RIFF_ERROR;

	if (!rf->byteorder) /* Initialize riff info from header chunk fcc and size */
		return i_setup_riff(rf, stor);

	if (fo->is_basic) {
		int err = 0;
		if (stor < fo->chunksize) /* Not enough to fill out chunk */
			return RIFF_CHUNK_INCOMPLETE;
		else if (RIFF_CHUNK_CONSUMED != (err = i_add_basictype(rf, fo->chunk_type, fo->chunksize)))
			return err;
		fo->chunkinfo_index = rf->n_basics - 1;
		return RIFF_CHUNK_CONSUMED;
	} else if (fo->is_list) {
		int err = 0;
		if (rf->list_end > 0) {
			/* I don't think lists can be subchunks of other lists */
			riff_clear(rf);
			return RIFF_NOT_RIFF;
		}
		if (stor < 4) /* Not enough to get subtype */
			return RIFF_CHUNK_INCOMPLETE;
		else if (RIFF_CHUNK_CONSUMED != (err = i_add_listtype(rf, fo->chunk_type, fo->chunksize)))
			return err;
		fo->chunkinfo_index = rf->n_lists - 1;
		return RIFF_CHUNK_CONSUMED;
	} else { /* We don't know yet. */
		int cktype = 0;
		if (stor < 8) { /* Not enough for fcc + size */
			/* Size can be 0, I'm not sure about fcc though */
			return RIFF_CHUNK_INCOMPLETE;
		}
		rf->cpy_cc(&fo->chunkcc, ckdata, 4);
		rf->cpy_data(&fo->chunksize, ckdata + 4, 4);
		rf->consumed += 8;
		if ((cktype = i_get_basictype(fo->chunkcc))) {
			fo->is_basic = 1;
			fo->is_list = 0;
			fo->chunk_type = cktype;
		} else if ((cktype = i_get_listtype(fo->chunkcc))) {
			fo->is_basic = 0;
			fo->is_list = 1;
			fo->chunk_type = cktype;
		} else { /* desync/unrecognized chunk */
			rf->consumed += fo->chunksize; /* skip it for now */
			return RIFF_CHUNK_UNRECOGNIZED;
		}
		return RIFF_CONSUME_MORE;
	}
}
