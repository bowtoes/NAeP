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

#ifndef WSPBNK_H
#define WSPBNK_H

#include <brrtools/brrapi.h>
#include <brrtools/brrtypes.h>

#include "common_input.h"
#include "riff.h"

BRRCPPSTART

typedef struct wem_geom {
	brru4 offset;
	brru4 size;
	riff_byteorderT byteorder;
} wem_geomT;
typedef struct wsp {
	wem_geomT *wems;
	brru4 wem_count;
} wspT;

void BRRCALL wsp_clear(wspT *const wsp);

int BRRCALL wspbnk_extract(numbersT *const numbers, const char *const input, brrsz input_length,
    const input_optionsT *const options, codebook_libraryT *const library);

BRRCPPEND

#endif /* WSPBNK_H */
