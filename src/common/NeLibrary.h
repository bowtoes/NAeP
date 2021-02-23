#ifndef NeLibrary_h
#define NeLibrary_h

#include "common/NeTypes.h"

/* get element (size elementsize) idx in data */
#define NeIDX(data, idx, elementsize) (((NeBy *)(data)) + ((idx) * (elementsize)))
#define NeBLOCKSIZE 2048

/* Return length of str excluding null-terminator, up to max */
NeSz NeStrlen(const char *const str, NeSz max);

/* If size == 0, free cur if necessary and return NULL
 * if cur and zero, free cur and calloc
 * if cur and not zero, realloc cur
 * On error, hard crashes? what to do then? */
void *NeSafeAlloc(void *cur, NeSz size, int zero);

void NeReverse(NeBy *const buf, NeSz buflen);

/* Copy a subset of src into dst from start to end
 * If end is before start, the copied data is in backwards order
 * Does wrap-around
 * Returns number of bytes copied */
NeSz NeCopy(NeBy *const dst, NeSz dstlen,
             const NeBy *const src, NeSz srclen,
             NeOf start, NeOf end);

/* Find first offset of needle in haystack; return offset
 * Returns -1 on error, stksz if not found */
/* To find every instance of 'ndl' in 'hay', do a loop like:
 *  for (k = 0; k < hay_length; k += ndl_length) {
 *      k = NeFind(hay, hay_length, ndl, ndl_length, k);
 *      // if k == -1, error. if k == hay_length, ndl not present
 *  }*/
NeOf NeFind(const void *const hay, NeSz haysz,
        const void *const ndl, NeSz ndlsz, NeSz iof);
/* Same as NeFind, except starts from the back */
NeOf NeRfind(const void *const hay, NeSz haysz,
        const void *const ndl, NeSz ndlsz, NeSz iof);

#endif /* NeLibrary_h */
