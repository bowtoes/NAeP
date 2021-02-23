#ifndef NeMisc_h
#define NeMisc_h

#include "common/NeTypes.h"

/* https://graphics.stanford.edu/~seander/bithacks.html#SwappingBitsXOR */
#define NeSWAP(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

/* Return 'in % mod', with overflow checking */
NeOf NeGCF(NeOf a, NeOf b);
NeOf NeSmartMod(NeOf in, NeSz mod, NeOf of);
NeSz NeDigitCount(NeOf n);

#endif /* NeMisc_h */
