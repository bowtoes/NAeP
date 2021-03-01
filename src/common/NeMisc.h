#ifndef NeMisc_h
#define NeMisc_h

#include "common/NeTypes.h"

/* https://graphics.stanford.edu/~seander/bithacks.html#SwappingBitsXOR */
#define NeSWAP(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

#define NeBITS8(d) \
			(((d) & 0x80) >> 7),\
			(((d) & 0x40) >> 6),\
			(((d) & 0x20) >> 5),\
			(((d) & 0x10) >> 4),\
			(((d) & 0x08) >> 3),\
			(((d) & 0x04) >> 2),\
			(((d) & 0x02) >> 1),\
			(((d) & 0x01) >> 0)
#define NeRBITS8(d) \
			(((d) & 0x01) >> 0),\
			(((d) & 0x02) >> 1),\
			(((d) & 0x04) >> 2),\
			(((d) & 0x08) >> 3),\
			(((d) & 0x10) >> 4),\
			(((d) & 0x20) >> 5),\
			(((d) & 0x40) >> 6),\
			(((d) & 0x80) >> 7)

/* Return 'in % mod', with overflow checking */
NeOf NeGCF(NeOf a, NeOf b);
NeOf NeSmartMod(NeOf in, NeSz mod, NeOf of);
NeSz NeDigitCount(NeOf n);

#endif /* NeMisc_h */
