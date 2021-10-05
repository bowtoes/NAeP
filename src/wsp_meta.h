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

#ifndef WSP_META_H
#define WSP_META_H

#include <brrtools/brrtypes.h>

#include "input.h"
#include "riff.h"

typedef struct wem_geometry {
	brru4 offset; /* Offset into the data where the RIFF fourcc starts */
	brru4 size; /* Size of the RIFF chunk including fourcc and size */
	riff_byteorderT byteorder;
} wem_geometryT;
typedef struct wsp_meta {
	wem_geometryT *wems;
	brru4 wem_count;
} wsp_metaT;

/* '*wsp' should be initalized to 0 */
int wsp_meta_init(wsp_metaT *const wsp, const char *const buffer, brrsz buffer_size);
void wsp_meta_clear(wsp_metaT *const wsp);

int wsp_meta_convert_wems(const wsp_metaT *const wsp, const char *const buffer,
    nestateT *const state, const neinputT *const input, const codebook_libraryT *const library,
    const char *const output_root);
int wsp_meta_extract_wems(const wsp_metaT *const wsp, const char *const buffer,
    nestateT *const state, const neinputT *const input,
    const char *const output_root);

#endif /* WSP_META_H */
