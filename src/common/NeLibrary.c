#include "common/NeLibrary.h"

#include <stdlib.h> /* malloc() */
#include <string.h> /* memcmp() */
#include "common/NeDebugging.h"
#include "common/NeMisc.h"

NeSz
NeStrlen(const char *const str, NeSz max)
{
	NeSz i = 0;
	if (!str || !max)
		return 0;
	for (char a = str[0]; i < max && a != 0; ++i, a = str[i]);
	return i;
}

void *
NeSafeAlloc(void *cur, NeSz size, int zero)
{
	if (!size) {
		if (cur)
			free(cur);
		return NULL;
	}
	if (!cur) {
		if (zero) {
			if (!(cur = calloc(1, size)))
				NeTRACE("Failed to calloc %zu bytes : %m", size);
		} else {
			if (!(cur = malloc(size)))
				NeTRACE("Failed to malloc %zu bytes : %m", size);
		}
	} else {
		if (zero) {
			free(cur);
			if (!(cur = calloc(1, size)))
				NeTRACE("Failed to calloc %zu bytes : %m", size);
		} else {
			if (!(cur = realloc(cur, size)))
				NeTRACE("Failed to realloc %zu bytes : %m", size);
		}
	}

	return cur;
}

void
NeReverse(NeBy *const buf, NeSz buflen)
{
	if (!buf || !buflen)
		return;
	for (NeSz i = 0; i < buflen / 2; ++i)
		NeSWAP(buf[i], buf[buflen - 1 - i]);
}
void
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
NeCopy(NeBy *const dst, NeSz dstlen,
             const NeBy *const src, NeSz srclen,
             NeOf start, NeOf end)
{
	NeSz i = 0;
	/* can't copy any bytes */
	if (!src || !dst || !srclen || !dstlen)
		return 0;
	/* no length of bytes to copy */
	if (start == end)
		return 0;

	start = NeSmartMod(start, srclen, 1);
	end = NeSmartMod(end, srclen, 1);

	if (start > end) {
		for (NeOf k = start; k >= end && i < dstlen; --k, ++i)
			dst[i] = src[k];
	} else {
		for (NeOf k = start; k < end && i < dstlen; ++k, ++i)
			dst[i] = src[k];
	}
	return i;
}

NeOf
NeFind(const void *const hay, NeSz haysz,
        const void *const ndl, NeSz ndlsz, NeSz iof)
{
	NeOf of = iof;
	NeBy cmp;
	const NeBy *const h = (const NeBy *const)hay,
	           *const n = (const NeBy *const)ndl;

	if (!hay || !ndl)
		return -1;
	if (!haysz || !ndlsz || ndlsz > haysz)
		return haysz;

	/* don't check the last ndlsz - 1 bytes, they can't match */
	for (;of < haysz - (ndlsz - 1); ++of) {
		cmp = 1;
		for (NeSz i = 0; i < ndlsz; ++i)
			cmp &= n[i] == h[i + of - iof];
		if (cmp)
			break;
	}
	if (!cmp)
		return haysz;
	return of;
}

NeOf
NeRfind(const void *const hay, NeSz haysz,
		const void *const ndl, NeSz ndlsz, NeSz iofs)
{
	NeOf of = haysz - (ndlsz - 1) - 1;
	NeBy cmp;
	const NeBy *const h = (const NeBy *const)hay,
	           *const n = (const NeBy *const)ndl;

	if (!hay || !ndl)
		return -1;
	if (!haysz || !ndlsz || ndlsz > haysz)
		return haysz;

	for (;of >= 0; --of) {
		cmp = 1;
		for (NeSz i = 0; i < ndlsz; ++i)
			cmp &= n[i] == h[i + of - iofs];
		if (cmp)
			break;
	}
	if (!cmp)
		return haysz;
	return of;
}
