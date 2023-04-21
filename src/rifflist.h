/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#ifndef NAeP_nerifflist_h
#define NAeP_nerifflist_h

#include <brrtools/brrtypes.h>

#include "riff.h"

/* A utility for scanning and converting riffs concatenated into a single file */

typedef struct riffgeometry {
	brrsz buffer_offset; /* Offset into the data where the RIFF fourcc starts */
	brru4 riff_size; /* Size of the RIFF chunk including fourcc and size */
	riff_byteorder_t byteorder;
} riffgeometry_t;

typedef struct rifflist {
	riffgeometry_t *riffs;
	brrsz n_riffs;
} rifflist_t;

/* Scans 'buffer' for all RIFF chunks contained within, and stores their position and size (geometry) in 'list'.
 * On error, leaves 'list' unaffected and returns an error code.
 * If a corrupted RIFF is found, scanning ceases and only the RIFFs up to the corrupted are stored.
 * */
int rifflist_scan(rifflist_t *const out_list, const unsigned char *const buffer, brrsz buffer_size);
void rifflist_clear(rifflist_t *const list);

typedef struct nestate nestate_t;
typedef struct neinput neinput_t;
typedef struct codebook_library codebook_library_t;
int rifflist_convert(
    const rifflist_t *const list,
    const unsigned char *const buffer,
    nestate_t *const state,
    const neinput_t *const input,
    const codebook_library_t *const library,
    const char *const output_root
);
int rifflist_extract(
    const rifflist_t *const list,
    const unsigned char *const buffer,
    nestate_t *const state,
    const neinput_t *const input,
    const char *const output_root
);

#endif /* NAeP_nerifflist_h */
