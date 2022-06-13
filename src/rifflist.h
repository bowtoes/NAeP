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

#ifndef NAeP_nerifflist_h
#define NAeP_nerifflist_h

#include <brrtools/brrtypes.h>

#include "neutil.h"
#include "riff.h"

struct riffgeometry {
	brrsz buffer_offset; /* Offset into the data where the RIFF fourcc starts */
	brru4 riff_size; /* Size of the RIFF chunk including fourcc and size */
	riff_byteorder_t byteorder;
};
struct rifflist {
	riffgeometry_t *riffs;
	brrsz n_riffs;
};

/* Scans 'buffer' for all RIFF chunks contained within, and stores their position and size (geometry) in 'list'.
 * On error, leaves 'list' unaffected and returns an error code.
 * If a corrupted RIFF is found, scanning ceases and only the RIFFs up to the corrupted are stored.
 * */
int rifflist_scan(rifflist_t *const out_list, const unsigned char *const buffer, brrsz buffer_size);
void rifflist_clear(rifflist_t *const list);

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
