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

#define _riff_keepsies
#include "riff.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrdata.h>
#include <brrtools/brrendian.h>

#include "lib.h"

#define RIFF_BUFF_EXTRA 4096
#define RIFF_BUFF_MAX INT_MAX

#define _array_processor(_type_, _cc_) FCC_GET_INT(#_cc_"    "),
#define _arrlen(_k_) (sizeof(_k_)/sizeof((_k_)[0]))
#define _getter_boiler(_t_,_t2_) \
	const brru4 riff_##_t_##_ccs[riff_##_t_##_count - 1] = { _riff_##_t_##s_gen(_array_processor) }; \
    riff_##_t_##_t2_##_t \
    riff_cc_##_t_##_t2_(brru4 cc) { \
    	riff_##_t_##_t2_##_t t = 0; \
    	for (;t < _arrlen(riff_##_t_##_ccs); ++t) { \
    		if (cc == riff_##_t_##_ccs[t]) \
    			return t + 1; \
    	} \
    	return 0; \
    }
_riff_boiler_gen(_getter_boiler)
#undef _getter_boiler
#undef _array_processor

riff_byteorder_t
riff_cc_byteorder(brru4 cc)
{
	for (riff_byteorder_t t = 0; t < 4; ++t) {
		if (cc == riff_root_ccs[t])
			return 1 + t;
	}
	return 0;
}

static inline void*
cpy_rvs(void *restrict const dst, const void *restrict const src, size_t size)
{
	memcpy(dst, src, size);
	brrdata_reverse_bytes(dst, size);
	return dst;
}
static inline void*
move_rvs(void *const dst, const void *const src, size_t size)
{
	memmove(dst, src, size);
	brrdata_reverse_bytes(dst, size);
	return dst;
}

#if BRRENDIAN_SYSTEM == BRRENDIAN_BIG
# define _riff_copier_idx(_bo_) (4 - (_bo_))
#else
# define _riff_copier_idx(_bo_) ((_bo_) - 1)
#endif

static const riff_copier_t _riff_copy_table[8] = {
	           /* ---> LE                       */
	           /*                       <--- BE */
	/* FourCC */ memcpy, memcpy,  cpy_rvs, cpy_rvs,
	/* Data   */ memcpy, cpy_rvs, memcpy,  cpy_rvs,
};
riff_copier_t
riff_copier_cc(riff_byteorder_t byteorder)
{
	if (!byteorder)
		return _riff_copy_table[0];
	return _riff_copy_table[0 + _riff_copier_idx(byteorder)];
}
riff_copier_t
riff_copier_data(riff_byteorder_t byteorder)
{
	if (!byteorder)
		return _riff_copy_table[4];
	return _riff_copy_table[4 + _riff_copier_idx(byteorder)];
}

void
riff_chunkstate_clear(riff_chunkstate_t *const chunkstate)
{
	if (chunkstate) {
		memset(chunkstate, 0, sizeof(*chunkstate));
	}
}

void
riff_datasync_clear(riff_datasync_t *const datasync)
{
	if (datasync) {
		if (datasync->data)
			free(datasync->data);
		memset(datasync, 0, sizeof(*datasync));
	}
}
int
riff_datasync_check(const riff_datasync_t *const datasync)
{
	if (!datasync || datasync->consumed > datasync->stored || datasync->stored > datasync->storage)
		return -1;
	return 0;
}
int
riff_datasync_from_buffer(riff_datasync_t *const datasync, unsigned char *const buffer, brrsz buffer_size)
{
	if (riff_datasync_check(datasync))
		return -1;
	if (!buffer || !buffer_size)
		return 0;
	datasync->data = buffer;
	datasync->stored = datasync->storage = buffer_size;
	return 0;
}
int
riff_datasync_seek(riff_datasync_t *const datasync, brrof offset)
{
	if (riff_datasync_check(datasync))
		return -1;
	if (offset > 0 && offset + datasync->consumed > datasync->stored)
		offset = -datasync->consumed + datasync->stored;
	else if (offset < 0 && -offset > datasync->consumed)
		offset = -datasync->consumed;
	datasync->consumed += offset;
	return 0;
}
char *
riff_datasync_buffer(riff_datasync_t *const datasync, brrsz size)
{
	if (riff_datasync_check(datasync))
		return NULL;

	if (datasync->consumed) {
		datasync->stored -= datasync->consumed;
		if (datasync->stored)
			memmove(datasync->data, datasync->data + datasync->consumed, datasync->stored);
		datasync->consumed = 0;
	}

	if (size > datasync->storage - datasync->stored) {
		brru4 new_size;
		if (size + datasync->stored > RIFF_BUFF_MAX - RIFF_BUFF_EXTRA)
			return NULL;
		new_size = datasync->stored + size + RIFF_BUFF_EXTRA;
		if (brrlib_alloc((void **)&datasync->data, new_size, 0))
			return NULL;
		datasync->storage = new_size;
	}
	return (char *)datasync->data + datasync->stored;
}
int
riff_datasync_apply(riff_datasync_t *const datasync, brrsz size)
{
	if (riff_datasync_check(datasync))
		return RIFF_ERROR;
	if (size + datasync->stored > datasync->storage)
		return RIFF_ERROR;
	datasync->stored += size;
	return 0;
}

void
riff_clear(riff_t *const rf)
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
int
riff_init(riff_t *const rf)
{
	if (rf)
		memset(rf, 0, sizeof(*rf));
	return 0;
}
int
riff_check(const riff_t *const rf)
{
	if (!rf)
		return RIFF_ERROR;
	return 0;
}

static inline int
i_setup_riff(riff_t *const rf, riff_datasync_t *const datasync)
{
	unsigned char *ckdata = datasync->data + datasync->consumed;
	brrsz stor = datasync->stored - datasync->consumed;
	brru4 formatcc, rootcc;
	if (stor < 12) { /* Not enough chunkcc, size, and formatcc */
		return RIFF_CHUNK_INCOMPLETE;
	}
	memcpy(&rootcc, ckdata, 4);
	if (!(datasync->byteorder = riff_cc_byteorder(rootcc))) {
		riff_datasync_clear(datasync);
		riff_clear(rf);
		return RIFF_NOT_RIFF;
	}
	datasync->cpy_cc   = riff_copier_cc(datasync->byteorder);
	datasync->cpy_data = riff_copier_data(datasync->byteorder);

	datasync->cpy_data(&rf->total_size, ckdata + 4, 4);
	datasync->cpy_cc(&formatcc, ckdata + 8, 4);
	datasync->consumed += 12;
	rf->root = riff_cc_root(rootcc);
	rf->format = riff_cc_format(formatcc);
	return RIFF_CONSUME_MORE;
}
static inline int
i_add_basic_type(riff_t *const rf, riff_datasync_t *const datasync, riff_basic_type_t cktype, brru4 cksize)
{
	unsigned char *ckdata = datasync->data + datasync->consumed;
	riff_basic_chunk_t basic = {0};
	basic.type = cktype;
	basic.size = cksize;
	if (brrlib_alloc((void **)&basic.data, basic.size, 1)) {
		return RIFF_ERROR;
	}
	datasync->cpy_data(basic.data, ckdata, basic.size);
	datasync->consumed += basic.size;
	if (brrlib_alloc((void **)&rf->basics, (rf->n_basics + 1) * sizeof(*rf->basics), 0)) {
		return RIFF_ERROR;
	}
	rf->basics[rf->n_basics++] = basic;
	if (datasync->list_end == BRRSZ_MAX) {
		/* List was corrupted, stream is too */
		return RIFF_CORRUPTED;
	}

	if (datasync->list_end > 0) { /* Again, assuming lists can't have lists as subelements */
		riff_list_chunk_t *list = &rf->lists[rf->n_lists - 1];
		if (list->n_basics == 0)
			list->first_basic_index = rf->n_basics - 1;
		list->n_basics++;
		datasync->list_end -= basic.size + 8;
	}
	return RIFF_CHUNK_CONSUMED;
}
static inline int
i_add_list_type(riff_t *const rf, riff_datasync_t *const datasync, brru4 cksize)
{
	unsigned char *ckdata = datasync->data + datasync->consumed;
	riff_list_chunk_t list = {0};
	brru4 formatcc;
	datasync->cpy_cc(&formatcc, ckdata, 4);
	datasync->consumed += 4;
	list.size = cksize;
	list.type = riff_cc_list_format(formatcc);
	list.first_basic_index = rf->n_basics;
	list.n_basics = 0;
	datasync->list_end = list.size - 4; /* list size includes formatcc*/
	if (brrlib_alloc((void **)&rf->lists, (rf->n_lists + 1) * sizeof(*rf->lists), 0)) {
		return RIFF_ERROR;
	}
	rf->lists[rf->n_lists++] = list;
	return RIFF_CHUNK_CONSUMED;
}
int
riff_consume_chunk(riff_t *const riff, riff_chunkstate_t *const chunk_state, riff_datasync_t *const datasync)
{
	if (riff_check(riff) || riff_datasync_check(datasync) || !chunk_state)
		return RIFF_ERROR;

	unsigned char *ckdata = datasync->data + datasync->consumed;
	brrs8 stor = datasync->stored - datasync->consumed;

	// The byteorder is unset, meaning we've only just started consumption
	// Initialize riff info from header chunk fcc and size
	if (!datasync->byteorder)
		return i_setup_riff(riff, datasync);

	if (chunk_state->is_basic) {
		// Not enough to fill out chunk
		if (stor < chunk_state->chunksize) {
			return RIFF_CHUNK_INCOMPLETE;
		} else {
			int err = i_add_basic_type(riff, datasync, chunk_state->chunk_type, chunk_state->chunksize);
			if (err != RIFF_CHUNK_CONSUMED)
				return err;
		}
		chunk_state->chunk_index = riff->n_basics - 1;
		return RIFF_CHUNK_CONSUMED;

	} else if (chunk_state->is_list) {
		if (datasync->list_end > 0) {
			// I don't think lists can be subchunks of other lists
			return RIFF_NOT_RIFF;
		}

		if (stor < 4) {
			// Not enough to get sub_type
			return RIFF_CHUNK_INCOMPLETE;
		} else {
			int err = i_add_list_type(riff, datasync, chunk_state->chunksize);
			if (err != RIFF_CHUNK_CONSUMED)
				return err;
		}
		chunk_state->chunk_index = riff->n_lists - 1;
		return RIFF_CHUNK_CONSUMED;

	} else {
		/* We don't know what type of chunk (basic or list) this is yet. */
		int cktype = 0;

 		/* Not enough for fcc + size */
		if (stor < 8) {
			/* Size can be 0, I'm not sure about fcc though */
			return RIFF_CHUNK_INCOMPLETE;
		}

		datasync->cpy_cc(&chunk_state->chunkcc, ckdata, 4);
		datasync->cpy_data(&chunk_state->chunksize, ckdata + 4, 4);

		if ((cktype = riff_cc_basic_type(chunk_state->chunkcc))) {
			chunk_state->is_basic = 1;
			chunk_state->is_list = 0;
			chunk_state->chunk_type = cktype;

		} else if ((cktype = riff_cc_list_type(chunk_state->chunkcc))) {
			chunk_state->is_basic = 0;
			chunk_state->is_list = 1;
			chunk_state->chunk_type = cktype;

		} else {
			/* desync/unrecognized chunk */
			/* We didn't see anything */
			return RIFF_CHUNK_UNRECOGNIZED;
		}
		datasync->consumed += 8;
		return RIFF_CONSUME_MORE;
	}
}
