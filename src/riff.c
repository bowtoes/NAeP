/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#define _riff_keepsies
#include "riff.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "nelog.h"

#define RIFF_BUFF_EXTRA 4096
#define RIFF_BUFF_MAX INT_MAX

const fcc_t fcc_RIFF = fcc_str(,"RIFF");
const fcc_t fcc_RIFX = fcc_str(,"RIFX");
const fcc_t fcc_XFIR = fcc_str(,"XFIR");
const fcc_t fcc_FFIR = fcc_str(,"FFIR");

#define _array_processor(_type_, _cc_) fcc_str("    ",#_cc_),
#define _arrlen(_k_) (sizeof(_k_)/sizeof((_k_)[0]))
#define _getter_boiler(_name_,_subname_)\
	const fcc_t riff_##_name_##_ccs[riff_##_name_##_count - 1] = {\
		_riff_##_name_##s_gen(_array_processor)\
	};\
    riff_##_name_##_subname_##_t\
    riff_cc_##_name_##_subname_(fcc_t cc)\
	{\
    	for (riff_##_name_##_subname_##_t i = 0;i < _arrlen(riff_##_name_##_ccs); ++i) {\
    		if (!fcccmp(cc, riff_##_name_##_ccs[i]))\
    			return i + 1;\
    	}\
    	return 0;\
    }
_riff_boiler_gen(_getter_boiler)
#undef _getter_boiler
#undef _array_processor

static void*
cpy_rvs(void *restrict const dst, const void *restrict const src, brrsz size)
{
	memcpy(dst, src, size);
	for (brrsz i = 0; i < size / 2; ++i) {
		char a = ((char*)dst)[i];
		((char*)dst)[i] = ((char*)dst)[size - i - 1];
		((char*)dst)[size - i - 1] = a;
	}
	return dst;
}
static void*
move_rvs(void *const dst, const void *const src, size_t size)
{
	memmove(dst, src, size);
	for (brrsz i = 0; i < size / 2; ++i) {
		char a = ((char*)dst)[i];
		((char*)dst)[i] = ((char*)dst)[size - i - 1];
		((char*)dst)[size - i - 1] = a;
	}
	return dst;
}
#define straight memcpy
#define reverse cpy_rvs
const riff_copier_t _riff_copy_table[] = {
	           /*[<------ LE ------>][                ]*/
	           /*[                  ][<----- BE ----->]*/
	/* FourCC */ straight, straight, reverse,  reverse,
	/* Data   */ straight, reverse,  straight, reverse,
};

riff_byteorder_t
riff_cc_byteorder(fcc_t cc)
{
	for (riff_byteorder_t i = 0; i < 4; ++i) {
		if (!fcccmp(cc, riff_root_ccs[i]))
			return 1 + i;
	}
	return riff_byteorder_unrecognized;
}

int
riff_datasync_from_buffer(riff_datasync_t *const datasync, unsigned char *const buffer, brrsz buffer_size)
{
	if (riff_datasync_check(datasync)) {
		Pro(,"RIFF Bad datasync in datasync_from_buffer");
		return -1;
	}

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

int
riff_datasync_apply(riff_datasync_t *const datasync, brrsz size)
{
	if (riff_datasync_check(datasync))
		return -1;
	if (size + datasync->stored > datasync->storage)
		return -1;
	datasync->stored += size;
	return 0;
}

void
riff_datasync_clear(riff_datasync_t *const datasync)
{
	if (datasync) {
		memset(datasync, 0, sizeof(*datasync));
	}
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

void
riff_chunkstate_zero(riff_chunkstate_t *const chunkstate)
{
	if (chunkstate)
		memset(chunkstate, 0, sizeof(*chunkstate));
}

int
riff_check(const riff_t *const rf)
{
	if (!rf)
		return -1;
	return 0;
}

/* TODO rewrite these so they do as little pointer-dereference as possible */
static inline int
i_setup_riff(riff_t *const rf, riff_datasync_t *const datasync)
{
	const unsigned char *ckdata = datasync->data + datasync->consumed;
	brrsz stor = datasync->stored - datasync->consumed;
	if (stor < 12) { /* Not enough chunkcc, size, and formatcc */
		return riff_status_chunk_incomplete;
	}

	fcc_t rootcc = {.n=4};
	memcpy(&rootcc.v, ckdata, 4);
	if (!(datasync->byteorder = riff_cc_byteorder(rootcc))) {
		return riff_status_not_riff;
	}
	datasync->cpy_cc   = riff_copier_cc(datasync->byteorder);
	datasync->cpy_data = riff_copier_data(datasync->byteorder);

	datasync->cpy_data(&rf->total_size, ckdata + 4, 4);

	fcc_t formatcc = {.n=4};
	datasync->cpy_cc(&formatcc.v, ckdata + 8, 4);
	datasync->consumed += 12;
	rf->root = riff_cc_root(rootcc);
	rf->format = riff_cc_format(formatcc);
	return riff_status_consume_again;
}

static inline int
i_add_basic_type(riff_t *const rf, riff_datasync_t *const datasync, riff_basic_type_t cktype, brru4 cksize)
{
	const unsigned char *ckdata = datasync->data + datasync->consumed;
	riff_basic_chunk_t basic = {
		.offset = datasync->consumed,
		.type = cktype,
		.size = cksize,
		.data = malloc(cksize),
	};
	if (!basic.data)
		return riff_status_system_error;

	datasync->cpy_data(basic.data, ckdata, basic.size);
	datasync->consumed += basic.size;

	riff_basic_chunk_t *new = realloc(rf->basics, sizeof(*new) * (rf->n_basics + 1));
	if (!new)
		return riff_status_system_error;
	new[rf->n_basics++] = basic;
	rf->basics = new;

	if (datasync->list_end == BRRSZ_MAX) {
		/* List was corrupted, stream is too */
		return riff_status_corrupt;
	}

	if (datasync->list_end > 0) { /* Again, assuming lists can't have lists as subelements */
		riff_list_chunk_t *list = &rf->lists[rf->n_lists - 1];
		if (list->n_basics == 0)
			list->first_basic_index = rf->n_basics - 1;
		list->n_basics++;
		datasync->list_end -= basic.size + 8;
	}
	return riff_status_chunk_consumed;
}

static inline int
i_add_list_type(riff_t *const rf, riff_datasync_t *const datasync, brru4 cksize)
{
	const unsigned char *ckdata = datasync->data + datasync->consumed;
	riff_list_chunk_t list = {.offset=datasync->consumed};
	fcc_t formatcc = {.n=4};
	datasync->cpy_cc(&formatcc.v, ckdata, 4);
	datasync->consumed += 4;
	list.size = cksize;
	list.format = riff_cc_list_format(formatcc);
	list.first_basic_index = rf->n_basics;
	list.n_basics = 0;
	datasync->list_end = list.size - 4; /* list size includes formatcc */

	riff_list_chunk_t *new = realloc(rf->lists, sizeof(*new) * (rf->n_lists + 1));
	if (!new)
		return riff_status_system_error;
	new[rf->n_lists++] = list;
	rf->lists = new;

	return riff_status_chunk_consumed;
}

int
riff_consume_chunk(riff_t *const riff, riff_chunkstate_t *const chunkstate, riff_datasync_t *const datasync)
{
	if (riff_check(riff) || riff_datasync_check(datasync) || !chunkstate)
		return -1;

	const unsigned char *ckdata = datasync->data + datasync->consumed;
	brrs8 stor = datasync->stored - datasync->consumed;

	/* First chunk (RIFF) */
	if (!datasync->byteorder) {
		datasync->status = i_setup_riff(riff, datasync);
		return datasync->status == riff_status_chunk_consumed ? 0 : -1;

	/* Basic chunk */
	} else if (chunkstate->is_basic) {
		if (stor < chunkstate->chunksize) {
			// Not enough to fill out chunk
			datasync->status = riff_status_chunk_incomplete;
			return -1;
		} else {
			datasync->status = i_add_basic_type(riff, datasync, chunkstate->chunk_type, chunkstate->chunksize);
			if (datasync->status != riff_status_chunk_consumed)
				return -1;
		}
		chunkstate->chunk_index = riff->n_basics - 1;
		return 0;

	/* List chunk */
	} else if (chunkstate->is_list) {
		if (datasync->list_end > 0) {
			// I don't think lists can be subchunks of other lists
			datasync->status = riff_status_not_riff;
			return -1;
		}

		if (stor < 4) {
			// Not enough to get sub_type
			datasync->status = riff_status_chunk_incomplete;
			return -1;
		} else {
			datasync->status = i_add_list_type(riff, datasync, chunkstate->chunksize);
			if (datasync->status != riff_status_chunk_consumed)
				return -1;
		}
		chunkstate->chunk_index = riff->n_lists - 1;
		return 0;

	/* New chunk */
	} else {
 		/* Not enough for fcc + size */
		if (stor < 8) {
			/* Size can be 0, I'm not sure about fcc though */
			datasync->status = riff_status_chunk_incomplete;
			return -1;
		}

		chunkstate->chunkcc.n = 4;
		datasync->cpy_cc(&chunkstate->chunkcc.v, ckdata, 4);
		datasync->cpy_data(&chunkstate->chunksize, ckdata + 4, 4);

		int cktype = 0;
		if ((cktype = riff_cc_basic_type(chunkstate->chunkcc))) {
			chunkstate->is_basic = 1;
			chunkstate->is_list = 0;
			chunkstate->chunk_type = cktype;

		} else if ((cktype = riff_cc_list_type(chunkstate->chunkcc))) {
			chunkstate->is_basic = 0;
			chunkstate->is_list = 1;
			chunkstate->chunk_type = cktype;

		} else {
			/* desync/unrecognized chunk */
			/* We didn't see anything */
			datasync->status = riff_status_chunk_unrecognized;
			return -1;
		}
		datasync->consumed += 8;
		datasync->status = riff_status_consume_again;
		return -1;
	}
}
