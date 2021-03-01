#ifndef NeLibrary_h
#define NeLibrary_h

#include "common/NeTypes.h"

/* get element (size elementsize) idx in data */
#define NeIDX(data, idx, elementsize) (((NeBy *)(data)) + ((idx) * (elementsize)))

/* If size == 0, free cur if necessary and return NULL
 * if cur and zero, free cur and calloc
 * if cur and not zero, realloc cur
 * On error, hard crashes? what to do then? */
void *NeSafeAlloc(void *cur, NeSz size, int zero);

void NeReverse(void *const buf, NeSz buflen);

/* Copy a subset of src into dst from start to end
 * If end is before start, the copied data is in backwards order
 * Does wrap-around
 * Very similar to python slicing
 * Returns number of bytes copied */
NeSz NeSlice(void *const dst, NeSz dstlen,
             const void *const src, NeSz srclen,
             NeOf start, NeOf end);

//#define NeCopy(d, dl, s, sl) NeSlice(d, dl, s, sl, 0, -1)
/* Copies at most dstlen bytes from src into dst */
NeSz (NeCopy)(void *const dst, NeSz dstlen,
			const void *const src, NeSz srclen);
/* Find first offset of ndl in hay; return offset
 * Returns -1 if error
 * Retruns haysz if not found*/
/* To find every instance of 'ndl' in 'hay', do a loop like:
	for (NeOf of = 0; of >= 0 && (of = NeFind(h, hs, n, ns, of)) >= 0 && of < hs; of += ns) {
        // do stuff
    }
   To do so backwards, with Rfind:
	for (NeOf of = hs - 1; of >= 0 && (of = NeRfind(h, hs, n, ns, of)) >= 0 && of < hs; of -= ns) {
        // do stuff
    }
*/
NeOf NeFind(const void *const hay, NeSz haysz,
        const void *const ndl, NeSz ndlsz, NeOf iof);
/* Same as NeFind, except starts from the back and searches backward */
/* iof is always relative to start of hay (0) */
NeOf NeRfind(const void *const hay, NeSz haysz,
        const void *const ndl, NeSz ndlsz, NeOf iof);

#endif /* NeLibrary_h */
