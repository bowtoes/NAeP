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

#include "common/NeMisc.h"

brrof
NeGCF(brrof a, brrof b)
{
	if (a == b)
		return a;
	if (!a || !b)
		return 0;

	while (b != 0) {
		brrof t = b;
		b = a % t;
		a = t;
	}

	return a;
}
brrof
NeSmartMod(brrof in, brrsz mod, brrof of)
{
	if (mod == 0)
		return 0;
	if (in >= 0) {
		in = (brrof)((brrsz)in % mod);
	} else {
		/* https://stackoverflow.com/a/43295944/13528679 */
		brrsz t = (brrsz)(-(in + 1));
		in = (brrof)(mod - 1 - (t % mod)) + of;
	}
	return in;
}

brrsz
NeDigitCount(brrof n)
{
	brrsz c = 1;
	if (n == 0)
		return c;
	if (n < 0) {
		n = n == BRRTOOLS_BRROF_MIN ? -(n + 1) : -n;
	}
	while (n/=10) c++; /* teehee */
	return c;
}
