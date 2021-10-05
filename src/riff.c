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

#define _riff_keepsies
#include "riff.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrmem.h>
#include <brrtools/brrendian.h>

#include "lib.h"

#define RIFF_BUFF_EXTRA 4096
#define RIFF_BUFF_MAX INT_MAX

#define _array_processor(_type_, _cc_) FCC_GET_INT(#_cc_"    "),
#define _arrlen(_k_) (sizeof(_k_)/sizeof((_k_)[0]))
#define _getter_boiler(_t_,_t2_) \
	const brru4 riff_##_t_##_ccs[riff_##_t_##_count - 1] = { _riff_##_t_##s_gen(_array_processor) }; \
    riff_##_t_##_t2_##T BRRCALL \
    riff_cc_##_t_##_t2_(brru4 cc) { \
    	riff_##_t_##_t2_##T t = 0; \
    	for (;t < _arrlen(riff_##_t_##_ccs); ++t) { \
    		if (cc == riff_##_t_##_ccs[t]) \
    			return t + 1; \
    	} \
    	return 0; \
    }
_riff_boiler_gen(_getter_boiler)
#undef _getter_boiler
#undef _array_processor

riff_byteorderT BRRCALL
riff_cc_byteorder(brru4 cc)
{
	for (riff_byteorderT t = 0;t < 4; ++t) {
		if (cc == riff_root_ccs[t])
			return 1 + t;
	}
	return 0;
}

static void*
cpy_rvs(void *restrict const dst, const void *restrict const src, size_t size)
{
	memcpy(dst, src, size);
	_brrmem_reverse(dst, size);
	return dst;
}
static void*
move_rvs(void *const dst, const void *const src, size_t size)
{
	memmove(dst, src, size);
	_brrmem_reverse(dst, size);
	return dst;
}

#if BRRENDIAN_SYSTEM == BRRENDIAN_BIG
# define _riff_copier_idx(_bo_) (4 - (_bo_))
#else
# define _riff_copier_idx(_bo_) ((_bo_) - 1)
#endif

static void *(*const _riff_copy_table[8])(void *const, const void *const, size_t) = {
	memcpy, memcpy,  cpy_rvs, cpy_rvs,
	memcpy, cpy_rvs, memcpy,  cpy_rvs,
};
riff_copierT BRRCALL
riff_copier_cc(riff_byteorderT byteorder)
{
	if (!byteorder)
		return _riff_copy_table[0];
	return _riff_copy_table[0 + _riff_copier_idx(byteorder)];
}
riff_copierT BRRCALL
riff_copier_data(riff_byteorderT byteorder)
{
	if (!byteorder)
		return _riff_copy_table[4];
	return _riff_copy_table[4 + _riff_copier_idx(byteorder)];
}

void BRRCALL
riff_chunk_info_clear(riff_chunk_infoT *const sc)
{
	if (sc) {
		memset(sc, 0, sizeof(*sc));
	}
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
int BRRCALL
riff_data_sync_check(const riff_data_syncT *const ds)
{
	if (!ds || ds->consumed > ds->stored || ds->stored > ds->storage)
		return -1;
	return 0;
}
int BRRCALL
riff_data_sync_from_buffer(riff_data_syncT *const ds,
    unsigned char *const buffer, brrsz buffer_size)
{
	if (riff_data_sync_check(ds))
		return -1;
	else if (!buffer || !buffer_size)
		return 0;
	ds->data = buffer;
	ds->stored = ds->storage = buffer_size;
	return 0;
}
int BRRCALL
riff_data_sync_seek(riff_data_syncT *const ds, brrof offset)
{
	if (riff_data_sync_check(ds))
		return -1;
	if (offset > 0 && offset + ds->consumed > ds->stored)
		offset = -ds->consumed + ds->stored;
	else if (offset < 0 && -offset > ds->consumed)
		offset = -ds->consumed;
	ds->consumed += offset;
	return 0;
}
char *BRRCALL
riff_data_sync_buffer(riff_data_syncT *const ds, brrsz size)
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
		if (size + ds->stored > RIFF_BUFF_MAX - RIFF_BUFF_EXTRA)
			return NULL;
		new_size = ds->stored + size + RIFF_BUFF_EXTRA;
		if (brrlib_alloc((void **)&ds->data, new_size, 0))
			return NULL;
		ds->storage = new_size;
	}
	return (char *)ds->data + ds->stored;
}
int BRRCALL
riff_data_sync_apply(riff_data_syncT *const ds, brrsz size)
{
	if (riff_data_sync_check(ds))
		return RIFF_ERROR;
	if (size + ds->stored > ds->storage)
		return RIFF_ERROR;
	ds->stored += size;
	return 0;
}

void BRRCALL
riff_clear(riffT *const rf)
{
	if (rf) {
		if (rf->basics) {
			for (brrsz i = 0; i < rf->n_basics; ++i)
				free(rf->basics[i].data);
			free(rf->basics);
		}
		if (rf->lists)
			free(rf->lists);
		memset(rf, 0, sizeof(*rf));
	}
}
int BRRCALL
riff_init(riffT *const rf)
{
	if (rf)
		memset(rf, 0, sizeof(*rf));
	return 0;
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
	brrsz stor = ds->stored - ds->consumed;
	brru4 formatcc, rootcc;
	if (stor < 12) { /* Not enough chunkcc, size, and formatcc */
		return RIFF_CHUNK_INCOMPLETE;
	}
	memcpy(&rootcc, ckdata, 4);
	if (!(ds->byteorder = riff_cc_byteorder(rootcc))) {
		riff_data_sync_clear(ds);
		riff_clear(rf);
		return RIFF_NOT_RIFF;
	}
	ds->cpy_cc   = riff_copier_cc(ds->byteorder);
	ds->cpy_data = riff_copier_data(ds->byteorder);

	ds->cpy_data(&rf->total_size, ckdata + 4, 4);
	ds->cpy_cc(&formatcc, ckdata + 8, 4);
	ds->consumed += 12;
	rf->root = riff_cc_root(rootcc);
	rf->format = riff_cc_format(formatcc);
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
		return RIFF_ERROR;
	}
	ds->cpy_data(basic.data, ckdata, basic.size);
	ds->consumed += basic.size;
	if (brrlib_alloc((void **)&rf->basics, (rf->n_basics + 1) * sizeof(*rf->basics), 0)) {
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
		return RIFF_CORRUPTED;
	}
	return RIFF_CHUNK_CONSUMED;
}
static int BRRCALL
i_add_list_type(riffT *const rf, riff_data_syncT *const ds, brru4 cksize)
{
	unsigned char *ckdata = ds->data + ds->consumed;
	riff_list_chunkT list = {0};
	brru4 formatcc;
	ds->cpy_cc(&formatcc, ckdata, 4);
	ds->consumed += 4;
	list.size = cksize;
	list.type = riff_cc_list_format(formatcc);
	list.first_basic_index = rf->n_basics;
	list.n_basics = 0;
	ds->list_end = list.size - 4; /* list size includes formatcc*/
	if (brrlib_alloc((void **)&rf->lists, (rf->n_lists + 1) * sizeof(*rf->lists), 0)) {
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
		sc->chunk_index = rf->n_basics - 1;
		return RIFF_CHUNK_CONSUMED;
	} else if (sc->is_list) {
		int err = 0;
		if (ds->list_end > 0) {
			/* I don't think lists can be subchunks of other lists */
			return RIFF_NOT_RIFF;
		}
		if (stor < 4) /* Not enough to get sub_type */
			return RIFF_CHUNK_INCOMPLETE;
		else if (RIFF_CHUNK_CONSUMED != (err = i_add_list_type(rf, ds, sc->chunksize)))
			return err;
		sc->chunk_index = rf->n_lists - 1;
		return RIFF_CHUNK_CONSUMED;
	} else { /* We don't know yet. */
		int cktype = 0;
		if (stor < 8) { /* Not enough for fcc + size */
			/* Size can be 0, I'm not sure about fcc though */
			return RIFF_CHUNK_INCOMPLETE;
		}
		ds->cpy_cc(&sc->chunkcc, ckdata, 4);
		ds->cpy_data(&sc->chunksize, ckdata + 4, 4);
		if ((cktype = riff_cc_basic_type(sc->chunkcc))) {
			sc->is_basic = 1;
			sc->is_list = 0;
			sc->chunk_type = cktype;
		} else if ((cktype = riff_cc_list_type(sc->chunkcc))) {
			sc->is_basic = 0;
			sc->is_list = 1;
			sc->chunk_type = cktype;
		} else { /* desync/unrecognized chunk */
			/* We didn't see anything */
			return RIFF_CHUNK_UNRECOGNIZED;
		}
		ds->consumed += 8;
		return RIFF_CONSUME_MORE;
	}
}
