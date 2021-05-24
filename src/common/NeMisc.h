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

#ifndef NeMisc_h
#define NeMisc_h

#include <brrtools/brrtypes.h>

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
#define NeTOGGLE(a) ((a)=!(a))

/* Return 'in % mod', with overflow checking */
brrof NeGCF(brrof a, brrof b);
brrof NeSmartMod(brrof in, brrsz mod, brrof of);
brrsz NeDigitCount(brrof n);

#endif /* NeMisc_h */
