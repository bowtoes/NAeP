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

NeOf
NeGCF(NeOf a, NeOf b)
{
	if (a == b)
		return a;
	if (!a || !b)
		return 0;

	while (b != 0) {
		NeOf t = b;
		b = a % t;
		a = t;
	}

	return a;
}
NeOf
NeSmartMod(NeOf in, NeSz mod, NeOf of)
{
	if (mod == 0)
		return 0;
	if (in >= 0) {
		in = (NeOf)((NeSz)in % mod);
	} else {
		/* https://stackoverflow.com/a/43295944/13528679 */
		NeSz t = (NeSz)(-(in + 1));
		in = (NeOf)(mod - 1 - (t % mod)) + of;
	}
	return in;
}

NeSz
NeDigitCount(NeOf n)
{
	NeSz c = 1;
	if (n == 0)
		return c;
	if (n < 0) {
		n = n == NeOFMIN ? -(n + 1) : -n;
	}
	while (n/=10) c++; /* teehee */
	return c;
}
