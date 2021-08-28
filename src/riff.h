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
#define _riff_basictypes(_p_) \
    _p_(basictype, data) \
    _p_(basictype, cue) \
    _p_(basictype, fmt) \
    _p_(basictype, akd) \
    _p_(basictype, labl)
/* Simple data chunks */
typedef enum riff_basictype {
	riff_basictype_unrecognized = 0,
	_riff_basictypes(_enum_defs)
	riff_basictype_count,
} riff_basictypeT;
extern const brru4 riff_basictypes[riff_basictype_count - 1];

#define _riff_listtypes(_p_) \
    _p_(listtype, RIFF) \
    _p_(listtype, RIFX) \
    _p_(listtype, XFIR) \
    _p_(listtype, FFIR) \
    _p_(listtype, LIST)
/* Types with subchunks */
typedef enum riff_listtype {
	riff_listtype_unrecognized = 0,
	_riff_listtypes(_enum_defs)
	riff_listtype_count,
} riff_listtypeT;
extern const brru4 riff_listtypes[riff_listtype_count - 1];

#define _riff_subtypes(_p_) \
    _p_(subtype, adtl)
/* LIST subtypes */
typedef enum riff_subtype {
	riff_subtype_unrecognized = 0,
	_riff_subtypes(_enum_defs)
	riff_subtype_count,
} riff_subtypeT;
extern const brru4 riff_subtypes[riff_subtype_count - 1];

#define _riff_datatypes(_p_) \
    _p_(datatype, WAVE)
/* RIFF file data types */
typedef enum riff_datatype {
	riff_datatype_unrecognized = 0,
	_riff_datatypes(_enum_defs)
	riff_datatype_count,
} riff_datatypeT;
#undef _enum_defs
extern const brru4 riff_datatypes[riff_datatype_count - 1];

#define _int_defs(_l_, _d_) extern const brru4 riff_##_l_##_int_##_d_;
#define _fcc_defs(_l_, _d_) extern const fourccT riff_##_l_##_fcc_##_d_;
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

#if !defined(_riff_keep_types_defines)
# undef _riff_basictypes
# undef _riff_listtypes
# undef _riff_subtypes
# undef _riff_datatypes
#endif

typedef enum riff_byteorder {
	riff_byteorder_unrecognized = 0,
	riff_byteorder_RIFF,
	riff_byteorder_RIFX,
	riff_byteorder_XFIR,
	riff_byteorder_FFIR,
} riff_byteorderT;
typedef struct riff_basic_chunkinfo {
	riff_basictypeT type;
	brru4 size;
	unsigned char *data;
} riff_basic_chunkinfoT;
typedef struct riff_list_chunkinfo {
	riff_listtypeT type;
	brru4 size;
	riff_subtypeT subtype;
	brru4 first_basic_index;
	brru4 n_basics;
} riff_list_chunkinfoT;
typedef struct riff {
	unsigned char *data;
	brrs8 storage;
	brrs8 stored;
	brrs8 consumed;
	brrs8 list_end;

	void *(*BRRCALL cpy_cc)(void *const, const void *restrict const, size_t);
	void *(*BRRCALL move_cc)(void *const, const void *const, size_t);
	void *(*BRRCALL cpy_data)(void *restrict const, const void *restrict const, size_t);
	void *(*BRRCALL move_data)(void *const, const void *const, size_t);

	riff_basic_chunkinfoT *basics;
	riff_list_chunkinfoT *lists;
	brru4 n_basics;
	brru4 n_lists;

	brru4 total_size;
	riff_byteorderT byteorder;
	riff_datatypeT datatype;
} riffT;
typedef struct riff_chunkinfo {
	int chunk_type;    /* Which type of basic/list chunk is this? What it means depends on is_basic and is_list */
	int chunkinfo_index;
	brru1 is_basic:1;
	brru1 is_list:1;
	brru4 chunkcc;
	brru4 chunksize;
} riff_chunkinfoT;
void BRRCALL riff_chunkinfo_clear(riff_chunkinfoT *const fo);

#define RIFF_CHUNK_UNRECOGNIZED 2
#define RIFF_CHUNK_CONSUMED 1
#define RIFF_CHUNK_INCOMPLETE 0
#define RIFF_ERROR -1
#define RIFF_NOT_RIFF -2
#define RIFF_CONSUME_MORE -3
#define RIFF_CORRUPTED -4

/* -1 : error
 *  0 : success
 * */
int BRRCALL riff_init(riffT *const rf);
void BRRCALL riff_clear(riffT *const rf);
/* -1 : invalid/broken riff
 *  0 : valid riff
 * */
int BRRCALL riff_check(riffT *const rf);
/* NULL : error
 * */
char *BRRCALL riff_buffer(riffT *const rf, brru4 size);
/* -1 : error
 *  0 : success
 * */
int BRRCALL riff_apply_buffer(riffT *const rf, brru4 size);
/* -1 : error
 *  0 : not enough data
 *  1 : chunk consumed successfully
 * */
int BRRCALL riff_consume_chunk(riffT *const rf, riff_chunkinfoT *const fo);

#define FMT_BASIC_FIELDS    brru2 format_tag; brru2 n_channels; brru4 samples_per_sec; brru4 avg_bytes_per_sec; brru2 block_align
#define FMT_PCM_FIELDS      FMT_BASIC_FIELDS; brru2 bits_per_sample
#define FMT_EXTENDED_FIELDS FMT_PCM_FIELDS; brru2 cbSize
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
