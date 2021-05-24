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

#ifndef NeLibrary_h
#define NeLibrary_h

#include <brrtools/brrtypes.h>

#include "common/NeErrors.h"

/* get element (size elementsize) idx in data */
#define NeIDX(data, idx, elementsize) (((brrby *)(data)) + ((idx) * (elementsize)))

/* Should be used as maximum size alloced at once, though safe alloc doesn't
   check */
#define NeBLOCKSIZE 2048
/* If size == 0, free cur if necessary and return NULL */
/* if cur and zero, free cur and calloc */
/* if cur and not zero, realloc cur */
/* On error, hard crashes? what to do then? */
void *NeSafeAlloc(void *cur, brrsz size, int zero);

void NeReverse(void *const buf, brrsz buflen);

/* Copy a subset of src into dst from start to end */
/* If end is before start, the copied data is in backwards order */
/* Does wrap-around */
/* Very similar to python slicing */
/* Returns number of bytes copied */
brrsz NeSlice(void *const dst, brrsz dstlen,
             const void *const src, brrsz srclen,
             brrof start, brrof end);

//#define NeCopy(d, dl, s, sl) NeSlice(d, dl, s, sl, 0, -1)

/* Copies at most dstlen bytes from src into dst */
brrsz (NeCopy)(void *const dst, brrsz dstlen,
			const void *const src, brrsz srclen);
/* Find first offset of ndl in hay; return offset */
/* Returns -1 if error */
/* Retruns haysz if not found*/
/* To find every instance of 'ndl' in 'hay', do a loop like: */
/*  for (brrof of = 0; of >= 0 && (of = NeFind(h, hs, n, ns, of)) >= 0 && of < hs; of += ns) {*/
/*      // do stuff */
/*  } */
/* To do so backwards, with Rfind: */
/*  for (brrof of = hs - 1; of >= 0 && (of = NeRfind(h, hs, n, ns, of)) >= 0 && of < hs; of -= ns) { */
/*      // do stuff */
/*  } */
brrof NeFind(const void *const hay, brrsz haysz,
        const void *const ndl, brrsz ndlsz, brrof iof);
/* Same as NeFind, except starts from the back and searches backward */
/* iof is always relative to start of hay (0) */
brrof NeRfind(const void *const hay, brrsz haysz,
        const void *const ndl, brrsz ndlsz, brrof iof);

#endif /* NeLibrary_h */
