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

#ifndef RIFF_CONSUMER_H
#define RIFF_CONSUMER_H

#include <stdio.h>

#include <brrtools/brrtypes.h>

#if defined(RIFF_EXTENDED)
# include RIFF_EXTENDED
#endif

/* TODO this header is ridiculously messy and I think difficult to use */

#if !defined(RIFF_EXTENDED_BASICS)
# define RIFF_EXTENDED_BASICS(...)
#endif
#if !defined(RIFF_EXTENDED_LISTS)
# define RIFF_EXTENDED_LISTS(...)
#endif
#if !defined(RIFF_EXTENDED_LIST_FORMATS)
# define RIFF_EXTENDED_LIST_FORMATS(...)
#endif
#if !defined(RIFF_EXTENDED_ROOTS)
# define RIFF_EXTENDED_ROOTS(...)
#endif
#if !defined(RIFF_EXTENDED_FORMATS)
# define RIFF_EXTENDED_FORMATS(...)
#endif

#define _riff_basics_gen(_processor_) \
    _processor_(basic,cue) \
    _processor_(basic,data) \
    _processor_(basic,fmt) \
    _processor_(basic,JUNK) \
    _processor_(basic,labl) \
    RIFF_EXTENDED_BASICS(_processor_)

#define _riff_lists_gen(_processor_) \
    _processor_(list,LIST) \
    RIFF_EXTENDED_LISTS(_processor_)

#define _riff_list_formats_gen(_processor_) \
    _processor_(list_format,adtl) \
    RIFF_EXTENDED_LIST_FORMATS(_processor_)

#define _riff_roots_gen(_processor_) \
    _processor_(root,RIFF) \
    _processor_(root,RIFX) \
    _processor_(root,XFIR) \
    _processor_(root,FFIR) \
    RIFF_EXTENDED_ROOTS(_processor_)

#define _riff_formats_gen(_processor_) \
    _processor_(format,WAVE) \
    RIFF_EXTENDED_FORMATS(_processor_)

#define _riff_boiler_gen(_boiler_) \
    _boiler_(basic,_type) \
    _boiler_(list,_type) \
    _boiler_(list_format,) \
    _boiler_(root,) \
    _boiler_(format,) \

#define _enum_processor(_type_, _cc_) riff_##_type_##_##_cc_,
#define _enum_boiler(_t_,_t2_) \
    typedef enum riff_##_t_##_t2_ { \
    	riff_##_t_##_unrecognized = 0, \
    	_riff_##_t_##s_gen(_enum_processor) \
    	riff_##_t_##_count, \
    } riff_##_t_##_t2_##T; \
    extern const brru4 riff_##_t_##_ccs[riff_##_t_##_count - 1]; \
    riff_##_t_##_t2_##T riff_cc_##_t_##_t2_(brru4 cc);
_riff_boiler_gen(_enum_boiler)
#undef _enum_boiler
#undef _enum_processor

#if !defined(_riff_keepsies)
# undef _riff_basics_gen
# undef _riff_lists_gen
# undef _riff_roots_gen
# undef _riff_formats_gen
# undef _riff_boiler_gen
#endif

typedef void *(*riff_copier_t)(void *const, const void *const, size_t);

/* The byte-order of a RIFF file */
typedef enum riff_byteorder {
	riff_byteorder_unrecognized = 0,
	riff_byteorder_RIFF, /* Chunk character codes and data are stored little-endian */
	riff_byteorder_RIFX, /* Chunk character codes are stored little-endian and data is stored big-endian */
	riff_byteorder_XFIR, /* Chunk character codes are stored big-endian and data is stored little-endian */
	riff_byteorder_FFIR, /* Chunk character codes and data are stored big-endian */
} riff_byteorder_t;

/* Storage for the data of a basic riff chunk */
typedef struct riff_basic_chunk {
	unsigned char *data;   /* Chunk data, heap-allocated; freed through 'riff_clear' */
	riff_basic_typeT type; /* Type of chunk */
	brru4 size;            /* Size of chunk data in bytes */
} riff_basic_chunk_t;

/* A representation of a riff LIST chunk */
typedef struct riff_list_chunk {
	riff_list_formatT type;  /* Type of the list */
	brru4 size;              /* Size of list data in bytes */
	brru4 first_basic_index; /* Index in the riff struct of the first child chunk */
	brru4 n_basics;          /* How many children this list has */
} riff_list_chunkT;

/* Used to help reading chunks into a riff struct */
typedef struct riff_data_sync {
	unsigned char *data;
	brrsz storage;
	brrsz stored;
	brrsz consumed;
	brrsz list_end;

	riff_copier_t cpy_cc;
	riff_copier_t cpy_data;

	riff_byteorder_t byteorder;
} riff_data_sync_t;

/* Basic information about the chunk currently being read */
typedef struct riff_chunk_info {
	brru4 chunkcc;     /* Fourcc of the chunk */
	brru4 chunksize;   /* Size of the chunk's data */
	int chunk_type;    /* Which type of basic/list chunk is this? What it means depends on is_basic and is_list */
	brru4 chunk_index; /* Index position of the chunk in the riff struct */
	brru1 is_basic:1;  /* Whether the chunk is basic */
	brru1 is_list:1;   /* Whether the chunk is a list */
} riff_chunk_info_t;

/* Storage for riff chunks */
typedef struct riff {
	riff_basic_chunk_t *basics; /* Array of basic chunks */
	riff_list_chunkT *lists;   /* Array of LIST chunks */
	brrsz n_basics;            /* Number of chunks in the basics array */
	brrsz n_lists;             /* Number of LISTs in the lists array */

	brru4 total_size;          /* Total size of the RIFF structure */
	riff_rootT root;           /* Type of RIFF file root chunk */
	riff_formatT format;       /* Format specification of the RIFF file/data */
} riff_t;

riff_byteorder_t riff_cc_byteorder(brru4 cc);
riff_copier_t riff_copier_cc(riff_byteorder_t byteorder);
riff_copier_t riff_copier_data(riff_byteorder_t byteorder);

#define RIFF_CHUNK_UNRECOGNIZED 2
#define RIFF_CHUNK_CONSUMED 1
#define RIFF_CHUNK_INCOMPLETE 0
#define RIFF_ERROR -1
#define RIFF_CONSUME_MORE -2
#define RIFF_NOT_RIFF -3
#define RIFF_CORRUPTED -4

void riff_chunk_info_clear(riff_chunk_info_t *const sc);

void riff_data_sync_clear(riff_data_sync_t *const ds);
/* -1 : invalid/broken data sync
 *  0 : valid data_sync
 * */
int riff_data_sync_check(const riff_data_sync_t *const ds);
/* -1 : invalid arguments/data sync
 *  0 : success
 * */
int riff_data_sync_from_buffer(riff_data_sync_t *const ds,
    unsigned char *const buffer, brrsz buffer_size);
/* -1 : invalid offset/data sync
 *  0 : success
 * */
int riff_data_sync_seek(riff_data_sync_t *const ds, brrof offset);
/* NULL : error
 * */
char *riff_data_sync_buffer(riff_data_sync_t *const ds, brrsz size);
/* -1 : error
 *  0 : success
 * */
int riff_data_sync_apply(riff_data_sync_t *const ds, brrsz size);

void riff_clear(riff_t *const rf);
/* -1 : error
 *  0 : success
 * */
int riff_init(riff_t *const rf);
/* -1 : invalid/broken riff
 *  0 : valid riff
 * */
int riff_check(const riff_t *const rf);
/*
 * -4 : data corrupted
 * -3 : not riff
 * -2 : need to consume again
 * -1 : error
 *  0 : not enough data
 *  1 : chunk consumed successfully
 *  2 : chunk unrecognized
 * */
int riff_consume_chunk(riff_t *const rf, riff_chunk_info_t *const sc, riff_data_sync_t *const ds);

#endif /* RIFF_CONSUMER_H */
