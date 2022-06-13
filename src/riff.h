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

#include <brrtools/brrendian.h>

#include "neutil.h"

#if defined(RIFF_EXTENDED)
# include RIFF_EXTENDED
#endif

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

#define _riff_basics_gen(_processor_)\
    _processor_(basic,cue)\
    _processor_(basic,data)\
    _processor_(basic,fmt)\
    _processor_(basic,JUNK)\
    _processor_(basic,labl)\
    RIFF_EXTENDED_BASICS(_processor_)

#define _riff_lists_gen(_processor_)\
    _processor_(list,LIST)\
    RIFF_EXTENDED_LISTS(_processor_)

#define _riff_list_formats_gen(_processor_)\
    _processor_(list_format,adtl)\
    RIFF_EXTENDED_LIST_FORMATS(_processor_)

#define _riff_roots_gen(_processor_)\
    _processor_(root,RIFF)\
    _processor_(root,RIFX)\
    _processor_(root,XFIR)\
    _processor_(root,FFIR)\
    RIFF_EXTENDED_ROOTS(_processor_)

#define _riff_formats_gen(_processor_)\
    _processor_(format,WAVE)\
    RIFF_EXTENDED_FORMATS(_processor_)

#define _riff_boiler_gen(_boiler_)\
    _boiler_(basic,_type)\
    _boiler_(list,_type)\
    _boiler_(list_format,)\
    _boiler_(root,)\
    _boiler_(format,)\

/* TODO I know these macros are a little ridiculous, but */

#define _enum_processor(_type_, _cc_) riff_ ##_type_## _ ##_cc_,
/* Typedefs the enum 'riff_ename(_subname)_t' with the names and values specified in the above '_riff_*_gen's.
 * Declares the function 'riff_cc_ename(_subname)(fcc_t cc)', which matches the given FourCC to a given
 * 'riff_ename(_subname)_t)', or returns 'riff_ename(_subname)_unrecognized' (0) if there is no match. */
#define _enum_boiler(_ename_,_subname_)\
    typedef enum riff_ ##_ename_##_subname_ {\
    	riff_ ##_ename_## _unrecognized = 0,\
    	_riff_ ##_ename_## s_gen(_enum_processor)\
    	riff_ ##_ename_## _count,\
    } riff_ ##_ename_##_subname_## _t;\
    extern const fcc_t riff_ ##_ename_## _ccs[riff_ ##_ename_## _count - 1];\
    riff_ ##_ename_##_subname_## _t riff_cc_ ##_ename_## _subname_(fcc_t cc);
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

/* The byte-order of a RIFF file */
typedef enum riff_byteorder {
	riff_byteorder_unrecognized = 0,
	riff_byteorder_RIFF, /* Chunk FourCCs and data are stored little-endian */
	riff_byteorder_RIFX, /* Chunk FourCCs are stored little-endian and data is stored big-endian */
	riff_byteorder_XFIR, /* Chunk FourCCs are stored big-endian and data is stored little-endian */
	riff_byteorder_FFIR, /* Chunk FourCCs and data are stored big-endian */
	riff_byteorder_count,
} riff_byteorder_t;

#if BRRENDIAN_SYSTEM == BRRENDIAN_BIG
# define _riff_copier_idx(_bo_) (4 - (_bo_))
#else
# define _riff_copier_idx(_bo_) ((_bo_) - 1)
#endif
extern const riff_copier_t _riff_copy_table[2 * (riff_byteorder_count-1)];

riff_byteorder_t
riff_cc_byteorder(fcc_t cc);
#define riff_copier_cc(_byte_order_) (_riff_copy_table[(_byte_order_)?_riff_copier_idx(_byte_order_):0])
#define riff_copier_data(_byte_order_) (_riff_copy_table[(_byte_order_)?4+_riff_copier_idx(_byte_order_):4])

/* 'chunkstate' and 'datasync' were inspired by libogg */

typedef enum riff_status {
	riff_status_chunk_consumed = 0, // Chunk was successfully consumed.
	riff_status_chunk_unrecognized, // Unrecognized chunk FourCC
	riff_status_chunk_incomplete,   // Insufficient data to complete the chunk.
	riff_status_consume_again,      // Need to consume more data to complete the chunk.
	riff_status_not_riff,           // Data is not RIFF.
	riff_status_system_error,       // System error (typically allocation failure).
	riff_status_corrupt,            // Chunk is corrupt.
} riff_status_t;

/* Used to help read chunks into a riff struct */
struct riff_datasync {
	unsigned char *data;
	brrsz storage;
	brrsz stored;
	brrsz consumed;
	brrsz list_end;

	riff_copier_t cpy_cc;
	riff_copier_t cpy_data;

	riff_byteorder_t byteorder;
	riff_status_t status;
};

/* Returns 0 for correct input, -1 for bad input. */
int
riff_datasync_check(const riff_datasync_t *const datasync);

/* Returns 0 on success or -1 on bad input. */
int
riff_datasync_from_buffer(riff_datasync_t *const datasync, unsigned char *const buffer, brrsz buffer_size);

/* Returns 0 on success or -1 on bad input. */
int
riff_datasync_seek(riff_datasync_t *const datasync, brrof offset);

/* Returns NULL on error. */
unsigned char *
riff_datasync_buffer(riff_datasync_t *const datasync, brrsz size);

/* Returns 0 on success or -1 on error. */
int
riff_datasync_apply(riff_datasync_t *const datasync, brrsz size);

void
riff_datasync_clear(riff_datasync_t *const datasync);

#define RIFF_CHUNK_TYPE_UNKNOWN 0
#define RIFF_CHUNK_TYPE_BASIC 1
#define RIFF_CHUNK_TYPE_LIST 2

/* Basic information about the chunk currently being read */
struct riff_chunkstate {
	fcc_t chunkcc;      /* Fourcc of the chunk */
	brru4 chunksize;    /* Size of the chunk's data */
	brru4 chunk_index;  /* Index position of the chunk in the riff struct */

	brru4 chunk_type:4; /* Which type of basic/list chunk is this? What it means depends on is_basic and is_list */
	brru4 is_basic:1;   /* Whether the chunk is basic */
	brru4 is_list:1;    /* Whether the chunk is a list */
};

void
riff_chunkstate_zero(riff_chunkstate_t *const chunkstate);

/* Storage for the data of a basic riff chunk */
struct riff_basic_chunk {
	riff_basic_type_t type; /* Type of chunk */
	brru4 size;             /* Size of chunk data in bytes */
	unsigned char *data;             /* Chunk data, heap-allocated; freed through 'riff_clear' */
};

/* A representation of a riff LIST chunk.
 * Stores only the index of the first chunk in the list, and the number of chunks in the list.
 * */
struct riff_list_chunk {
	riff_list_format_t format; /* Type of the list */
	brru4 size;                /* Size of list data in bytes */
	brru4 first_basic_index;   /* Index in the riff struct of the first child chunk */
	brru4 n_basics;            /* How many children this list has */
};

/* Storage for riff chunks */
struct riff {
	riff_basic_chunk_t *basics; /* Array of basic chunks */
	riff_list_chunk_t *lists;   /* Array of LIST chunks */
	brrsz n_basics;             /* Number of chunks in the basics array */
	brrsz n_lists;              /* Number of LISTs in the lists array */

	brru4 total_size;           /* Total size of the RIFF data */
	riff_root_t root;           /* Type of RIFF file root chunk */
	riff_format_t format;       /* Format specification of the RIFF file/data */
};

/* Returns 0 for correct input, -1 for bad input. */
int
riff_check(const riff_t *const riff);

/* Returns 0 on success or -1 on error. */
int
riff_init(riff_t *const riff);
/* Consumes single chunks from RIFF data in 'datasync' and store in into 'riff', with current chunk status stored in 'chunkstate'.
 * Returns 0 on when a chunk has been consumed or -1 otherwise.
 * Check 'datasync->status' to see why a chunk hasn't been consumed.
 * */
int
riff_consume_chunk(riff_t *const riff, riff_chunkstate_t *const chunkstate, riff_datasync_t *const datasync);

void
riff_clear(riff_t *const riff);

#endif /* RIFF_CONSUMER_H */
