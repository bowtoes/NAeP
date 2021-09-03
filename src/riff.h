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

#ifndef PROCESS_WEM_H
#define PROCESS_WEM_H

#include <stdio.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

#include "fcc.h"

BRRCPPSTART

#define _enum_defs(_l_, _d_) riff_##_l_##_##_d_,
#define _riff_basic_types(_p_) \
    _p_(basic_type, data) \
    _p_(basic_type, cue) \
    _p_(basic_type, fmt) \
    _p_(basic_type, akd) \
    _p_(basic_type, vorb) \
    _p_(basic_type, JUNK) \
    _p_(basic_type, labl)
/* Simple data chunks */
typedef enum riff_basic_type {
	riff_basic_type_unrecognized = 0,
	_riff_basic_types(_enum_defs)
	riff_basic_type_count,
} riff_basic_typeT;
extern const brru4 riff_basic_types[riff_basic_type_count - 1];

#define _riff_list_types(_p_) \
    _p_(list_type, RIFF) \
    _p_(list_type, RIFX) \
    _p_(list_type, XFIR) \
    _p_(list_type, FFIR) \
    _p_(list_type, LIST)
/* Types with subchunks */
typedef enum riff_list_type {
	riff_list_type_unrecognized = 0,
	_riff_list_types(_enum_defs)
	riff_list_type_count,
} riff_list_typeT;
extern const brru4 riff_list_types[riff_list_type_count - 1];

#define _riff_sub_types(_p_) \
    _p_(sub_type, adtl)
/* LIST sub_types */
typedef enum riff_sub_type {
	riff_sub_type_unrecognized = 0,
	_riff_sub_types(_enum_defs)
	riff_sub_type_count,
} riff_sub_typeT;
extern const brru4 riff_sub_types[riff_sub_type_count - 1];

#define _riff_data_types(_p_) \
    _p_(data_type, WAVE)
/* RIFF file data types */
typedef enum riff_data_type {
	riff_data_type_unrecognized = 0,
	_riff_data_types(_enum_defs)
	riff_data_type_count,
} riff_data_typeT;
#undef _enum_defs
extern const brru4 riff_data_types[riff_data_type_count - 1];

#define _int_defs(_l_, _d_) extern const brru4 riff_##_l_##_int_##_d_;
#define _fcc_defs(_l_, _d_) extern const fourccT riff_##_l_##_fcc_##_d_;
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

#if !defined(_riff_keep_types_defines)
# undef _riff_basic_types
# undef _riff_list_types
# undef _riff_sub_types
# undef _riff_data_types
#endif

typedef enum riff_byteorder {
	riff_byteorder_unrecognized = 0,
	riff_byteorder_RIFF,
	riff_byteorder_RIFX,
	riff_byteorder_XFIR,
	riff_byteorder_FFIR,
} riff_byteorderT;
typedef struct riff_basic_chunk {
	riff_basic_typeT type; /* Type of chunk */
	brru4 size;  /* Size of chunk data in bytes */
	unsigned char *data; /* Chunk data, heap-allocated; freed through 'riff_clear' */
} riff_basic_chunkT;
typedef struct riff_list_chunk {
	riff_list_typeT type; /* Type of chunk */
	brru4 size; /* Size of chunk data in bytes */
	riff_sub_typeT sub_type; /* Subtype of the list */
	brru4 first_basic_index; /* Index in the riff struct of the first child chunk */
	brru4 n_basics; /* How many children the list has */
} riff_list_chunkT;
typedef struct riff_data_sync {
	unsigned char *data;
	brrs8 storage;
	brrs8 stored;
	brrs8 consumed;
	brrs8 list_end;

	void *(*BRRCALL cpy_cc)(void *const, const void *restrict const, size_t);
	void *(*BRRCALL move_cc)(void *const, const void *const, size_t);
	void *(*BRRCALL cpy_data)(void *restrict const, const void *restrict const, size_t);
	void *(*BRRCALL move_data)(void *const, const void *const, size_t);

	riff_byteorderT byteorder;
} riff_data_syncT;
typedef struct riff_chunk_info {
	int chunk_type; /* Which type of basic/list chunk is this? What it means depends on is_basic and is_list */
	int chunk_info_index; /* Final index position of the chunk in the riff struct */
	brru1 is_basic:1; /* Is the chunk basic? */
	brru1 is_list:1; /* Is the chunk a list? */
	brru4 chunkcc; /* Fourcc of the chunk */
	brru4 chunksize; /* Size of the chunk's data */
} riff_chunk_infoT;
typedef struct riff {
	riff_basic_chunkT *basics;
	riff_list_chunkT *lists;
	brru4 n_basics;
	brru4 n_lists;

	brru4 total_size;
	riff_data_typeT data_type;
} riffT;

#define RIFF_CHUNK_UNRECOGNIZED 2
#define RIFF_CHUNK_CONSUMED 1
#define RIFF_CHUNK_INCOMPLETE 0
#define RIFF_ERROR -1
#define RIFF_NOT_RIFF -2
#define RIFF_CONSUME_MORE -3
#define RIFF_CORRUPTED -4

void BRRCALL riff_chunk_info_clear(riff_chunk_infoT *const sc);

/* -1 : invalid/broken data_sync
 *  0 : valid data_sync
 * */
int BRRCALL riff_data_sync_check(const riff_data_syncT *const ds);
void BRRCALL riff_data_sync_clear(riff_data_syncT *const ds);
/* NULL : error
 * */
char *BRRCALL riff_data_sync_buffer(riff_data_syncT *const ds, brru4 size);
/* -1 : error
 *  0 : success
 * */
int BRRCALL riff_data_sync_apply(riff_data_syncT *const ds, brru4 size);

void BRRCALL riff_clear(riffT *const rf);
/* -1 : error
 *  0 : success
 * */
int BRRCALL riff_init(riffT *const rf);
/* -1 : invalid/broken riff
 *  0 : valid riff
 * */
int BRRCALL riff_check(const riffT *const rf);
/* -1 : error
 *  0 : not enough data
 *  1 : chunk consumed successfully
 * */
int BRRCALL riff_consume_chunk(riffT *const rf, riff_chunk_infoT *const sc, riff_data_syncT *const ds);

#define FMT_BASIC_FIELDS \
    brru2 format_tag; \
    brru2 n_channels; \
    brru4 samples_per_sec; \
    brru4 avg_byte_rate; \
    brru2 block_align
#define FMT_PCM_FIELDS \
    FMT_BASIC_FIELDS; \
    brru2 bits_per_sample
#define FMT_EXTENDED_FIELDS \
    FMT_PCM_FIELDS; \
    brru2 extra_size
typedef struct riff_fmt_basic {
	FMT_BASIC_FIELDS;
	brru2 padding;
} riff_fmt_basicT;
typedef struct riff_fmt_pcm {
	union {
		riff_fmt_basicT fmt_basic;
		struct {
			FMT_PCM_FIELDS;
		};
	};
} riff_fmt_pcmT;
typedef struct riff_fmt_extended {
	union {
		riff_fmt_basicT fmt_basic;
		riff_fmt_pcmT fmt_pcm;
		struct {
			FMT_PCM_FIELDS;
		};
	};
	brru2 cbSize;
	brru2 padding;
} riff_fmt_extendedT;
typedef struct riff_fmt_extensible {
	union {
		riff_fmt_basicT fmt_basic;
		riff_fmt_pcmT fmt_pcm;
		riff_fmt_extendedT fmt_extended;
		struct {
			FMT_EXTENDED_FIELDS;
			union {
				brru2 valid_pits_per_sample;
				brru2 samples_per_block;
				brru2 reserverd;
			};
		};
	};
	brru4 channel_mask;
	union {
		struct {
			brru4 guid_1;
			brru2 guid_2;
			brru2 guid_3;
			brru1 guid_4[8];
		};
		brru4 guid[4];
	};
} riff_fmt_extensibleT;
#undef FMT_EXTENDED_FIELDS
#undef FMT_PCM_FIELDS
#undef FMT_BASIC_FIELDS

typedef enum riff_instancetype {
	riff_instancetype_unrecognized = 0,
	riff_instancetype_fmt_basic,
	riff_instancetype_fmt_pcm,
	riff_instancetype_fmt_extended,
	riff_instancetype_fmt_extensible,
	riff_instancetype_count,
} riff_instancetypeT;


BRRCPPEND

#endif /* PROCESS_WEM_H */
