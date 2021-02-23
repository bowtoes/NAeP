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
	NeSz c = 0;
	if (n == 0)
		return 1;
	if (n < 0)
		n = -(n + 1);
	for (; n > 0; ++c, n /= 10);
	return c;
}
