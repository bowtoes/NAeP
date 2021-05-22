#include "common/NeLibrary.h"

#include <stdlib.h> /* malloc() */
#include <string.h> /* memcmp() */
#include "common/NeDebugging.h"
#include "common/NeLogging.h"
#include "common/NeMisc.h"
#include "common/NeTypes.h"


/* should this allocation be automatically chunked? */
void *
NeSafeAlloc(void *cur, NeSz size, int zero)
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
		NeASSERTM(cur = calloc(1, size), "Failed to calloc %zu bytes : %m", size);
	} else if (!cur) {
		NeASSERTM(cur = malloc(size), "Failed to malloc %zu bytes : %m", size);
	} else {
		NeASSERTM(cur = realloc(cur, size), "Failed to realloc %zu bytes : %m", size);
	}

	return cur;
}

void
NeReverse(void *const buf, NeSz buflen)
{
	NeBy *const b = (NeBy *const)buf;
	if (!buf || !buflen)
		return;
	for (NeSz i = 0; i < buflen / 2; ++i)
		NeSWAP(b[i], b[buflen - 1 - i]);
}

static void
NeReverseElements(NeBy *const buf, NeSz elcount, NeSz elsize)
{
	if (!buf || !elcount || !elsize)
		return;
	for (NeSz i = 0; i < elcount / 2; ++i) {
		NeBy *a = NeIDX(buf, i, elsize);
		NeBy *b = NeIDX(buf, elcount - 1 - i, elsize);
		for (NeSz j = 0; j < elsize; ++j)
			NeSWAP(a[j], b[j]);
	}
}

NeSz
NeSlice(void *const dst, NeSz dstlen,
             const void *const src, NeSz srclen,
             NeOf start, NeOf end)
{
	NeSz i = 0;
	NeBy *const d = (NeBy *const)dst;
	const NeBy *const s = (const NeBy *const)src;
	/* can't copy any bytes */
	if (!src || !dst || !srclen || !dstlen)
		return NeERGINVALID;

	start = NeSmartMod(start, srclen, 1);
	end = NeSmartMod(end, srclen, 1);
	/* no length of bytes to copy */
	if (start == end)
		return NeERGNONE;

	if (start > end) {
		for (NeOf k = start - 1; k >= end && i < dstlen; --k, ++i)
			d[i] = s[k];
	} else {
		for (NeOf k = start; k < end && i < dstlen; ++k, ++i)
			d[i] = s[k];
	}
	return i;
}

NeSz
(NeCopy)(void *const dst, NeSz dstlen, const void *const src, NeSz srclen)
{
	NeSz mn = 0;
	if (!dst || !src || !dstlen || !srclen)
		return 0;
	mn = dstlen < srclen ? dstlen : srclen;
	memcpy(dst, src, mn);
	return mn;
}

NeOf
NeFind(const void *const hay, NeSz haysz,
        const void *const ndl, NeSz ndlsz, NeOf iof)
{
	NeBy cmp;
	const NeBy *const h = (const NeBy *const)hay,
	           *const n = (const NeBy *const)ndl;

	if (!hay || !ndl)
		return NeERGINVALID;
	if (!haysz || !ndlsz || ndlsz > haysz)
		return haysz;

	/* don't enter the last ndlsz - 1 bytes, they can't match */
	for (;iof < haysz - (ndlsz - 1); ++iof) {
		cmp = 1;
		for (NeOf i = 0; i < ndlsz; ++i)
			cmp &= n[i] == h[i + iof];
		if (cmp)
			break;
	}
	if (!cmp)
		return haysz;
	return iof;
}

NeOf
NeRfind(const void *const hay, NeSz haysz,
		const void *const ndl, NeSz ndlsz, NeOf iof)
{
	NeBy cmp;
	const NeBy *const h = (const NeBy *const)hay,
	           *const n = (const NeBy *const)ndl;

	if (!hay || !ndl)
		return NeERGINVALID;
	if (!haysz || !ndlsz || ndlsz > haysz)
		return haysz;

	for (;iof>=ndlsz-1;--iof) {
		cmp = 1;
		for (NeOf i = 0; i < ndlsz; ++i)
			cmp &= n[i] == h[i + iof];
		if (cmp)
			break;
	}
	if (!cmp)
		return haysz;
	return iof;
}
