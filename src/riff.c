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
_riff_basic_types(_int_defs)
_riff_basic_types(_fcc_defs)
_riff_list_types(_int_defs)
_riff_list_types(_fcc_defs)
_riff_sub_types(_int_defs)
_riff_sub_types(_fcc_defs)
_riff_data_types(_int_defs)
_riff_data_types(_fcc_defs)
#undef _fcc_defs
#undef _int_defs

#define _arr_defs(_l_, _d_) riff_##_l_##_int_##_d_,
const brru4 riff_basic_types[riff_basic_type_count - 1] = { _riff_basic_types(_arr_defs) };
const brru4 riff_list_types[riff_list_type_count - 1] = { _riff_list_types(_arr_defs) };
const brru4 riff_sub_types[riff_sub_type_count - 1] = { _riff_sub_types(_arr_defs) };
const brru4 riff_data_types[riff_data_type_count - 1] = { _riff_data_types(_arr_defs) };
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
	if (fcc == riff_list_type_int_RIFF) {
		return riff_byteorder_RIFF;
	} else if (fcc == riff_list_type_int_RIFX) {
		return riff_byteorder_RIFX;
	} else if (fcc == riff_list_type_int_XFIR) {
		return riff_byteorder_XFIR;
	} else if (fcc == riff_list_type_int_FFIR) {
		return riff_byteorder_FFIR;
	} else {
		return riff_byteorder_unrecognized;
	}
}
static riff_basic_typeT BRRCALL
i_get_basic_type(brru4 fcc)
{
	for (riff_basic_typeT i = 1; i < riff_basic_type_count; ++i) {
		if (fcc == riff_basic_types[i - 1])
			return i;
	}
	return riff_basic_type_unrecognized;
}
static riff_list_typeT BRRCALL
i_get_list_type(brru4 fcc)
{
	for (riff_list_typeT i = 1; i < riff_list_type_count; ++i) {
		if (fcc == riff_list_types[i - 1])
			return i;
	}
	return riff_list_type_unrecognized;
}
static riff_sub_typeT BRRCALL
i_get_sub_type(brru4 fcc)
{
	for (riff_sub_typeT i = 1; i < riff_sub_type_count; ++i) {
		if (fcc == riff_sub_types[i - 1])
			return i;
	}
	return riff_sub_type_unrecognized;
}
static riff_data_typeT BRRCALL
i_get_data_type(brru4 fcc)
{
	for (riff_data_typeT i = 1; i < riff_data_type_count; ++i) {
		if (fcc == riff_data_types[i - 1])
			return i;
	}
	return riff_data_type_unrecognized;
}

void BRRCALL
riff_chunk_info_clear(riff_chunk_infoT *const sc)
{
	if (sc) {
		memset(sc, 0, sizeof(*sc));
	}
}

int BRRCALL
riff_data_sync_check(const riff_data_syncT *const ds)
{
	if (!ds)
		return -1;
	return 0;
}
void BRRCALL
riff_data_sync_clear(riff_data_syncT *const ds)
{
	if (ds) {
		if (ds->data)
			free(ds->data);
		memset(ds, 0, sizeof(*ds));
	}
}
char *BRRCALL
riff_data_sync_buffer(riff_data_syncT *const ds, brru4 size)
{
	if (riff_data_sync_check(ds))
		return NULL;
	if (ds->consumed) {
		ds->stored -= ds->consumed;
		if (ds->stored)
			memmove(ds->data, ds->data + ds->consumed, ds->stored);
		ds->consumed = 0;
	}

	if (size > ds->storage - ds->stored) {
		brru4 new_size;
		if (size > RIFF_BUFF_MAX - RIFF_BUFF_EXTRA - ds->stored) {
			riff_data_sync_clear(ds);
			return NULL;
		}
		new_size = ds->stored + size + RIFF_BUFF_EXTRA;
		if (brrlib_alloc((void **)&ds->data, new_size, 0)) {
			riff_data_sync_clear(ds);
			return NULL;
		}
		ds->storage = new_size;
	}
	return (char *)ds->data + ds->stored;
}
int BRRCALL
riff_data_sync_apply(riff_data_syncT *const ds, brru4 size)
{
	if (riff_data_sync_check(ds))
		return RIFF_ERROR;
	if (size + ds->stored > ds->storage)
		return RIFF_ERROR;
	ds->stored += size;
	return 0;
}

int BRRCALL
riff_init(riffT *const rf)
{
	if (rf)
		memset(rf, 0, sizeof(*rf));
	return 0;
}
void BRRCALL
riff_clear(riffT *const rf)
{
	if (rf) {
		if (rf->basics) {
			for (brru4 i = 0; i < rf->n_basics; ++i)
				brrlib_alloc((void **)&rf->basics[i].data, 0, 0);
			brrlib_alloc((void **)&rf->basics, 0, 0);
		}
		if (rf->lists)
			brrlib_alloc((void **)&rf->lists, 0, 0);
		memset(rf, 0, sizeof(*rf));
	}
}
int BRRCALL
riff_check(const riffT *const rf)
{
	if (!rf)
		return RIFF_ERROR;
	return 0;
}

static int BRRCALL
i_setup_riff(riffT *const rf, riff_data_syncT *const ds)
{
	unsigned char *ckdata = ds->data + ds->consumed;
	brrs8 stor = ds->stored - ds->consumed;
	brru4 datacc, typecc;
	int idx = 0;
	if (stor < 4) { /* Not enough DATA */
		return RIFF_CHUNK_INCOMPLETE;
	}
	memcpy(&typecc, ckdata, 4);
	if (!(ds->byteorder = i_determine_byteorder(typecc))) {
		riff_data_sync_clear(ds);
		riff_clear(rf);
		return RIFF_NOT_RIFF;
	}
#if BRRENDIAN_SYSTE == BRRENDIAN_BIG
	idx = 3 - (rf->byteorder - 1);
#else
	idx = ds->byteorder - 1;
#endif
	/* Function pointers for interpreting data from the given riff. */
	ds->cpy_cc    = memfuntable[ 0 + idx];
	ds->move_cc   = memfuntable[ 4 + idx];
	ds->cpy_data  = memfuntable[ 8 + idx];
	ds->move_data = memfuntable[12 + idx];

	ds->cpy_data(&rf->total_size, ckdata + 4, 4);
	ds->cpy_cc(&datacc, ckdata + 8, 4);
	ds->consumed += 12;
	rf->data_type = i_get_data_type(datacc);
	return RIFF_CONSUME_MORE;
}
static int BRRCALL
i_add_basic_type(riffT *const rf, riff_data_syncT *const ds, riff_basic_typeT cktype, brru4 cksize)
{
	unsigned char *ckdata = ds->data + ds->consumed;
	riff_basic_chunkT basic = {0};
	basic.type = cktype;
	basic.size = cksize;
	if (brrlib_alloc((void **)&basic.data, basic.size, 1)) {
		riff_data_sync_clear(ds);
		riff_clear(rf);
		return RIFF_ERROR;
	}
	ds->cpy_data(basic.data, ckdata, basic.size);
	ds->consumed += basic.size;
	if (brrlib_alloc((void **)&rf->basics, (rf->n_basics + 1) * sizeof(*rf->basics), 0)) {
		riff_data_sync_clear(ds);
		riff_clear(rf);
		return RIFF_ERROR;
	}
	rf->basics[rf->n_basics++] = basic;
	if (ds->list_end > 0) { /* Again, assuming lists can't have lists as subelements */
		riff_list_chunkT *list = &rf->lists[rf->n_lists - 1];
		if (list->n_basics == 0)
			list->first_basic_index = rf->n_basics - 1;
		list->n_basics++;
		ds->list_end -= basic.size + 8;
	} else if (ds->list_end < 0) {
		/* List was corrupted, stream is too */
		riff_data_sync_clear(ds);
		riff_clear(rf);
		return RIFF_CORRUPTED;
	}
	return RIFF_CHUNK_CONSUMED;
}
static int BRRCALL
i_add_list_type(riffT *const rf, riff_data_syncT *const ds, riff_list_typeT cktype, brru4 cksize)
{
	unsigned char *ckdata = ds->data + ds->consumed;
	riff_list_chunkT list = {0};
	brru4 subcc;
	ds->cpy_cc(&subcc, ckdata, 4);
	ds->consumed += 4;
	list.size = cksize;
	list.type = cktype;
	list.sub_type = i_get_sub_type(subcc);
	list.first_basic_index = rf->n_basics;
	list.n_basics = 0;
	ds->list_end = list.size - 4; /* list size includes sub_type */
	if (brrlib_alloc((void **)&rf->lists, (rf->n_lists + 1) * sizeof(*rf->lists), 0)) {
		riff_data_sync_clear(ds);
		riff_clear(rf);
		return RIFF_ERROR;
	}
	rf->lists[rf->n_lists++] = list;
	return RIFF_CHUNK_CONSUMED;
}
int BRRCALL
riff_consume_chunk(riffT *const rf, riff_chunk_infoT *const sc, riff_data_syncT *const ds)
{
	unsigned char *ckdata = ds->data + ds->consumed;
	brrs8 stor = ds->stored - ds->consumed;
	if (riff_check(rf) || riff_data_sync_check(ds) || !sc)
		return RIFF_ERROR;

	if (!ds->byteorder) /* Initialize riff info from header chunk fcc and size */
		return i_setup_riff(rf, ds);

	if (sc->is_basic) {
		int err = 0;
		if (stor < sc->chunksize) /* Not enough to fill out chunk */
			return RIFF_CHUNK_INCOMPLETE;
		else if (RIFF_CHUNK_CONSUMED != (err = i_add_basic_type(rf, ds, sc->chunk_type, sc->chunksize)))
			return err;
		sc->chunk_info_index = rf->n_basics - 1;
		return RIFF_CHUNK_CONSUMED;
	} else if (sc->is_list) {
		int err = 0;
		if (ds->list_end > 0) {
			/* I don't think lists can be subchunks of other lists */
			riff_data_sync_clear(ds);
			riff_clear(rf);
			return RIFF_NOT_RIFF;
		}
		if (stor < 4) /* Not enough to get sub_type */
			return RIFF_CHUNK_INCOMPLETE;
		else if (RIFF_CHUNK_CONSUMED != (err = i_add_list_type(rf, ds, sc->chunk_type, sc->chunksize)))
			return err;
		sc->chunk_info_index = rf->n_lists - 1;
		return RIFF_CHUNK_CONSUMED;
	} else { /* We don't know yet. */
		int cktype = 0;
		if (stor < 8) { /* Not enough for fcc + size */
			/* Size can be 0, I'm not sure about fcc though */
			return RIFF_CHUNK_INCOMPLETE;
		}
		ds->cpy_cc(&sc->chunkcc, ckdata, 4);
		ds->cpy_data(&sc->chunksize, ckdata + 4, 4);
		ds->consumed += 8;
		if ((cktype = i_get_basic_type(sc->chunkcc))) {
			sc->is_basic = 1;
			sc->is_list = 0;
			sc->chunk_type = cktype;
		} else if ((cktype = i_get_list_type(sc->chunkcc))) {
			sc->is_basic = 0;
			sc->is_list = 1;
			sc->chunk_type = cktype;
		} else { /* desync/unrecognized chunk */
			ds->consumed += sc->chunksize; /* skip it for now */
			return RIFF_CHUNK_UNRECOGNIZED;
		}
		return RIFF_CONSUME_MORE;
	}
}
