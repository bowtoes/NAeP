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

#include "common/NeLibrary.h"

#include <stdlib.h>
#include <string.h>

#include <brrtools/brrdebug.h>
#include <brrtools/brrlog.h>
#include "common/NeMisc.h"


/* should this allocation be automatically chunked? */
void *
NeSafeAlloc(void *cur, brrsz size, int zero)
{
	if (!size) {
		if (cur) {
			free(cur);
		}
		return NULL;
	}
	if (zero) {
		if (cur) {
			free(cur);
		}
		BRRDEBUG_ASSERTM(cur = calloc(1, size), "Failed to calloc %zu bytes : %m", size);
	} else if (!cur) {
		BRRDEBUG_ASSERTM(cur = malloc(size), "Failed to malloc %zu bytes : %m", size);
	} else {
		BRRDEBUG_ASSERTM(cur = realloc(cur, size), "Failed to realloc %zu bytes : %m", size);
	}

	return cur;
}

void
NeReverse(void *const buf, brrsz buflen)
{
	brrby *const b = (brrby *const)buf;
	if (!buf || !buflen)
		return;
	for (brrsz i = 0; i < buflen / 2; ++i)
		NeSWAP(b[i], b[buflen - 1 - i]);
}

static void
NeReverseElements(brrby *const buf, brrsz elcount, brrsz elsize)
{
	if (!buf || !elcount || !elsize)
		return;
	for (brrsz i = 0; i < elcount / 2; ++i) {
		brrby *a = NeIDX(buf, i, elsize);
		brrby *b = NeIDX(buf, elcount - 1 - i, elsize);
		for (brrsz j = 0; j < elsize; ++j)
			NeSWAP(a[j], b[j]);
	}
}

brrsz
NeSlice(void *const dst, brrsz dstlen,
             const void *const src, brrsz srclen,
             brrof start, brrof end)
{
	brrsz i = 0;
	brrby *const d = (brrby *const)dst;
	const brrby *const s = (const brrby *const)src;
	/* can't copy any bytes */
	if (!src || !dst || !srclen || !dstlen)
		return NeERGINVALID;

	start = NeSmartMod(start, srclen, 1);
	end = NeSmartMod(end, srclen, 1);
	/* no length of bytes to copy */
	if (start == end)
		return NeERGNONE;

	if (start > end) {
		for (brrof k = start - 1; k >= end && i < dstlen; --k, ++i)
			d[i] = s[k];
	} else {
		for (brrof k = start; k < end && i < dstlen; ++k, ++i)
			d[i] = s[k];
	}
	return i;
}

brrsz
(NeCopy)(void *const dst, brrsz dstlen, const void *const src, brrsz srclen)
{
	brrsz mn = 0;
	if (!dst || !src || !dstlen || !srclen)
		return 0;
	mn = dstlen < srclen ? dstlen : srclen;
	memcpy(dst, src, mn);
	return mn;
}

brrof
NeFind(const void *const hay, brrsz haysz,
        const void *const ndl, brrsz ndlsz, brrof iof)
{
	brrby cmp;
	const brrby *const h = (const brrby *const)hay,
	           *const n = (const brrby *const)ndl;

	if (!hay || !ndl)
		return NeERGINVALID;
	if (!haysz || !ndlsz || ndlsz > haysz)
		return haysz;

	/* don't enter the last ndlsz - 1 bytes, they can't match */
	for (;iof < haysz - (ndlsz - 1); ++iof) {
		cmp = 1;
		for (brrof i = 0; i < ndlsz; ++i)
			cmp &= n[i] == h[i + iof];
		if (cmp)
			break;
	}
	if (!cmp)
		return haysz;
	return iof;
}

brrof
NeRfind(const void *const hay, brrsz haysz,
		const void *const ndl, brrsz ndlsz, brrof iof)
{
	brrby cmp;
	const brrby *const h = (const brrby *const)hay,
	           *const n = (const brrby *const)ndl;

	if (!hay || !ndl)
		return NeERGINVALID;
	if (!haysz || !ndlsz || ndlsz > haysz)
		return haysz;

	for (;iof>=ndlsz-1;--iof) {
		cmp = 1;
		for (brrof i = 0; i < ndlsz; ++i)
			cmp &= n[i] == h[i + iof];
		if (cmp)
			break;
	}
	if (!cmp)
		return haysz;
	return iof;
}
