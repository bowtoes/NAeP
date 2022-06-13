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

#ifndef NAeP_nepath_h
#define NAeP_nepath_h

#include <brrtools/brrpath.h>

#include "neutil.h"

struct nepath {
	const char *cstr;
	brrsz length;
	brrpath_stat_result_t st;
};

int
nepath_init(nepath_t *const path, const char *const arg);

int
nepath_read(const nepath_t *const path, void **const buffer);

/* Compares extension of 'path' to each given cstr (list terminated with NULL),
 * and returns the index of the first match, or -1 if no match.
 * If 'match' is not NULL, it's set to the pointer of the matching string, or NULL if no match. */
int
nepath_extension_cmp(const nepath_t *const path, const char **match, ...);

/* Changes the extension of 'path' to 'newext' if 'path' has an extension,
 * or adds the extension if it doesn't and stores the result in 'dst'.
 * 'dst' must have enough space to hold the new output. */
int
nepath_extension_replace(
    const nepath_t *restrict const path,
    const char *restrict const newext,
    int extlen,
    char *restrict const dst
);

#endif /* NAeP_nepath_h */
