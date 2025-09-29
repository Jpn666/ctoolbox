/*
 * Copyright (C) 2023, jpn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctoolbox/str2flt.h>
#include <ctoolbox/ctype.h>


#define FLT32MODE 0
#define FLT64MODE 1

/* Float type parameters */
struct TFLTType {
	int32 sbits;  /* significand bits */
	int32 ebits;  /* exponent bits */
	int32 ebias;  /* exponent bias */
	int32 emask;  /* exponent mask */

	int32 minexponentroundtoeven;
	int32 maxexponentroundtoeven;
};


#define FLT32EXPBITS  8
#define FLT64EXPBITS 11

static const struct TFLTType flttype[] = {
	{23, FLT32EXPBITS,  -127, (1l << FLT32EXPBITS) - 1, -17, 23},  /* flt32 */
	{52, FLT64EXPBITS, -1023, (1l << FLT64EXPBITS) - 1,  -4, 23}   /* flt64 */
};


#define FLTINF(MB, EB) (((1ull << ((EB) + 0)) - 1) << (MB))
#define FLTNAN(MB, EB) (((1ull << ((EB) + 1)) - 1) << (MB))

#define FLT32INF FLTINF(23,  8)
#define FLT32NAN FLTNAN(23,  8)
#define FLT64INF FLTINF(52, 11)
#define FLT64NAN FLTINF(52, 11)


/* */
struct TFltResult {
	int64 significand;
	int64 exponent;
};


/* ****************************************************************************
 * Simple decimal conversion ported from Golang decimal conversion
 * go/src/strconv/decimal.go
 * Using some optimizations from Wuff implementation:
 * https://github.com/google/wuffs/pull/87
 *
 * See also:
 * https://nigeltao.github.io/blog/2020/parse-number-f64-simple.html
 *************************************************************************** */

/* Mutiprecision decimal number */
struct TDecimal {
	int32 decimalpoint;
	int32 overflow;
	int32 n;

	/* Big-enddian representation */
	uint8 digits[800];
};

/* Maximum shift that we can do in one pass without overflow */
#define DECIMALMAXLSHIFT (32 - 6)
#define DECIMALMAXRSHIFT (64 - 4)


static void
decimaltrim(struct TDecimal* decimal)
{
	int32 j;

	for (j = decimal->n - 1; j; j--) {
		if (decimal->digits[j]) {
			break;
		}
	}
	decimal->n = j + 1;
}

static void
decimalrshift(struct TDecimal* decimal, uintxx amount)
{
	uint64 n;
	uint64 m;
	int32 r;
	int32 w;

	r = 0;
	w = 0;
	n = 0;
	while ((n >> amount) == 0) {
		if (r >= decimal->n) {
			if (n == 0) {
				decimal->n = 0;
				return;
			}
			while ((n >> amount) == 0) {
				n = n * 10;
				r++;
			}
			break;
		}
		n = n * 10 + decimal->digits[r++];
	}
	decimal->decimalpoint -= r - 1;

	m = (((uint64) 1) << amount) - 1;
	while (r < decimal->n) {
		uint32 c;

		c = decimal->digits[r++];
		decimal->digits[w++] = (uint8) (n >> amount);
		n = ((n & m) * 10) + c;
	}

	while (n) {
		if ((uint32) w < sizeof(decimal->digits)) {
			decimal->digits[w++] = (uint8) (n >> amount);
		}
		else {
			if (n >> amount) {
				decimal->overflow = 1;
			}
		}
		n = (n & m) * 10;
	}

	decimal->n = w;
	decimaltrim(decimal);
}


static const int32 lscheat[] = {
	0x00000001, 0x00020101,
	0x00060102, 0x00080103,
	0x000e0203, 0x00120204,
	0x00180205, 0x001e0305,
	0x00260306, 0x002c0307,
	0x00360407, 0x003e0408,
	0x00480409, 0x0052040a,
	0x005c050a, 0x0068050b,
	0x0076050c, 0x0084060c,
	0x0090060d, 0x009e060e,
	0x00ac070e, 0x00bc070f,
	0x00ce0710, 0x00e00711,
	0x00f20811, 0x01060812,
	0x01180813, 0x012e0913,
	0x01420914, 0x01580915,
	0x016e0a15, 0x01860a16,
	0x019c0a17, 0x01b60a18,
	0x01d00b18, 0x01e80b19,
	0x02020b1a, 0x021c0c1a,
	0x02380c1b, 0x02560c1c,
	0x02740d1c, 0x02900d1d,
	0x02ae0d1e, 0x02cc0d1f,
	0x02ee0e1f, 0x030e0e20,
	0x03300e21, 0x03520f21,
	0x03760f22, 0x03980f23,
	0x03be1023, 0x03e21024,
	0x04081025, 0x042e1026,
	0x04541126, 0x047c1127,
	0x04a61128, 0x04d01228,
	0x04f81229, 0x0522122a,
	0x054c132a
};

static const uint8 lscheatdigits[] = {
	1, 0, 5, 0, 0, 0, 2, 5, 1, 2, 5, 0, 0, 0, 6, 2,
	5, 0, 3, 1, 2, 5, 0, 0, 1, 5, 6, 2, 5, 0, 7, 8,
	1, 2, 5, 0, 0, 0, 3, 9, 0, 6, 2, 5, 1, 9, 5, 3,
	1, 2, 5, 0, 0, 0, 9, 7, 6, 5, 6, 2, 5, 0, 4, 8,
	8, 2, 8, 1, 2, 5, 0, 0, 2, 4, 4, 1, 4, 0, 6, 2,
	5, 0, 1, 2, 2, 0, 7, 0, 3, 1, 2, 5, 6, 1, 0, 3,
	5, 1, 5, 6, 2, 5, 0, 0, 3, 0, 5, 1, 7, 5, 7, 8,
	1, 2, 5, 0, 0, 0, 1, 5, 2, 5, 8, 7, 8, 9, 0, 6,
	2, 5, 0, 0, 7, 6, 2, 9, 3, 9, 4, 5, 3, 1, 2, 5,
	3, 8, 1, 4, 6, 9, 7, 2, 6, 5, 6, 2, 5, 0, 1, 9,
	0, 7, 3, 4, 8, 6, 3, 2, 8, 1, 2, 5, 9, 5, 3, 6,
	7, 4, 3, 1, 6, 4, 0, 6, 2, 5, 0, 0, 4, 7, 6, 8,
	3, 7, 1, 5, 8, 2, 0, 3, 1, 2, 5, 0, 0, 0, 2, 3,
	8, 4, 1, 8, 5, 7, 9, 1, 0, 1, 5, 6, 2, 5, 0, 0,
	1, 1, 9, 2, 0, 9, 2, 8, 9, 5, 5, 0, 7, 8, 1, 2,
	5, 0, 5, 9, 6, 0, 4, 6, 4, 4, 7, 7, 5, 3, 9, 0,
	6, 2, 5, 0, 0, 0, 2, 9, 8, 0, 2, 3, 2, 2, 3, 8,
	7, 6, 9, 5, 3, 1, 2, 5, 1, 4, 9, 0, 1, 1, 6, 1,
	1, 9, 3, 8, 4, 7, 6, 5, 6, 2, 5, 0, 0, 0, 7, 4,
	5, 0, 5, 8, 0, 5, 9, 6, 9, 2, 3, 8, 2, 8, 1, 2,
	5, 0, 3, 7, 2, 5, 2, 9, 0, 2, 9, 8, 4, 6, 1, 9,
	1, 4, 0, 6, 2, 5, 0, 0, 1, 8, 6, 2, 6, 4, 5, 1,
	4, 9, 2, 3, 0, 9, 5, 7, 0, 3, 1, 2, 5, 0, 9, 3,
	1, 3, 2, 2, 5, 7, 4, 6, 1, 5, 4, 7, 8, 5, 1, 5,
	6, 2, 5, 0, 0, 0, 4, 6, 5, 6, 6, 1, 2, 8, 7, 3,
	0, 7, 7, 3, 9, 2, 5, 7, 8, 1, 2, 5, 2, 3, 2, 8,
	3, 0, 6, 4, 3, 6, 5, 3, 8, 6, 9, 6, 2, 8, 9, 0,
	6, 2, 5, 0, 0, 0, 1, 1, 6, 4, 1, 5, 3, 2, 1, 8,
	2, 6, 9, 3, 4, 8, 1, 4, 4, 5, 3, 1, 2, 5, 0, 0,
	5, 8, 2, 0, 7, 6, 6, 0, 9, 1, 3, 4, 6, 7, 4, 0,
	7, 2, 2, 6, 5, 6, 2, 5, 2, 9, 1, 0, 3, 8, 3, 0,
	4, 5, 6, 7, 3, 3, 7, 0, 3, 6, 1, 3, 2, 8, 1, 2,
	5, 0, 1, 4, 5, 5, 1, 9, 1, 5, 2, 2, 8, 3, 6, 6,
	8, 5, 1, 8, 0, 6, 6, 4, 0, 6, 2, 5, 7, 2, 7, 5,
	9, 5, 7, 6, 1, 4, 1, 8, 3, 4, 2, 5, 9, 0, 3, 3,
	2, 0, 3, 1, 2, 5, 0, 0, 3, 6, 3, 7, 9, 7, 8, 8,
	0, 7, 0, 9, 1, 7, 1, 2, 9, 5, 1, 6, 6, 0, 1, 5,
	6, 2, 5, 0, 0, 0, 1, 8, 1, 8, 9, 8, 9, 4, 0, 3,
	5, 4, 5, 8, 5, 6, 4, 7, 5, 8, 3, 0, 0, 7, 8, 1,
	2, 5, 0, 0, 9, 0, 9, 4, 9, 4, 7, 0, 1, 7, 7, 2,
	9, 2, 8, 2, 3, 7, 9, 1, 5, 0, 3, 9, 0, 6, 2, 5,
	4, 5, 4, 7, 4, 7, 3, 5, 0, 8, 8, 6, 4, 6, 4, 1,
	1, 8, 9, 5, 7, 5, 1, 9, 5, 3, 1, 2, 5, 0, 2, 2,
	7, 3, 7, 3, 6, 7, 5, 4, 4, 3, 2, 3, 2, 0, 5, 9,
	4, 7, 8, 7, 5, 9, 7, 6, 5, 6, 2, 5, 1, 1, 3, 6,
	8, 6, 8, 3, 7, 7, 2, 1, 6, 1, 6, 0, 2, 9, 7, 3,
	9, 3, 7, 9, 8, 8, 2, 8, 1, 2, 5, 0, 0, 0, 5, 6,
	8, 4, 3, 4, 1, 8, 8, 6, 0, 8, 0, 8, 0, 1, 4, 8,
	6, 9, 6, 8, 9, 9, 4, 1, 4, 0, 6, 2, 5, 0, 2, 8,
	4, 2, 1, 7, 0, 9, 4, 3, 0, 4, 0, 4, 0, 0, 7, 4,
	3, 4, 8, 4, 4, 9, 7, 0, 7, 0, 3, 1, 2, 5, 0, 0,
	1, 4, 2, 1, 0, 8, 5, 4, 7, 1, 5, 2, 0, 2, 0, 0,
	3, 7, 1, 7, 4, 2, 2, 4, 8, 5, 3, 5, 1, 5, 6, 2,
	5, 0, 7, 1, 0, 5, 4, 2, 7, 3, 5, 7, 6, 0, 1, 0,
	0, 1, 8, 5, 8, 7, 1, 1, 2, 4, 2, 6, 7, 5, 7, 8,
	1, 2, 5, 0, 0, 0, 3, 5, 5, 2, 7, 1, 3, 6, 7, 8,
	8, 0, 0, 5, 0, 0, 9, 2, 9, 3, 5, 5, 6, 2, 1, 3,
	3, 7, 8, 9, 0, 6, 2, 5, 1, 7, 7, 6, 3, 5, 6, 8,
	3, 9, 4, 0, 0, 2, 5, 0, 4, 6, 4, 6, 7, 7, 8, 1,
	0, 6, 6, 8, 9, 4, 5, 3, 1, 2, 5, 0, 0, 0, 8, 8,
	8, 1, 7, 8, 4, 1, 9, 7, 0, 0, 1, 2, 5, 2, 3, 2,
	3, 3, 8, 9, 0, 5, 3, 3, 4, 4, 7, 2, 6, 5, 6, 2,
	5, 0, 4, 4, 4, 0, 8, 9, 2, 0, 9, 8, 5, 0, 0, 6,
	2, 6, 1, 6, 1, 6, 9, 4, 5, 2, 6, 6, 7, 2, 3, 6,
	3, 2, 8, 1, 2, 5, 0, 0, 2, 2, 2, 0, 4, 4, 6, 0,
	4, 9, 2, 5, 0, 3, 1, 3, 0, 8, 0, 8, 4, 7, 2, 6,
	3, 3, 3, 6, 1, 8, 1, 6, 4, 0, 6, 2, 5, 0, 1, 1,
	1, 0, 2, 2, 3, 0, 2, 4, 6, 2, 5, 1, 5, 6, 5, 4,
	0, 4, 2, 3, 6, 3, 1, 6, 6, 8, 0, 9, 0, 8, 2, 0,
	3, 1, 2, 5, 5, 5, 5, 1, 1, 1, 5, 1, 2, 3, 1, 2,
	5, 7, 8, 2, 7, 0, 2, 1, 1, 8, 1, 5, 8, 3, 4, 0,
	4, 5, 4, 1, 0, 1, 5, 6, 2, 5, 0, 0, 2, 7, 7, 5,
	5, 5, 7, 5, 6, 1, 5, 6, 2, 8, 9, 1, 3, 5, 1, 0,
	5, 9, 0, 7, 9, 1, 7, 0, 2, 2, 7, 0, 5, 0, 7, 8,
	1, 2, 5, 0, 0, 0, 1, 3, 8, 7, 7, 7, 8, 7, 8, 0,
	7, 8, 1, 4, 4, 5, 6, 7, 5, 5, 2, 9, 5, 3, 9, 5,
	8, 5, 1, 1, 3, 5, 2, 5, 3, 9, 0, 6, 2, 5, 0, 0,
	6, 9, 3, 8, 8, 9, 3, 9, 0, 3, 9, 0, 7, 2, 2, 8,
	3, 7, 7, 6, 4, 7, 6, 9, 7, 9, 2, 5, 5, 6, 7, 6,
	2, 6, 9, 5, 3, 1, 2, 5, 3, 4, 6, 9, 4, 4, 6, 9,
	5, 1, 9, 5, 3, 6, 1, 4, 1, 8, 8, 8, 2, 3, 8, 4,
	8, 9, 6, 2, 7, 8, 3, 8, 1, 3, 4, 7, 6, 5, 6, 2,
	5, 0, 1, 7, 3, 4, 7, 2, 3, 4, 7, 5, 9, 7, 6, 8,
	0, 7, 0, 9, 4, 4, 1, 1, 9, 2, 4, 4, 8, 1, 3, 9,
	1, 9, 0, 6, 7, 3, 8, 2, 8, 1, 2, 5, 8, 6, 7, 3,
	6, 1, 7, 3, 7, 9, 8, 8, 4, 0, 3, 5, 4, 7, 2, 0,
	5, 9, 6, 2, 2, 4, 0, 6, 9, 5, 9, 5, 3, 3, 6, 9,
	1, 4, 0, 6, 2, 5, 0, 0
};

static int32
getdigitsdelta(struct TDecimal* decimal, uintxx amount)
{
	int32 j;
	int32 count;
	int32 delta;
	const uint8* ldgts;

	j = lscheat[amount];
	count = (j >> 0x00) & 0xff;
	delta = (j >> 0x08) & 0xff;
	ldgts = lscheatdigits + (j >> 0x10);

	for (j = 0; j < count; j++) {
		if (j >= decimal->n) {
			delta--;
			break;
		}
		if (decimal->digits[j] ^ ldgts[j]) {
			delta -= (decimal->digits[j] < ldgts[j]);
			break;
		}
	}
	return delta;
}

static void
decimallshift(struct TDecimal* decimal, uintxx amount)
{
	int32 r;
	int32 w;
	int32 delta;
	uint64 n;
	uint64 quotient;
	uint64 remainder;

	delta = getdigitsdelta(decimal, amount);
	r = decimal->n - 1;
	w = decimal->n + delta;

	n = 0;
	while (r >= 0) {
		n += ((uint64) decimal->digits[r--]) << amount;
		remainder = n - ((quotient = (n * 429496730) >> 32) * 10);
		w--;
		if (w < (int32) sizeof(decimal->digits)) {
			decimal->digits[w] = (uint8) remainder;
		}
		else {
			if (remainder) {
				decimal->overflow = 1;
			}
		}
		n = quotient;
	}

	while (n > 0) {
		remainder = n - ((quotient = (n * 429496730) >> 32) * 10);
		w--;
		if (w < (int32) sizeof(decimal->digits)) {
			decimal->digits[w] = (uint8) remainder;
		}
		else {
			if (remainder) {
				decimal->overflow = 1;
			}
		}
		n = quotient;
	}

	decimal->n += delta;
	if ((uint32) decimal->n >= sizeof(decimal->digits)) {
		decimal->n = sizeof(decimal->digits);
	}
	decimal->decimalpoint += delta;
	decimaltrim(decimal);
}

static void
decimalshift(struct TDecimal* decimal, intxx amount)
{
	if (amount > 0) {
		while (amount > +DECIMALMAXLSHIFT) {
			decimallshift(decimal, DECIMALMAXLSHIFT);
			amount -= DECIMALMAXLSHIFT;
		}
		if (amount) {
			decimallshift(decimal, (uintxx) +amount);
		}
		return;
	}

	if (amount < 0) {
		while (amount < -DECIMALMAXRSHIFT) {
			decimalrshift(decimal, DECIMALMAXRSHIFT);
			amount += DECIMALMAXRSHIFT;
		}
		if (amount) {
			decimalrshift(decimal, (uintxx) -amount);
		}
		return;
	}
}

static int32
shouldroundup(struct TDecimal* decimal, int32 n)
{
	uint32 c;

	if (n < 0 || n >= (int32) decimal->n) {
		return 0;
	}
	c = (uint32) n;
	if (decimal->digits[c] == 5 && c + 1 == (uint32) decimal->n) {
		if (decimal->overflow) {
			return 0;
		}
		return c > 0 && (decimal->digits[c - 1] & 1);
	}
	return decimal->digits[n] >= 5;
}

static uint64
decimalroundinteger(struct TDecimal* decimal)
{
	int32 i;
	uint64 n;

	if (decimal->decimalpoint > 20) {
		return 0xffffffffffffffffull;
	}

	n = 0;
	i = 0;
	while (i < decimal->decimalpoint && i < (int32) decimal->n) {
		n = (n * 10) + decimal->digits[i++];
	}
	while (i < decimal->decimalpoint) {
		n = (n * 10); i++;
	}

	if (shouldroundup(decimal, decimal->decimalpoint)) {
		n++;
	}
	return n;
}

static void
parsedecimal(const uint8* s, int32 total, int32 e10, struct TDecimal* r)
{
	int32 n;

	r->overflow = 0;
	r->n        = 0;

	n = total;
	for (n = total; n; s++) {
		uint32 c;

		c = s[0];
		if (ctb_isdigit(c)) {
			if (sizeof(r->digits) > (uint32) r->n) {
				r->digits[r->n++] = (uint8) (c - 0x30);
			}
			else {
				if (c ^ 0x30) {
					r->overflow = 1;
				}
			}
			n--;
		}
	}
	r->decimalpoint = total + e10;
	decimaltrim(r);
}


static struct TFltResult
decimaltobinary(struct TDecimal* decimal, const struct TFLTType* f)
{
	int32 n;
	int32 additionallshift;
	int32  exp;
	uint64 snd;
	struct TFltResult r;

	/* Decimal powers of 10 to powers of 2 */
	const int32 pow10to2[] = {
		0, 3, 6, 9, 13, 16, 19, 23, 26, 29, 33, 36, 39, 43, 46, 49, 53, 56, 59
	};

	/* Zero */
	if (decimal->n == 0 || (decimal->n == 1 && decimal->digits[0] == 0)) {
		return (struct TFltResult) {0, 0};
	}

	/* Underflow */
	if (decimal->decimalpoint < -324) {
		return (struct TFltResult) {0, 0};
	}

	/* Overflow */
	if (decimal->decimalpoint > +311) {
		return (struct TFltResult) {0, f->emask};
	}

	/* Scale by powers of 2 until it's in range [0.1, 10.0) */
	exp = 0;
	while (decimal->decimalpoint > 1) {
		n = 60;

		if (+decimal->decimalpoint < 19)
			n = pow10to2[+decimal->decimalpoint];
		decimalshift(decimal, -n);
		if (decimal->decimalpoint < -2047) {
			return (struct TFltResult) {0, 0};
		}
		exp += n;
	}

	while (decimal->decimalpoint < 0) {
		n = 60;

		if (-decimal->decimalpoint < 19)
			n = pow10to2[-decimal->decimalpoint] + 1;
		decimalshift(decimal, +n);
		if (decimal->decimalpoint > +2047) {
			return (struct TFltResult) {0, f->emask};
		}
		exp -= n;
	}

	/* The mantissa in the range [0.1, 10) we need to scale in to the
	 * range [2, 1) */
	n = 100 * decimal->digits[0];
	if (decimal->n >= 3) {
		n += 10 * decimal->digits[1] + decimal->digits[2];
	}
	else {
		if (decimal->n > 1) {
			n += 10 * decimal->digits[1];
			if (decimal->n > 2) {
				n += decimal->digits[2];
			}
		}
	}

	additionallshift = 0;
	if (decimal->decimalpoint == 0) {
		/* The mantissa is in the range [.1 .. 1) we need to scale it up */
		if (n < 125) {
			additionallshift = 4;
		}
		else {
			if (n < 250) {
				additionallshift = 3;
			}
			else {
				if (n < 500) {
					additionallshift = 2;
				}
				else {
					additionallshift = 1;
				}
			}
		}
	}
	else {
		/* The mantissa is in the range [1 .. 10) we need to scale it down */
		if (n < 200) {
			additionallshift = 0;
		}
		else {
			if (n < 400) {
				additionallshift = -1;
			}
			else {
				if (n < 800) {
					additionallshift = -2;
				}
				else {
					additionallshift = -3;
				}
			}
		}
	}

	exp -= additionallshift;
	additionallshift += f->sbits;

	/* Minimum representable exponent is bias + 1
	 * If the exponent is smaller, move it up and adjust the mantissa
	 * accordingly */
	if (exp < f->ebias + 1) {
		n = f->ebias + 1 - exp;
		decimalshift(decimal, -n);
		exp += n;
	}

	/* Check for overflow */
	if (exp - f->ebias >= f->emask) {
		return (struct TFltResult) {0, f->emask};
	}

	/* Extract 53 bits for the mantissa (in base-2) */
	decimalshift(decimal, additionallshift);
	snd = decimalroundinteger(decimal);

	/* Rounding might have added a bit */
	if ((snd >> (f->sbits + 1)) != 0) {
		snd = snd >> 1;
		exp++;
		if (exp - f->ebias >= f->emask) {
			return (struct TFltResult) {0, f->emask};
		}
	}

	/* Subnormal number */
	if ((snd >> f->sbits) == 0) {
		exp = f->ebias;
	}

	r.significand = (int64) ((snd)            & ((1ull << f->sbits) - 1));
	r.exponent    = (int64) ((exp - f->ebias) & (f->emask));
	return r;
}

/* ****************************************************************************
 * Eisel-Lemire algorithm, "Number Parsing at a Gigabyte per Second" by
 * Daniel Lemire
 *
 * Ported from:
 * https://github.com/fastfloat/fast_float
 *************************************************************************** */

/*  */
struct TVal128 {
	uint64 lo;
	uint64 hi;
};


static struct TVal128
mul64to128(uint64 lhs, uint64 rhs)
{
	struct TVal128 r;

#if defined(__GNUC__) && defined(__SIZEOF_INT128__)
	__uint128_t product;

	product = ((__uint128_t) lhs) * ((__uint128_t) rhs);
	r.lo = (uint64) (product >> 00);
	r.hi = (uint64) (product >> 64);
#else
	/*
	 * Taken from xxHash3
	 * See also: https://stackoverflow.com/a/58381061 */
	uint64 loxlo;
	uint64 loxhi;
	uint64 hixlo;
	uint64 hixhi;
	uint64 upper;
	uint64 cross;

	/* first calculate all of the cross products. */
	loxlo = (lhs & 0xffffffff) * (rhs & 0xffffffff);
	hixlo = (lhs >> 32)        * (rhs & 0xffffffff);
	loxhi = (lhs & 0xffffffff) * (rhs >> 32);
	hixhi = (lhs >> 32)        * (rhs >> 32);

	/* now add the products together. These will never overflow. */
	cross = (loxlo >> 32) + (hixlo & 0xffffffff) + loxhi;
	upper = (hixlo >> 32) + (cross >> 32)        + hixhi;

	r.hi = upper;
	r.lo = (cross << 32) | (loxlo & 0xffffffff);
#endif
	return r;
}

static int32
countleadingzeros(uint64 n)
{
#if defined(__GNUC__)
	return __builtin_clzll(n);
#else
	static const unsigned char lztable[] = {
		0x08, 0x07, 0x06, 0x06, 0x05, 0x05, 0x05, 0x05,
		0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	uint32 r;
	uint32 m;
	uint32 a;
	uint32 b;

	r = 0;
	m = (uint32) (n >> 32);
	if (m == 0) {
		r += 32;
		m = (uint32) (n >> 00);
		if (m == 0) {
			return 64;
		}
	}

	a = lztable[(uint8) (m >> 0x18)];
	b = lztable[(uint8) (m >> 0x10)];
	if (a == 0x08) {
		r += 8;
	}
	else {
		return r + a;
	}
	if (b == 0x08) {
		r += 8;
	}
	else {
		return r + b;
	}

	a = lztable[(uint8) (m >> 0x08)];
	b = lztable[(uint8) (m >> 0x00)];
	if (a == 0x08) {
		r += 8;
	}
	else {
		return r + a;
	}
	if (b ^ 0x08) {
		return r + b;
	}
	return 64;
#endif
}


#define MINPOWEROF5 -342
#define MAXPOWEROF5 +308

static const uint64 (*powersoffive128)[2];


CTB_INLINE struct TVal128
computeapproximation(uint64 w, int64 q, uint64 precisionmask)
{
	struct TVal128 fstproduct;
	struct TVal128 sndproduct;
	int32 index;

	index = (int32) (q - MINPOWEROF5);

	fstproduct = mul64to128(w, powersoffive128[index][0]);
	if ((fstproduct.hi & precisionmask) == precisionmask) {
		/* Wider approximation */
		sndproduct = mul64to128(w, powersoffive128[index][1]);

		fstproduct.lo += sndproduct.hi;
		if (sndproduct.hi > fstproduct.lo) {
			fstproduct.hi++;
		}
	}
	return fstproduct;
}


#define POWER(Q) ((((152170 + 65536) * (Q)) >> 16) + 63)

static struct TFltResult
eisellemire(uint64 w, int32 q, const struct TFLTType* f)
{
	uint64 u;
	uint64 m;
	int64 p;
	int64 l;  /* The number of leading zeros of w */
	struct TVal128 z;

	if (q < MINPOWEROF5 || w == 0) {
		return (struct TFltResult) {0, 0};
	}

	if (q > MAXPOWEROF5) {
		return (struct TFltResult) {0, f->emask};
	}

	/* Normalize the decimal significand */
	w = w << (l = countleadingzeros(w));

	z = computeapproximation(w, q, 0xffffffffffffffffull >> (f->sbits + 3));
	if (z.lo == 0xffffffffffffffffull) {
		return (struct TFltResult) {-1ll, 0};
	}

	/* (upperbit) Value of the most significant bit of z */
	u = z.hi >> 63;

	/* The most significant 54 bits (64-bit) or 25 bits (32-bit) of the
	 * product z */
	m = z.hi >> (u + 64 - (uint64) f->sbits - 3);

	/* Expected binary exponent */
	p = POWER(q) - (l + (1 ^ (int64) u)) - f->ebias + 1;

	/* Subnormal number */
	if (p <= 0) {
		if (-p + 1 >= 64) {
			/* If we have more than 64 bits below the minimum exponent, you
			 * have a zero for sure */
			return (struct TFltResult) {0, 0};
		}
		m >>= -p + 1;
		m = m + (m & 1);  /* Round up */
		m = m >> 1;

		/* We need to check if rounding up has converted the subnormal
		 * into a normal number */
		if (m < 1ull << f->sbits) {
			p = 0;
		}
		else {
			p = 1;
		}
		goto L_DONE;
	}

	/* Check if we are between two floats */
	if (z.lo <= 1) {
		if (q >= f->minexponentroundtoeven && q <= f->maxexponentroundtoeven) {
			if ((m & 3) == 1) {
				if ((m << (u + 64 - (uint64) f->sbits - 3)) == z.hi) {
					/* If we fall right in between and and we have an
					 * even basis, we need to round down */
					m &= ~1ull;  /* Flip the last bit so we don't round up */
				}
			}
		}
	}

	m = (m + (m & 1)) >> 1;  /* Round up */
	if (m >= 2ull << f->sbits) {
		m = 1ull << f->sbits;
		p++;
	}

	m = m & ~(1ull << f->sbits);
	if (p >= f->emask) {
		/* Infinity */
		return (struct TFltResult) {0, f->emask};
	}

L_DONE:
	return (struct TFltResult) { (int64) m, p};
}

/* ****************************************************************************
 * Float parsing
 *************************************************************************** */

static int32
parsenan(const uint8* s, const struct TFLTType* f, uint64* r)
{
	if ((s[0] | 0x20) == 'i' && (s[1] | 0x20) == 'n' && (s[2] | 0x20) == 'f') {
		r[0] = FLTINF(f->sbits, f->ebits);

		if ((s[3] | 0x20) == 'i' &&
			(s[4] | 0x20) == 'n' &&
			(s[5] | 0x20) == 'i' &&
			(s[6] | 0x20) == 't' &&
			(s[7] | 0x20) == 'y') {
			return 8;
		}
		return 3;
	}
	if ((s[0] | 0x20) == 'n' && (s[1] | 0x20) == 'a' && (s[2] | 0x20) == 'n') {
		r[0] = FLTNAN(f->sbits, f->ebits);
		return 3;
	}
	return 0;
}

static int32
parseexponent(const uint8* s, int32* exponent)
{
	int32 i;
	int32 e;
	int32 isnegative;

	isnegative = 0;
	if (s[0] == 0x2d) {
		isnegative = 1;
		s++;
	}
	else {
		if (s[0] == 0x2b) {
			s++;
		}
	}

	i = 0;
	e = 0;
	while (ctb_isdigit(s[0])) {
		e = e * 10 + (s[0] - 0x30);
		if (e > 10000) {
			while (ctb_isdigit(s[0])) {
				s++;
				i++;
			}
			break;
		}
		s++;
		i++;
	}
	if (i == 0) {
		exponent[0] = 0;
		return 0;
	}

	if (isnegative)
		e = -e;
	exponent[0] = e;
	return i;
}


static uint64 tobinary(const uint8*, int32, uint64, int32, uintxx);

static eintxx
parsefloat(const uint8* src, const uint8** end, uintxx mode, uint64* result)
{
	int32 start;
	int32 total;
	uint32 c;
	int32 i;
	int32 isnegative;
	int32 decimalpoint;
	int32 e10;
	int32 zcountf;
	int32 zcounti;
	uint64 significand;
	eintxx r;
	const uint8* s;
	const struct TFLTType* f;

	s = src;

	/* Consume the white space */
	while (ctb_isspace(s[0])) {
		s++;
	}

	significand = 0;
	e10 = 0;  /* Exponent */
	decimalpoint = -1;

	/* Number of zeros after the decimal point if the integral part is zero */
	zcountf = 0;
	zcounti = 0;  /* Number of zeros before the decimal point */

	/* Check the sign */
	isnegative = 0;
	if (s[0] == 0x2d) {
		isnegative = 1;
		s++;
	}
	else {
		if (s[0] == 0x2b) {
			s++;
		}
	}

	f = flttype + mode;
	if (ctb_isdigit(s[0]) == 0) {
		if (s[0] == 0x2e) {
			decimalpoint = 0;
			s++;

			goto L1;
		}

		i = parsenan(s, f, result);
		if (i) {
			if (end) {
				end[0] = s + i;
			}
			return 0;
		}

		if (end) {
			end[0] = src;
		}

		result[0] = FLTNAN(f->sbits, f->ebits);
		return STR2FLT_ENAN;
	}

	/* 0s prefix */
	while (s[0] == 0x30) {
		zcounti++;
		s++;
	}

	if (s[0] == 0x2e) {
		decimalpoint = 0;
		s++;
	}

L1:
	/* */
	while (s[0] == 0x30) {
		zcountf++;
		s++;
	}

	start = (int32) (s - (const uint8*) src);

	/* Parse the first 19 digits */
	for (total = 0; total < 19; s++) {
		c = s[0];

		if (ctb_isdigit(c) == 0) {
			if (c == 0x2e) {
				if (decimalpoint ^ -1) {
					break;
				}

				decimalpoint = total;
				continue;
			}
			break;
		}
		significand = significand * 10 + (c - 0x30);
		total++;
	}

	/* */
	if (decimalpoint ^ -1) {
		while (ctb_isdigit(s[0])) {
			s++;
			total++;
		}
	}
	else {
		while (s[0]) {
			if (ctb_isdigit(s[0]) == 0) {
				if (s[0] == 0x2e) {
					if (decimalpoint ^ -1) {
						break;
					}

					decimalpoint = total;
					s++;
					continue;
				}
				break;
			}
			total++;
			s++;
		}
	}

	if (total == 0) {
		if (zcountf || zcounti) {
			result[0] = 0;

			/* Parse the exponent (if any) */
			if (s[0] == 0x45 || s[0] == 0x65) {
				int32 e[1];

				s++;
				s += (i = parseexponent(s, e));
				if (i == 0)
					s--;
			}

			if (end)
				end[0] = s;
			return 0;
		}

		if (end)
			end[0] = src;

		result[0] = FLTNAN(f->sbits, f->ebits);
		return STR2FLT_ENAN;
	}

	if (decimalpoint == -1) {
		decimalpoint = total;
	}

	/* We have an exponent */
	if (s[0] == 0x45 || s[0] == 0x65) {
		int32 e[1];

		s++;
		s += (i = parseexponent(s, e));
		if (i == 0)
			s--;
		e10 = e[0];
	}

	/* Normalize the exponent */
	e10 = e10 - (total - decimalpoint) - zcountf;

	result[0] = tobinary(src + start, total, significand, e10, mode);
	r = 0;
	switch (mode) {
		case FLT32MODE:
			if (result[0] == 0 || result[0] == FLT32INF) {
				r = STR2FLT_ERANGE;
			}
			if (isnegative)
				result[0] |= 1ull << ((sizeof(uint32) << 3) - 1);
			break;

		case FLT64MODE:
			if (result[0] == 0 || result[0] == FLT64INF) {
				r = STR2FLT_ERANGE;
			}
			if (isnegative)
				result[0] |= 1ull << ((sizeof(uint64) << 3) - 1);
			break;
	}
	if (end)
		end[0] = s;
	return r;
}

static uint64
tobinary(const uint8* start, int32 total, uint64 snd, int32 e10, uintxx mode)
{
	struct TFltResult r1;
	struct TFltResult r2;
	const struct TFLTType* f;

	f = flttype + mode;
	if (e10 == 0 && snd < (1ull << f->sbits)) {
		switch (mode) {
			case FLT32MODE: {
				union {
					flt32 f; uint32 u;
				}
				m;

				m.f = (flt32) snd;
				return (uint64) m.u;
			}
			break;

			case FLT64MODE: {
				union {
					flt64 f; uint64 u;
				}
				m;

				m.f = (flt64) snd;
				return (uint64) m.u;
			}
			break;
		}
	}

	if (total < 20) {
		r1 = eisellemire(snd, e10, f);
	}
	else {
		int32 ajustedexponent;

		ajustedexponent = (total + e10) - 19;
		r1 = eisellemire(snd + 0, ajustedexponent, f);
		r2 = eisellemire(snd + 1, ajustedexponent, f);

		if (r1.significand != r2.significand || r1.exponent != r2.exponent) {
			r1.significand = -1ll;
		}
		else {
			if (r2.significand == -1ll) {
				r1.significand = -1ll;
			}
		}
	}

	if (r1.significand == -1ll) {
		struct TDecimal decimal;

		parsedecimal(start, total, e10, &decimal);
		r1 = decimaltobinary(&decimal, f);
	}
	return (uint64) (r1.significand | (r1.exponent << f->sbits));
}


struct TToFltResult
str2flt64(const uint8* src, const uint8** end)
{
	struct TToFltResult result;
	uint64 u;
	union {
		flt64 f; uint64 u;
	}
	m;
	CTB_ASSERT(src);

	result.error = parsefloat(src, end, FLT64MODE, &u);

	m.u = u;
	result.value.asf64 = m.f;
	return result;
}

struct TToFltResult
str2flt32(const uint8* src, const uint8** end)
{
	struct TToFltResult result;
	uint64 u;
	union {
		flt32 f; uint32 u;
	}
	m;
	CTB_ASSERT(src);

	result.error = parsefloat(src, end, FLT32MODE, &u);

	m.u = (uint32) u;
	result.value.asf32 = m.f;
	return result;
}


/* ****************************************************************************
 * Tables
 *************************************************************************** */

static const uint64 powersoffive128_[][2] = {
	{0xeef453d6923bd65a, 0x113faa2906a13b3f},  /* -342 */
	{0x9558b4661b6565f8, 0x4ac7ca59a424c507},  /* -341 */
	{0xbaaee17fa23ebf76, 0x5d79bcf00d2df649},  /* -340 */
	{0xe95a99df8ace6f53, 0xf4d82c2c107973dc},  /* -339 */
	{0x91d8a02bb6c10594, 0x79071b9b8a4be869},  /* -338 */
	{0xb64ec836a47146f9, 0x9748e2826cdee284},  /* -337 */
	{0xe3e27a444d8d98b7, 0xfd1b1b2308169b25},  /* -336 */
	{0x8e6d8c6ab0787f72, 0xfe30f0f5e50e20f7},  /* -335 */
	{0xb208ef855c969f4f, 0xbdbd2d335e51a935},  /* -334 */
	{0xde8b2b66b3bc4723, 0xad2c788035e61382},  /* -333 */
	{0x8b16fb203055ac76, 0x4c3bcb5021afcc31},  /* -332 */
	{0xaddcb9e83c6b1793, 0xdf4abe242a1bbf3d},  /* -331 */
	{0xd953e8624b85dd78, 0xd71d6dad34a2af0d},  /* -330 */
	{0x87d4713d6f33aa6b, 0x8672648c40e5ad68},  /* -329 */
	{0xa9c98d8ccb009506, 0x680efdaf511f18c2},  /* -328 */
	{0xd43bf0effdc0ba48, 0x0212bd1b2566def2},  /* -327 */
	{0x84a57695fe98746d, 0x014bb630f7604b57},  /* -326 */
	{0xa5ced43b7e3e9188, 0x419ea3bd35385e2d},  /* -325 */
	{0xcf42894a5dce35ea, 0x52064cac828675b9},  /* -324 */
	{0x818995ce7aa0e1b2, 0x7343efebd1940993},  /* -323 */
	{0xa1ebfb4219491a1f, 0x1014ebe6c5f90bf8},  /* -322 */
	{0xca66fa129f9b60a6, 0xd41a26e077774ef6},  /* -321 */
	{0xfd00b897478238d0, 0x8920b098955522b4},  /* -320 */
	{0x9e20735e8cb16382, 0x55b46e5f5d5535b0},  /* -319 */
	{0xc5a890362fddbc62, 0xeb2189f734aa831d},  /* -318 */
	{0xf712b443bbd52b7b, 0xa5e9ec7501d523e4},  /* -317 */
	{0x9a6bb0aa55653b2d, 0x47b233c92125366e},  /* -316 */
	{0xc1069cd4eabe89f8, 0x999ec0bb696e840a},  /* -315 */
	{0xf148440a256e2c76, 0xc00670ea43ca250d},  /* -314 */
	{0x96cd2a865764dbca, 0x380406926a5e5728},  /* -313 */
	{0xbc807527ed3e12bc, 0xc605083704f5ecf2},  /* -312 */
	{0xeba09271e88d976b, 0xf7864a44c633682e},  /* -311 */
	{0x93445b8731587ea3, 0x7ab3ee6afbe0211d},  /* -310 */
	{0xb8157268fdae9e4c, 0x5960ea05bad82964},  /* -309 */
	{0xe61acf033d1a45df, 0x6fb92487298e33bd},  /* -308 */
	{0x8fd0c16206306bab, 0xa5d3b6d479f8e056},  /* -307 */
	{0xb3c4f1ba87bc8696, 0x8f48a4899877186c},  /* -306 */
	{0xe0b62e2929aba83c, 0x331acdabfe94de87},  /* -305 */
	{0x8c71dcd9ba0b4925, 0x9ff0c08b7f1d0b14},  /* -304 */
	{0xaf8e5410288e1b6f, 0x07ecf0ae5ee44dd9},  /* -303 */
	{0xdb71e91432b1a24a, 0xc9e82cd9f69d6150},  /* -302 */
	{0x892731ac9faf056e, 0xbe311c083a225cd2},  /* -301 */
	{0xab70fe17c79ac6ca, 0x6dbd630a48aaf406},  /* -300 */
	{0xd64d3d9db981787d, 0x092cbbccdad5b108},  /* -299 */
	{0x85f0468293f0eb4e, 0x25bbf56008c58ea5},  /* -298 */
	{0xa76c582338ed2621, 0xaf2af2b80af6f24e},  /* -297 */
	{0xd1476e2c07286faa, 0x1af5af660db4aee1},  /* -296 */
	{0x82cca4db847945ca, 0x50d98d9fc890ed4d},  /* -295 */
	{0xa37fce126597973c, 0xe50ff107bab528a0},  /* -294 */
	{0xcc5fc196fefd7d0c, 0x1e53ed49a96272c8},  /* -293 */
	{0xff77b1fcbebcdc4f, 0x25e8e89c13bb0f7a},  /* -292 */
	{0x9faacf3df73609b1, 0x77b191618c54e9ac},  /* -291 */
	{0xc795830d75038c1d, 0xd59df5b9ef6a2417},  /* -290 */
	{0xf97ae3d0d2446f25, 0x4b0573286b44ad1d},  /* -289 */
	{0x9becce62836ac577, 0x4ee367f9430aec32},  /* -288 */
	{0xc2e801fb244576d5, 0x229c41f793cda73f},  /* -287 */
	{0xf3a20279ed56d48a, 0x6b43527578c1110f},  /* -286 */
	{0x9845418c345644d6, 0x830a13896b78aaa9},  /* -285 */
	{0xbe5691ef416bd60c, 0x23cc986bc656d553},  /* -284 */
	{0xedec366b11c6cb8f, 0x2cbfbe86b7ec8aa8},  /* -283 */
	{0x94b3a202eb1c3f39, 0x7bf7d71432f3d6a9},  /* -282 */
	{0xb9e08a83a5e34f07, 0xdaf5ccd93fb0cc53},  /* -281 */
	{0xe858ad248f5c22c9, 0xd1b3400f8f9cff68},  /* -280 */
	{0x91376c36d99995be, 0x23100809b9c21fa1},  /* -279 */
	{0xb58547448ffffb2d, 0xabd40a0c2832a78a},  /* -278 */
	{0xe2e69915b3fff9f9, 0x16c90c8f323f516c},  /* -277 */
	{0x8dd01fad907ffc3b, 0xae3da7d97f6792e3},  /* -276 */
	{0xb1442798f49ffb4a, 0x99cd11cfdf41779c},  /* -275 */
	{0xdd95317f31c7fa1d, 0x40405643d711d583},  /* -274 */
	{0x8a7d3eef7f1cfc52, 0x482835ea666b2572},  /* -273 */
	{0xad1c8eab5ee43b66, 0xda3243650005eecf},  /* -272 */
	{0xd863b256369d4a40, 0x90bed43e40076a82},  /* -271 */
	{0x873e4f75e2224e68, 0x5a7744a6e804a291},  /* -270 */
	{0xa90de3535aaae202, 0x711515d0a205cb36},  /* -269 */
	{0xd3515c2831559a83, 0x0d5a5b44ca873e03},  /* -268 */
	{0x8412d9991ed58091, 0xe858790afe9486c2},  /* -267 */
	{0xa5178fff668ae0b6, 0x626e974dbe39a872},  /* -266 */
	{0xce5d73ff402d98e3, 0xfb0a3d212dc8128f},  /* -265 */
	{0x80fa687f881c7f8e, 0x7ce66634bc9d0b99},  /* -264 */
	{0xa139029f6a239f72, 0x1c1fffc1ebc44e80},  /* -263 */
	{0xc987434744ac874e, 0xa327ffb266b56220},  /* -262 */
	{0xfbe9141915d7a922, 0x4bf1ff9f0062baa8},  /* -261 */
	{0x9d71ac8fada6c9b5, 0x6f773fc3603db4a9},  /* -260 */
	{0xc4ce17b399107c22, 0xcb550fb4384d21d3},  /* -259 */
	{0xf6019da07f549b2b, 0x7e2a53a146606a48},  /* -258 */
	{0x99c102844f94e0fb, 0x2eda7444cbfc426d},  /* -257 */
	{0xc0314325637a1939, 0xfa911155fefb5308},  /* -256 */
	{0xf03d93eebc589f88, 0x793555ab7eba27ca},  /* -255 */
	{0x96267c7535b763b5, 0x4bc1558b2f3458de},  /* -254 */
	{0xbbb01b9283253ca2, 0x9eb1aaedfb016f16},  /* -253 */
	{0xea9c227723ee8bcb, 0x465e15a979c1cadc},  /* -252 */
	{0x92a1958a7675175f, 0x0bfacd89ec191ec9},  /* -251 */
	{0xb749faed14125d36, 0xcef980ec671f667b},  /* -250 */
	{0xe51c79a85916f484, 0x82b7e12780e7401a},  /* -249 */
	{0x8f31cc0937ae58d2, 0xd1b2ecb8b0908810},  /* -248 */
	{0xb2fe3f0b8599ef07, 0x861fa7e6dcb4aa15},  /* -247 */
	{0xdfbdcece67006ac9, 0x67a791e093e1d49a},  /* -246 */
	{0x8bd6a141006042bd, 0xe0c8bb2c5c6d24e0},  /* -245 */
	{0xaecc49914078536d, 0x58fae9f773886e18},  /* -244 */
	{0xda7f5bf590966848, 0xaf39a475506a899e},  /* -243 */
	{0x888f99797a5e012d, 0x6d8406c952429603},  /* -242 */
	{0xaab37fd7d8f58178, 0xc8e5087ba6d33b83},  /* -241 */
	{0xd5605fcdcf32e1d6, 0xfb1e4a9a90880a64},  /* -240 */
	{0x855c3be0a17fcd26, 0x5cf2eea09a55067f},  /* -239 */
	{0xa6b34ad8c9dfc06f, 0xf42faa48c0ea481e},  /* -238 */
	{0xd0601d8efc57b08b, 0xf13b94daf124da26},  /* -237 */
	{0x823c12795db6ce57, 0x76c53d08d6b70858},  /* -236 */
	{0xa2cb1717b52481ed, 0x54768c4b0c64ca6e},  /* -235 */
	{0xcb7ddcdda26da268, 0xa9942f5dcf7dfd09},  /* -234 */
	{0xfe5d54150b090b02, 0xd3f93b35435d7c4c},  /* -233 */
	{0x9efa548d26e5a6e1, 0xc47bc5014a1a6daf},  /* -232 */
	{0xc6b8e9b0709f109a, 0x359ab6419ca1091b},  /* -231 */
	{0xf867241c8cc6d4c0, 0xc30163d203c94b62},  /* -230 */
	{0x9b407691d7fc44f8, 0x79e0de63425dcf1d},  /* -229 */
	{0xc21094364dfb5636, 0x985915fc12f542e4},  /* -228 */
	{0xf294b943e17a2bc4, 0x3e6f5b7b17b2939d},  /* -227 */
	{0x979cf3ca6cec5b5a, 0xa705992ceecf9c42},  /* -226 */
	{0xbd8430bd08277231, 0x50c6ff782a838353},  /* -225 */
	{0xece53cec4a314ebd, 0xa4f8bf5635246428},  /* -224 */
	{0x940f4613ae5ed136, 0x871b7795e136be99},  /* -223 */
	{0xb913179899f68584, 0x28e2557b59846e3f},  /* -222 */
	{0xe757dd7ec07426e5, 0x331aeada2fe589cf},  /* -221 */
	{0x9096ea6f3848984f, 0x3ff0d2c85def7621},  /* -220 */
	{0xb4bca50b065abe63, 0x0fed077a756b53a9},  /* -219 */
	{0xe1ebce4dc7f16dfb, 0xd3e8495912c62894},  /* -218 */
	{0x8d3360f09cf6e4bd, 0x64712dd7abbbd95c},  /* -217 */
	{0xb080392cc4349dec, 0xbd8d794d96aacfb3},  /* -216 */
	{0xdca04777f541c567, 0xecf0d7a0fc5583a0},  /* -215 */
	{0x89e42caaf9491b60, 0xf41686c49db57244},  /* -214 */
	{0xac5d37d5b79b6239, 0x311c2875c522ced5},  /* -213 */
	{0xd77485cb25823ac7, 0x7d633293366b828b},  /* -212 */
	{0x86a8d39ef77164bc, 0xae5dff9c02033197},  /* -211 */
	{0xa8530886b54dbdeb, 0xd9f57f830283fdfc},  /* -210 */
	{0xd267caa862a12d66, 0xd072df63c324fd7b},  /* -209 */
	{0x8380dea93da4bc60, 0x4247cb9e59f71e6d},  /* -208 */
	{0xa46116538d0deb78, 0x52d9be85f074e608},  /* -207 */
	{0xcd795be870516656, 0x67902e276c921f8b},  /* -206 */
	{0x806bd9714632dff6, 0x00ba1cd8a3db53b6},  /* -205 */
	{0xa086cfcd97bf97f3, 0x80e8a40eccd228a4},  /* -204 */
	{0xc8a883c0fdaf7df0, 0x6122cd128006b2cd},  /* -203 */
	{0xfad2a4b13d1b5d6c, 0x796b805720085f81},  /* -202 */
	{0x9cc3a6eec6311a63, 0xcbe3303674053bb0},  /* -201 */
	{0xc3f490aa77bd60fc, 0xbedbfc4411068a9c},  /* -200 */
	{0xf4f1b4d515acb93b, 0xee92fb5515482d44},  /* -199 */
	{0x991711052d8bf3c5, 0x751bdd152d4d1c4a},  /* -198 */
	{0xbf5cd54678eef0b6, 0xd262d45a78a0635d},  /* -197 */
	{0xef340a98172aace4, 0x86fb897116c87c34},  /* -196 */
	{0x9580869f0e7aac0e, 0xd45d35e6ae3d4da0},  /* -195 */
	{0xbae0a846d2195712, 0x8974836059cca109},  /* -194 */
	{0xe998d258869facd7, 0x2bd1a438703fc94b},  /* -193 */
	{0x91ff83775423cc06, 0x7b6306a34627ddcf},  /* -192 */
	{0xb67f6455292cbf08, 0x1a3bc84c17b1d542},  /* -191 */
	{0xe41f3d6a7377eeca, 0x20caba5f1d9e4a93},  /* -190 */
	{0x8e938662882af53e, 0x547eb47b7282ee9c},  /* -189 */
	{0xb23867fb2a35b28d, 0xe99e619a4f23aa43},  /* -188 */
	{0xdec681f9f4c31f31, 0x6405fa00e2ec94d4},  /* -187 */
	{0x8b3c113c38f9f37e, 0xde83bc408dd3dd04},  /* -186 */
	{0xae0b158b4738705e, 0x9624ab50b148d445},  /* -185 */
	{0xd98ddaee19068c76, 0x3badd624dd9b0957},  /* -184 */
	{0x87f8a8d4cfa417c9, 0xe54ca5d70a80e5d6},  /* -183 */
	{0xa9f6d30a038d1dbc, 0x5e9fcf4ccd211f4c},  /* -182 */
	{0xd47487cc8470652b, 0x7647c3200069671f},  /* -181 */
	{0x84c8d4dfd2c63f3b, 0x29ecd9f40041e073},  /* -180 */
	{0xa5fb0a17c777cf09, 0xf468107100525890},  /* -179 */
	{0xcf79cc9db955c2cc, 0x7182148d4066eeb4},  /* -178 */
	{0x81ac1fe293d599bf, 0xc6f14cd848405530},  /* -177 */
	{0xa21727db38cb002f, 0xb8ada00e5a506a7c},  /* -176 */
	{0xca9cf1d206fdc03b, 0xa6d90811f0e4851c},  /* -175 */
	{0xfd442e4688bd304a, 0x908f4a166d1da663},  /* -174 */
	{0x9e4a9cec15763e2e, 0x9a598e4e043287fe},  /* -173 */
	{0xc5dd44271ad3cdba, 0x40eff1e1853f29fd},  /* -172 */
	{0xf7549530e188c128, 0xd12bee59e68ef47c},  /* -171 */
	{0x9a94dd3e8cf578b9, 0x82bb74f8301958ce},  /* -170 */
	{0xc13a148e3032d6e7, 0xe36a52363c1faf01},  /* -169 */
	{0xf18899b1bc3f8ca1, 0xdc44e6c3cb279ac1},  /* -168 */
	{0x96f5600f15a7b7e5, 0x29ab103a5ef8c0b9},  /* -167 */
	{0xbcb2b812db11a5de, 0x7415d448f6b6f0e7},  /* -166 */
	{0xebdf661791d60f56, 0x111b495b3464ad21},  /* -165 */
	{0x936b9fcebb25c995, 0xcab10dd900beec34},  /* -164 */
	{0xb84687c269ef3bfb, 0x3d5d514f40eea742},  /* -163 */
	{0xe65829b3046b0afa, 0x0cb4a5a3112a5112},  /* -162 */
	{0x8ff71a0fe2c2e6dc, 0x47f0e785eaba72ab},  /* -161 */
	{0xb3f4e093db73a093, 0x59ed216765690f56},  /* -160 */
	{0xe0f218b8d25088b8, 0x306869c13ec3532c},  /* -159 */
	{0x8c974f7383725573, 0x1e414218c73a13fb},  /* -158 */
	{0xafbd2350644eeacf, 0xe5d1929ef90898fa},  /* -157 */
	{0xdbac6c247d62a583, 0xdf45f746b74abf39},  /* -156 */
	{0x894bc396ce5da772, 0x6b8bba8c328eb783},  /* -155 */
	{0xab9eb47c81f5114f, 0x066ea92f3f326564},  /* -154 */
	{0xd686619ba27255a2, 0xc80a537b0efefebd},  /* -153 */
	{0x8613fd0145877585, 0xbd06742ce95f5f36},  /* -152 */
	{0xa798fc4196e952e7, 0x2c48113823b73704},  /* -151 */
	{0xd17f3b51fca3a7a0, 0xf75a15862ca504c5},  /* -150 */
	{0x82ef85133de648c4, 0x9a984d73dbe722fb},  /* -149 */
	{0xa3ab66580d5fdaf5, 0xc13e60d0d2e0ebba},  /* -148 */
	{0xcc963fee10b7d1b3, 0x318df905079926a8},  /* -147 */
	{0xffbbcfe994e5c61f, 0xfdf17746497f7052},  /* -146 */
	{0x9fd561f1fd0f9bd3, 0xfeb6ea8bedefa633},  /* -145 */
	{0xc7caba6e7c5382c8, 0xfe64a52ee96b8fc0},  /* -144 */
	{0xf9bd690a1b68637b, 0x3dfdce7aa3c673b0},  /* -143 */
	{0x9c1661a651213e2d, 0x06bea10ca65c084e},  /* -142 */
	{0xc31bfa0fe5698db8, 0x486e494fcff30a62},  /* -141 */
	{0xf3e2f893dec3f126, 0x5a89dba3c3efccfa},  /* -140 */
	{0x986ddb5c6b3a76b7, 0xf89629465a75e01c},  /* -139 */
	{0xbe89523386091465, 0xf6bbb397f1135823},  /* -138 */
	{0xee2ba6c0678b597f, 0x746aa07ded582e2c},  /* -137 */
	{0x94db483840b717ef, 0xa8c2a44eb4571cdc},  /* -136 */
	{0xba121a4650e4ddeb, 0x92f34d62616ce413},  /* -135 */
	{0xe896a0d7e51e1566, 0x77b020baf9c81d17},  /* -134 */
	{0x915e2486ef32cd60, 0x0ace1474dc1d122e},  /* -133 */
	{0xb5b5ada8aaff80b8, 0x0d819992132456ba},  /* -132 */
	{0xe3231912d5bf60e6, 0x10e1fff697ed6c69},  /* -131 */
	{0x8df5efabc5979c8f, 0xca8d3ffa1ef463c1},  /* -130 */
	{0xb1736b96b6fd83b3, 0xbd308ff8a6b17cb2},  /* -129 */
	{0xddd0467c64bce4a0, 0xac7cb3f6d05ddbde},  /* -128 */
	{0x8aa22c0dbef60ee4, 0x6bcdf07a423aa96b},  /* -127 */
	{0xad4ab7112eb3929d, 0x86c16c98d2c953c6},  /* -126 */
	{0xd89d64d57a607744, 0xe871c7bf077ba8b7},  /* -125 */
	{0x87625f056c7c4a8b, 0x11471cd764ad4972},  /* -124 */
	{0xa93af6c6c79b5d2d, 0xd598e40d3dd89bcf},  /* -123 */
	{0xd389b47879823479, 0x4aff1d108d4ec2c3},  /* -122 */
	{0x843610cb4bf160cb, 0xcedf722a585139ba},  /* -121 */
	{0xa54394fe1eedb8fe, 0xc2974eb4ee658828},  /* -120 */
	{0xce947a3da6a9273e, 0x733d226229feea32},  /* -119 */
	{0x811ccc668829b887, 0x0806357d5a3f525f},  /* -118 */
	{0xa163ff802a3426a8, 0xca07c2dcb0cf26f7},  /* -117 */
	{0xc9bcff6034c13052, 0xfc89b393dd02f0b5},  /* -116 */
	{0xfc2c3f3841f17c67, 0xbbac2078d443ace2},  /* -115 */
	{0x9d9ba7832936edc0, 0xd54b944b84aa4c0d},  /* -114 */
	{0xc5029163f384a931, 0x0a9e795e65d4df11},  /* -113 */
	{0xf64335bcf065d37d, 0x4d4617b5ff4a16d5},  /* -112 */
	{0x99ea0196163fa42e, 0x504bced1bf8e4e45},  /* -111 */
	{0xc06481fb9bcf8d39, 0xe45ec2862f71e1d6},  /* -110 */
	{0xf07da27a82c37088, 0x5d767327bb4e5a4c},  /* -109 */
	{0x964e858c91ba2655, 0x3a6a07f8d510f86f},  /* -108 */
	{0xbbe226efb628afea, 0x890489f70a55368b},  /* -107 */
	{0xeadab0aba3b2dbe5, 0x2b45ac74ccea842e},  /* -106 */
	{0x92c8ae6b464fc96f, 0x3b0b8bc90012929d},  /* -105 */
	{0xb77ada0617e3bbcb, 0x09ce6ebb40173744},  /* -104 */
	{0xe55990879ddcaabd, 0xcc420a6a101d0515},  /* -103 */
	{0x8f57fa54c2a9eab6, 0x9fa946824a12232d},  /* -102 */
	{0xb32df8e9f3546564, 0x47939822dc96abf9},  /* -101 */
	{0xdff9772470297ebd, 0x59787e2b93bc56f7},  /* -100 */
	{0x8bfbea76c619ef36, 0x57eb4edb3c55b65a},  /* -99 */
	{0xaefae51477a06b03, 0xede622920b6b23f1},  /* -98 */
	{0xdab99e59958885c4, 0xe95fab368e45eced},  /* -97 */
	{0x88b402f7fd75539b, 0x11dbcb0218ebb414},  /* -96 */
	{0xaae103b5fcd2a881, 0xd652bdc29f26a119},  /* -95 */
	{0xd59944a37c0752a2, 0x4be76d3346f0495f},  /* -94 */
	{0x857fcae62d8493a5, 0x6f70a4400c562ddb},  /* -93 */
	{0xa6dfbd9fb8e5b88e, 0xcb4ccd500f6bb952},  /* -92 */
	{0xd097ad07a71f26b2, 0x7e2000a41346a7a7},  /* -91 */
	{0x825ecc24c873782f, 0x8ed400668c0c28c8},  /* -90 */
	{0xa2f67f2dfa90563b, 0x728900802f0f32fa},  /* -89 */
	{0xcbb41ef979346bca, 0x4f2b40a03ad2ffb9},  /* -88 */
	{0xfea126b7d78186bc, 0xe2f610c84987bfa8},  /* -87 */
	{0x9f24b832e6b0f436, 0x0dd9ca7d2df4d7c9},  /* -86 */
	{0xc6ede63fa05d3143, 0x91503d1c79720dbb},  /* -85 */
	{0xf8a95fcf88747d94, 0x75a44c6397ce912a},  /* -84 */
	{0x9b69dbe1b548ce7c, 0xc986afbe3ee11aba},  /* -83 */
	{0xc24452da229b021b, 0xfbe85badce996168},  /* -82 */
	{0xf2d56790ab41c2a2, 0xfae27299423fb9c3},  /* -81 */
	{0x97c560ba6b0919a5, 0xdccd879fc967d41a},  /* -80 */
	{0xbdb6b8e905cb600f, 0x5400e987bbc1c920},  /* -79 */
	{0xed246723473e3813, 0x290123e9aab23b68},  /* -78 */
	{0x9436c0760c86e30b, 0xf9a0b6720aaf6521},  /* -77 */
	{0xb94470938fa89bce, 0xf808e40e8d5b3e69},  /* -76 */
	{0xe7958cb87392c2c2, 0xb60b1d1230b20e04},  /* -75 */
	{0x90bd77f3483bb9b9, 0xb1c6f22b5e6f48c2},  /* -74 */
	{0xb4ecd5f01a4aa828, 0x1e38aeb6360b1af3},  /* -73 */
	{0xe2280b6c20dd5232, 0x25c6da63c38de1b0},  /* -72 */
	{0x8d590723948a535f, 0x579c487e5a38ad0e},  /* -71 */
	{0xb0af48ec79ace837, 0x2d835a9df0c6d851},  /* -70 */
	{0xdcdb1b2798182244, 0xf8e431456cf88e65},  /* -69 */
	{0x8a08f0f8bf0f156b, 0x1b8e9ecb641b58ff},  /* -68 */
	{0xac8b2d36eed2dac5, 0xe272467e3d222f3f},  /* -67 */
	{0xd7adf884aa879177, 0x5b0ed81dcc6abb0f},  /* -66 */
	{0x86ccbb52ea94baea, 0x98e947129fc2b4e9},  /* -65 */
	{0xa87fea27a539e9a5, 0x3f2398d747b36224},  /* -64 */
	{0xd29fe4b18e88640e, 0x8eec7f0d19a03aad},  /* -63 */
	{0x83a3eeeef9153e89, 0x1953cf68300424ac},  /* -62 */
	{0xa48ceaaab75a8e2b, 0x5fa8c3423c052dd7},  /* -61 */
	{0xcdb02555653131b6, 0x3792f412cb06794d},  /* -60 */
	{0x808e17555f3ebf11, 0xe2bbd88bbee40bd0},  /* -59 */
	{0xa0b19d2ab70e6ed6, 0x5b6aceaeae9d0ec4},  /* -58 */
	{0xc8de047564d20a8b, 0xf245825a5a445275},  /* -57 */
	{0xfb158592be068d2e, 0xeed6e2f0f0d56712},  /* -56 */
	{0x9ced737bb6c4183d, 0x55464dd69685606b},  /* -55 */
	{0xc428d05aa4751e4c, 0xaa97e14c3c26b886},  /* -54 */
	{0xf53304714d9265df, 0xd53dd99f4b3066a8},  /* -53 */
	{0x993fe2c6d07b7fab, 0xe546a8038efe4029},  /* -52 */
	{0xbf8fdb78849a5f96, 0xde98520472bdd033},  /* -51 */
	{0xef73d256a5c0f77c, 0x963e66858f6d4440},  /* -50 */
	{0x95a8637627989aad, 0xdde7001379a44aa8},  /* -49 */
	{0xbb127c53b17ec159, 0x5560c018580d5d52},  /* -48 */
	{0xe9d71b689dde71af, 0xaab8f01e6e10b4a6},  /* -47 */
	{0x9226712162ab070d, 0xcab3961304ca70e8},  /* -46 */
	{0xb6b00d69bb55c8d1, 0x3d607b97c5fd0d22},  /* -45 */
	{0xe45c10c42a2b3b05, 0x8cb89a7db77c506a},  /* -44 */
	{0x8eb98a7a9a5b04e3, 0x77f3608e92adb242},  /* -43 */
	{0xb267ed1940f1c61c, 0x55f038b237591ed3},  /* -42 */
	{0xdf01e85f912e37a3, 0x6b6c46dec52f6688},  /* -41 */
	{0x8b61313bbabce2c6, 0x2323ac4b3b3da015},  /* -40 */
	{0xae397d8aa96c1b77, 0xabec975e0a0d081a},  /* -39 */
	{0xd9c7dced53c72255, 0x96e7bd358c904a21},  /* -38 */
	{0x881cea14545c7575, 0x7e50d64177da2e54},  /* -37 */
	{0xaa242499697392d2, 0xdde50bd1d5d0b9e9},  /* -36 */
	{0xd4ad2dbfc3d07787, 0x955e4ec64b44e864},  /* -35 */
	{0x84ec3c97da624ab4, 0xbd5af13bef0b113e},  /* -34 */
	{0xa6274bbdd0fadd61, 0xecb1ad8aeacdd58e},  /* -33 */
	{0xcfb11ead453994ba, 0x67de18eda5814af2},  /* -32 */
	{0x81ceb32c4b43fcf4, 0x80eacf948770ced7},  /* -31 */
	{0xa2425ff75e14fc31, 0xa1258379a94d028d},  /* -30 */
	{0xcad2f7f5359a3b3e, 0x096ee45813a04330},  /* -29 */
	{0xfd87b5f28300ca0d, 0x8bca9d6e188853fc},  /* -28 */
	{0x9e74d1b791e07e48, 0x775ea264cf55347e},  /* -27 */
	{0xc612062576589dda, 0x95364afe032a81a0},  /* -26 */
	{0xf79687aed3eec551, 0x3a83ddbd83f52210},  /* -25 */
	{0x9abe14cd44753b52, 0xc4926a9672793580},  /* -24 */
	{0xc16d9a0095928a27, 0x75b7053c0f178400},  /* -23 */
	{0xf1c90080baf72cb1, 0x5324c68b12dd6800},  /* -22 */
	{0x971da05074da7bee, 0xd3f6fc16ebca8000},  /* -21 */
	{0xbce5086492111aea, 0x88f4bb1ca6bd0000},  /* -20 */
	{0xec1e4a7db69561a5, 0x2b31e9e3d0700000},  /* -19 */
	{0x9392ee8e921d5d07, 0x3aff322e62600000},  /* -18 */
	{0xb877aa3236a4b449, 0x09befeb9fad487c3},  /* -17 */
	{0xe69594bec44de15b, 0x4c2ebe687989a9b4},  /* -16 */
	{0x901d7cf73ab0acd9, 0x0f9d37014bf60a11},  /* -15 */
	{0xb424dc35095cd80f, 0x538484c19ef38c95},  /* -14 */
	{0xe12e13424bb40e13, 0x2865a5f206b06fba},  /* -13 */
	{0x8cbccc096f5088cb, 0xf93f87b7442e45d4},  /* -12 */
	{0xafebff0bcb24aafe, 0xf78f69a51539d749},  /* -11 */
	{0xdbe6fecebdedd5be, 0xb573440e5a884d1c},  /* -10 */
	{0x89705f4136b4a597, 0x31680a88f8953031},  /* -09 */
	{0xabcc77118461cefc, 0xfdc20d2b36ba7c3e},  /* -08 */
	{0xd6bf94d5e57a42bc, 0x3d32907604691b4d},  /* -07 */
	{0x8637bd05af6c69b5, 0xa63f9a49c2c1b110},  /* -06 */
	{0xa7c5ac471b478423, 0x0fcf80dc33721d54},  /* -05 */
	{0xd1b71758e219652b, 0xd3c36113404ea4a9},  /* -04 */
	{0x83126e978d4fdf3b, 0x645a1cac083126ea},  /* -03 */
	{0xa3d70a3d70a3d70a, 0x3d70a3d70a3d70a4},  /* -02 */
	{0xcccccccccccccccc, 0xcccccccccccccccd},  /* -01 */
	{0x8000000000000000, 0x0000000000000000},  /* 000 */
	{0xa000000000000000, 0x0000000000000000},  /* 001 */
	{0xc800000000000000, 0x0000000000000000},  /* 002 */
	{0xfa00000000000000, 0x0000000000000000},  /* 003 */
	{0x9c40000000000000, 0x0000000000000000},  /* 004 */
	{0xc350000000000000, 0x0000000000000000},  /* 005 */
	{0xf424000000000000, 0x0000000000000000},  /* 006 */
	{0x9896800000000000, 0x0000000000000000},  /* 007 */
	{0xbebc200000000000, 0x0000000000000000},  /* 008 */
	{0xee6b280000000000, 0x0000000000000000},  /* 009 */
	{0x9502f90000000000, 0x0000000000000000},  /* 010 */
	{0xba43b74000000000, 0x0000000000000000},  /* 011 */
	{0xe8d4a51000000000, 0x0000000000000000},  /* 012 */
	{0x9184e72a00000000, 0x0000000000000000},  /* 013 */
	{0xb5e620f480000000, 0x0000000000000000},  /* 014 */
	{0xe35fa931a0000000, 0x0000000000000000},  /* 015 */
	{0x8e1bc9bf04000000, 0x0000000000000000},  /* 016 */
	{0xb1a2bc2ec5000000, 0x0000000000000000},  /* 017 */
	{0xde0b6b3a76400000, 0x0000000000000000},  /* 018 */
	{0x8ac7230489e80000, 0x0000000000000000},  /* 019 */
	{0xad78ebc5ac620000, 0x0000000000000000},  /* 020 */
	{0xd8d726b7177a8000, 0x0000000000000000},  /* 021 */
	{0x878678326eac9000, 0x0000000000000000},  /* 022 */
	{0xa968163f0a57b400, 0x0000000000000000},  /* 023 */
	{0xd3c21bcecceda100, 0x0000000000000000},  /* 024 */
	{0x84595161401484a0, 0x0000000000000000},  /* 025 */
	{0xa56fa5b99019a5c8, 0x0000000000000000},  /* 026 */
	{0xcecb8f27f4200f3a, 0x0000000000000000},  /* 027 */
	{0x813f3978f8940984, 0x4000000000000000},  /* 028 */
	{0xa18f07d736b90be5, 0x5000000000000000},  /* 029 */
	{0xc9f2c9cd04674ede, 0xa400000000000000},  /* 030 */
	{0xfc6f7c4045812296, 0x4d00000000000000},  /* 031 */
	{0x9dc5ada82b70b59d, 0xf020000000000000},  /* 032 */
	{0xc5371912364ce305, 0x6c28000000000000},  /* 033 */
	{0xf684df56c3e01bc6, 0xc732000000000000},  /* 034 */
	{0x9a130b963a6c115c, 0x3c7f400000000000},  /* 035 */
	{0xc097ce7bc90715b3, 0x4b9f100000000000},  /* 036 */
	{0xf0bdc21abb48db20, 0x1e86d40000000000},  /* 037 */
	{0x96769950b50d88f4, 0x1314448000000000},  /* 038 */
	{0xbc143fa4e250eb31, 0x17d955a000000000},  /* 039 */
	{0xeb194f8e1ae525fd, 0x5dcfab0800000000},  /* 040 */
	{0x92efd1b8d0cf37be, 0x5aa1cae500000000},  /* 041 */
	{0xb7abc627050305ad, 0xf14a3d9e40000000},  /* 042 */
	{0xe596b7b0c643c719, 0x6d9ccd05d0000000},  /* 043 */
	{0x8f7e32ce7bea5c6f, 0xe4820023a2000000},  /* 044 */
	{0xb35dbf821ae4f38b, 0xdda2802c8a800000},  /* 045 */
	{0xe0352f62a19e306e, 0xd50b2037ad200000},  /* 046 */
	{0x8c213d9da502de45, 0x4526f422cc340000},  /* 047 */
	{0xaf298d050e4395d6, 0x9670b12b7f410000},  /* 048 */
	{0xdaf3f04651d47b4c, 0x3c0cdd765f114000},  /* 049 */
	{0x88d8762bf324cd0f, 0xa5880a69fb6ac800},  /* 050 */
	{0xab0e93b6efee0053, 0x8eea0d047a457a00},  /* 051 */
	{0xd5d238a4abe98068, 0x72a4904598d6d880},  /* 052 */
	{0x85a36366eb71f041, 0x47a6da2b7f864750},  /* 053 */
	{0xa70c3c40a64e6c51, 0x999090b65f67d924},  /* 054 */
	{0xd0cf4b50cfe20765, 0xfff4b4e3f741cf6d},  /* 055 */
	{0x82818f1281ed449f, 0xbff8f10e7a8921a4},  /* 056 */
	{0xa321f2d7226895c7, 0xaff72d52192b6a0d},  /* 057 */
	{0xcbea6f8ceb02bb39, 0x9bf4f8a69f764490},  /* 058 */
	{0xfee50b7025c36a08, 0x02f236d04753d5b4},  /* 059 */
	{0x9f4f2726179a2245, 0x01d762422c946590},  /* 060 */
	{0xc722f0ef9d80aad6, 0x424d3ad2b7b97ef5},  /* 061 */
	{0xf8ebad2b84e0d58b, 0xd2e0898765a7deb2},  /* 062 */
	{0x9b934c3b330c8577, 0x63cc55f49f88eb2f},  /* 063 */
	{0xc2781f49ffcfa6d5, 0x3cbf6b71c76b25fb},  /* 064 */
	{0xf316271c7fc3908a, 0x8bef464e3945ef7a},  /* 065 */
	{0x97edd871cfda3a56, 0x97758bf0e3cbb5ac},  /* 066 */
	{0xbde94e8e43d0c8ec, 0x3d52eeed1cbea317},  /* 067 */
	{0xed63a231d4c4fb27, 0x4ca7aaa863ee4bdd},  /* 068 */
	{0x945e455f24fb1cf8, 0x8fe8caa93e74ef6a},  /* 069 */
	{0xb975d6b6ee39e436, 0xb3e2fd538e122b44},  /* 070 */
	{0xe7d34c64a9c85d44, 0x60dbbca87196b616},  /* 071 */
	{0x90e40fbeea1d3a4a, 0xbc8955e946fe31cd},  /* 072 */
	{0xb51d13aea4a488dd, 0x6babab6398bdbe41},  /* 073 */
	{0xe264589a4dcdab14, 0xc696963c7eed2dd1},  /* 074 */
	{0x8d7eb76070a08aec, 0xfc1e1de5cf543ca2},  /* 075 */
	{0xb0de65388cc8ada8, 0x3b25a55f43294bcb},  /* 076 */
	{0xdd15fe86affad912, 0x49ef0eb713f39ebe},  /* 077 */
	{0x8a2dbf142dfcc7ab, 0x6e3569326c784337},  /* 078 */
	{0xacb92ed9397bf996, 0x49c2c37f07965404},  /* 079 */
	{0xd7e77a8f87daf7fb, 0xdc33745ec97be906},  /* 080 */
	{0x86f0ac99b4e8dafd, 0x69a028bb3ded71a3},  /* 081 */
	{0xa8acd7c0222311bc, 0xc40832ea0d68ce0c},  /* 082 */
	{0xd2d80db02aabd62b, 0xf50a3fa490c30190},  /* 083 */
	{0x83c7088e1aab65db, 0x792667c6da79e0fa},  /* 084 */
	{0xa4b8cab1a1563f52, 0x577001b891185938},  /* 085 */
	{0xcde6fd5e09abcf26, 0xed4c0226b55e6f86},  /* 086 */
	{0x80b05e5ac60b6178, 0x544f8158315b05b4},  /* 087 */
	{0xa0dc75f1778e39d6, 0x696361ae3db1c721},  /* 088 */
	{0xc913936dd571c84c, 0x03bc3a19cd1e38e9},  /* 089 */
	{0xfb5878494ace3a5f, 0x04ab48a04065c723},  /* 090 */
	{0x9d174b2dcec0e47b, 0x62eb0d64283f9c76},  /* 091 */
	{0xc45d1df942711d9a, 0x3ba5d0bd324f8394},  /* 092 */
	{0xf5746577930d6500, 0xca8f44ec7ee36479},  /* 093 */
	{0x9968bf6abbe85f20, 0x7e998b13cf4e1ecb},  /* 094 */
	{0xbfc2ef456ae276e8, 0x9e3fedd8c321a67e},  /* 095 */
	{0xefb3ab16c59b14a2, 0xc5cfe94ef3ea101e},  /* 096 */
	{0x95d04aee3b80ece5, 0xbba1f1d158724a12},  /* 097 */
	{0xbb445da9ca61281f, 0x2a8a6e45ae8edc97},  /* 098 */
	{0xea1575143cf97226, 0xf52d09d71a3293bd},  /* 099 */
	{0x924d692ca61be758, 0x593c2626705f9c56},  /* 100 */
	{0xb6e0c377cfa2e12e, 0x6f8b2fb00c77836c},  /* 101 */
	{0xe498f455c38b997a, 0x0b6dfb9c0f956447},  /* 102 */
	{0x8edf98b59a373fec, 0x4724bd4189bd5eac},  /* 103 */
	{0xb2977ee300c50fe7, 0x58edec91ec2cb657},  /* 104 */
	{0xdf3d5e9bc0f653e1, 0x2f2967b66737e3ed},  /* 105 */
	{0x8b865b215899f46c, 0xbd79e0d20082ee74},  /* 106 */
	{0xae67f1e9aec07187, 0xecd8590680a3aa11},  /* 107 */
	{0xda01ee641a708de9, 0xe80e6f4820cc9495},  /* 108 */
	{0x884134fe908658b2, 0x3109058d147fdcdd},  /* 109 */
	{0xaa51823e34a7eede, 0xbd4b46f0599fd415},  /* 110 */
	{0xd4e5e2cdc1d1ea96, 0x6c9e18ac7007c91a},  /* 111 */
	{0x850fadc09923329e, 0x03e2cf6bc604ddb0},  /* 112 */
	{0xa6539930bf6bff45, 0x84db8346b786151c},  /* 113 */
	{0xcfe87f7cef46ff16, 0xe612641865679a63},  /* 114 */
	{0x81f14fae158c5f6e, 0x4fcb7e8f3f60c07e},  /* 115 */
	{0xa26da3999aef7749, 0xe3be5e330f38f09d},  /* 116 */
	{0xcb090c8001ab551c, 0x5cadf5bfd3072cc5},  /* 117 */
	{0xfdcb4fa002162a63, 0x73d9732fc7c8f7f6},  /* 118 */
	{0x9e9f11c4014dda7e, 0x2867e7fddcdd9afa},  /* 119 */
	{0xc646d63501a1511d, 0xb281e1fd541501b8},  /* 120 */
	{0xf7d88bc24209a565, 0x1f225a7ca91a4226},  /* 121 */
	{0x9ae757596946075f, 0x3375788de9b06958},  /* 122 */
	{0xc1a12d2fc3978937, 0x0052d6b1641c83ae},  /* 123 */
	{0xf209787bb47d6b84, 0xc0678c5dbd23a49a},  /* 124 */
	{0x9745eb4d50ce6332, 0xf840b7ba963646e0},  /* 125 */
	{0xbd176620a501fbff, 0xb650e5a93bc3d898},  /* 126 */
	{0xec5d3fa8ce427aff, 0xa3e51f138ab4cebe},  /* 127 */
	{0x93ba47c980e98cdf, 0xc66f336c36b10137},  /* 128 */
	{0xb8a8d9bbe123f017, 0xb80b0047445d4184},  /* 129 */
	{0xe6d3102ad96cec1d, 0xa60dc059157491e5},  /* 130 */
	{0x9043ea1ac7e41392, 0x87c89837ad68db2f},  /* 131 */
	{0xb454e4a179dd1877, 0x29babe4598c311fb},  /* 132 */
	{0xe16a1dc9d8545e94, 0xf4296dd6fef3d67a},  /* 133 */
	{0x8ce2529e2734bb1d, 0x1899e4a65f58660c},  /* 134 */
	{0xb01ae745b101e9e4, 0x5ec05dcff72e7f8f},  /* 135 */
	{0xdc21a1171d42645d, 0x76707543f4fa1f73},  /* 136 */
	{0x899504ae72497eba, 0x6a06494a791c53a8},  /* 137 */
	{0xabfa45da0edbde69, 0x0487db9d17636892},  /* 138 */
	{0xd6f8d7509292d603, 0x45a9d2845d3c42b6},  /* 139 */
	{0x865b86925b9bc5c2, 0x0b8a2392ba45a9b2},  /* 140 */
	{0xa7f26836f282b732, 0x8e6cac7768d7141e},  /* 141 */
	{0xd1ef0244af2364ff, 0x3207d795430cd926},  /* 142 */
	{0x8335616aed761f1f, 0x7f44e6bd49e807b8},  /* 143 */
	{0xa402b9c5a8d3a6e7, 0x5f16206c9c6209a6},  /* 144 */
	{0xcd036837130890a1, 0x36dba887c37a8c0f},  /* 145 */
	{0x802221226be55a64, 0xc2494954da2c9789},  /* 146 */
	{0xa02aa96b06deb0fd, 0xf2db9baa10b7bd6c},  /* 147 */
	{0xc83553c5c8965d3d, 0x6f92829494e5acc7},  /* 148 */
	{0xfa42a8b73abbf48c, 0xcb772339ba1f17f9},  /* 149 */
	{0x9c69a97284b578d7, 0xff2a760414536efb},  /* 150 */
	{0xc38413cf25e2d70d, 0xfef5138519684aba},  /* 151 */
	{0xf46518c2ef5b8cd1, 0x7eb258665fc25d69},  /* 152 */
	{0x98bf2f79d5993802, 0xef2f773ffbd97a61},  /* 153 */
	{0xbeeefb584aff8603, 0xaafb550ffacfd8fa},  /* 154 */
	{0xeeaaba2e5dbf6784, 0x95ba2a53f983cf38},  /* 155 */
	{0x952ab45cfa97a0b2, 0xdd945a747bf26183},  /* 156 */
	{0xba756174393d88df, 0x94f971119aeef9e4},  /* 157 */
	{0xe912b9d1478ceb17, 0x7a37cd5601aab85d},  /* 158 */
	{0x91abb422ccb812ee, 0xac62e055c10ab33a},  /* 159 */
	{0xb616a12b7fe617aa, 0x577b986b314d6009},  /* 160 */
	{0xe39c49765fdf9d94, 0xed5a7e85fda0b80b},  /* 161 */
	{0x8e41ade9fbebc27d, 0x14588f13be847307},  /* 162 */
	{0xb1d219647ae6b31c, 0x596eb2d8ae258fc8},  /* 163 */
	{0xde469fbd99a05fe3, 0x6fca5f8ed9aef3bb},  /* 164 */
	{0x8aec23d680043bee, 0x25de7bb9480d5854},  /* 165 */
	{0xada72ccc20054ae9, 0xaf561aa79a10ae6a},  /* 166 */
	{0xd910f7ff28069da4, 0x1b2ba1518094da04},  /* 167 */
	{0x87aa9aff79042286, 0x90fb44d2f05d0842},  /* 168 */
	{0xa99541bf57452b28, 0x353a1607ac744a53},  /* 169 */
	{0xd3fa922f2d1675f2, 0x42889b8997915ce8},  /* 170 */
	{0x847c9b5d7c2e09b7, 0x69956135febada11},  /* 171 */
	{0xa59bc234db398c25, 0x43fab9837e699095},  /* 172 */
	{0xcf02b2c21207ef2e, 0x94f967e45e03f4bb},  /* 173 */
	{0x8161afb94b44f57d, 0x1d1be0eebac278f5},  /* 174 */
	{0xa1ba1ba79e1632dc, 0x6462d92a69731732},  /* 175 */
	{0xca28a291859bbf93, 0x7d7b8f7503cfdcfe},  /* 176 */
	{0xfcb2cb35e702af78, 0x5cda735244c3d43e},  /* 177 */
	{0x9defbf01b061adab, 0x3a0888136afa64a7},  /* 178 */
	{0xc56baec21c7a1916, 0x088aaa1845b8fdd0},  /* 179 */
	{0xf6c69a72a3989f5b, 0x8aad549e57273d45},  /* 180 */
	{0x9a3c2087a63f6399, 0x36ac54e2f678864b},  /* 181 */
	{0xc0cb28a98fcf3c7f, 0x84576a1bb416a7dd},  /* 182 */
	{0xf0fdf2d3f3c30b9f, 0x656d44a2a11c51d5},  /* 183 */
	{0x969eb7c47859e743, 0x9f644ae5a4b1b325},  /* 184 */
	{0xbc4665b596706114, 0x873d5d9f0dde1fee},  /* 185 */
	{0xeb57ff22fc0c7959, 0xa90cb506d155a7ea},  /* 186 */
	{0x9316ff75dd87cbd8, 0x09a7f12442d588f2},  /* 187 */
	{0xb7dcbf5354e9bece, 0x0c11ed6d538aeb2f},  /* 188 */
	{0xe5d3ef282a242e81, 0x8f1668c8a86da5fa},  /* 189 */
	{0x8fa475791a569d10, 0xf96e017d694487bc},  /* 190 */
	{0xb38d92d760ec4455, 0x37c981dcc395a9ac},  /* 191 */
	{0xe070f78d3927556a, 0x85bbe253f47b1417},  /* 192 */
	{0x8c469ab843b89562, 0x93956d7478ccec8e},  /* 193 */
	{0xaf58416654a6babb, 0x387ac8d1970027b2},  /* 194 */
	{0xdb2e51bfe9d0696a, 0x06997b05fcc0319e},  /* 195 */
	{0x88fcf317f22241e2, 0x441fece3bdf81f03},  /* 196 */
	{0xab3c2fddeeaad25a, 0xd527e81cad7626c3},  /* 197 */
	{0xd60b3bd56a5586f1, 0x8a71e223d8d3b074},  /* 198 */
	{0x85c7056562757456, 0xf6872d5667844e49},  /* 199 */
	{0xa738c6bebb12d16c, 0xb428f8ac016561db},  /* 200 */
	{0xd106f86e69d785c7, 0xe13336d701beba52},  /* 201 */
	{0x82a45b450226b39c, 0xecc0024661173473},  /* 202 */
	{0xa34d721642b06084, 0x27f002d7f95d0190},  /* 203 */
	{0xcc20ce9bd35c78a5, 0x31ec038df7b441f4},  /* 204 */
	{0xff290242c83396ce, 0x7e67047175a15271},  /* 205 */
	{0x9f79a169bd203e41, 0x0f0062c6e984d386},  /* 206 */
	{0xc75809c42c684dd1, 0x52c07b78a3e60868},  /* 207 */
	{0xf92e0c3537826145, 0xa7709a56ccdf8a82},  /* 208 */
	{0x9bbcc7a142b17ccb, 0x88a66076400bb691},  /* 209 */
	{0xc2abf989935ddbfe, 0x6acff893d00ea435},  /* 210 */
	{0xf356f7ebf83552fe, 0x0583f6b8c4124d43},  /* 211 */
	{0x98165af37b2153de, 0xc3727a337a8b704a},  /* 212 */
	{0xbe1bf1b059e9a8d6, 0x744f18c0592e4c5c},  /* 213 */
	{0xeda2ee1c7064130c, 0x1162def06f79df73},  /* 214 */
	{0x9485d4d1c63e8be7, 0x8addcb5645ac2ba8},  /* 215 */
	{0xb9a74a0637ce2ee1, 0x6d953e2bd7173692},  /* 216 */
	{0xe8111c87c5c1ba99, 0xc8fa8db6ccdd0437},  /* 217 */
	{0x910ab1d4db9914a0, 0x1d9c9892400a22a2},  /* 218 */
	{0xb54d5e4a127f59c8, 0x2503beb6d00cab4b},  /* 219 */
	{0xe2a0b5dc971f303a, 0x2e44ae64840fd61d},  /* 220 */
	{0x8da471a9de737e24, 0x5ceaecfed289e5d2},  /* 221 */
	{0xb10d8e1456105dad, 0x7425a83e872c5f47},  /* 222 */
	{0xdd50f1996b947518, 0xd12f124e28f77719},  /* 223 */
	{0x8a5296ffe33cc92f, 0x82bd6b70d99aaa6f},  /* 224 */
	{0xace73cbfdc0bfb7b, 0x636cc64d1001550b},  /* 225 */
	{0xd8210befd30efa5a, 0x3c47f7e05401aa4e},  /* 226 */
	{0x8714a775e3e95c78, 0x65acfaec34810a71},  /* 227 */
	{0xa8d9d1535ce3b396, 0x7f1839a741a14d0d},  /* 228 */
	{0xd31045a8341ca07c, 0x1ede48111209a050},  /* 229 */
	{0x83ea2b892091e44d, 0x934aed0aab460432},  /* 230 */
	{0xa4e4b66b68b65d60, 0xf81da84d5617853f},  /* 231 */
	{0xce1de40642e3f4b9, 0x36251260ab9d668e},  /* 232 */
	{0x80d2ae83e9ce78f3, 0xc1d72b7c6b426019},  /* 233 */
	{0xa1075a24e4421730, 0xb24cf65b8612f81f},  /* 234 */
	{0xc94930ae1d529cfc, 0xdee033f26797b627},  /* 235 */
	{0xfb9b7cd9a4a7443c, 0x169840ef017da3b1},  /* 236 */
	{0x9d412e0806e88aa5, 0x8e1f289560ee864e},  /* 237 */
	{0xc491798a08a2ad4e, 0xf1a6f2bab92a27e2},  /* 238 */
	{0xf5b5d7ec8acb58a2, 0xae10af696774b1db},  /* 239 */
	{0x9991a6f3d6bf1765, 0xacca6da1e0a8ef29},  /* 240 */
	{0xbff610b0cc6edd3f, 0x17fd090a58d32af3},  /* 241 */
	{0xeff394dcff8a948e, 0xddfc4b4cef07f5b0},  /* 242 */
	{0x95f83d0a1fb69cd9, 0x4abdaf101564f98e},  /* 243 */
	{0xbb764c4ca7a4440f, 0x9d6d1ad41abe37f1},  /* 244 */
	{0xea53df5fd18d5513, 0x84c86189216dc5ed},  /* 245 */
	{0x92746b9be2f8552c, 0x32fd3cf5b4e49bb4},  /* 246 */
	{0xb7118682dbb66a77, 0x3fbc8c33221dc2a1},  /* 247 */
	{0xe4d5e82392a40515, 0x0fabaf3feaa5334a},  /* 248 */
	{0x8f05b1163ba6832d, 0x29cb4d87f2a7400e},  /* 249 */
	{0xb2c71d5bca9023f8, 0x743e20e9ef511012},  /* 250 */
	{0xdf78e4b2bd342cf6, 0x914da9246b255416},  /* 251 */
	{0x8bab8eefb6409c1a, 0x1ad089b6c2f7548e},  /* 252 */
	{0xae9672aba3d0c320, 0xa184ac2473b529b1},  /* 253 */
	{0xda3c0f568cc4f3e8, 0xc9e5d72d90a2741e},  /* 254 */
	{0x8865899617fb1871, 0x7e2fa67c7a658892},  /* 255 */
	{0xaa7eebfb9df9de8d, 0xddbb901b98feeab7},  /* 256 */
	{0xd51ea6fa85785631, 0x552a74227f3ea565},  /* 257 */
	{0x8533285c936b35de, 0xd53a88958f87275f},  /* 258 */
	{0xa67ff273b8460356, 0x8a892abaf368f137},  /* 259 */
	{0xd01fef10a657842c, 0x2d2b7569b0432d85},  /* 260 */
	{0x8213f56a67f6b29b, 0x9c3b29620e29fc73},  /* 261 */
	{0xa298f2c501f45f42, 0x8349f3ba91b47b8f},  /* 262 */
	{0xcb3f2f7642717713, 0x241c70a936219a73},  /* 263 */
	{0xfe0efb53d30dd4d7, 0xed238cd383aa0110},  /* 264 */
	{0x9ec95d1463e8a506, 0xf4363804324a40aa},  /* 265 */
	{0xc67bb4597ce2ce48, 0xb143c6053edcd0d5},  /* 266 */
	{0xf81aa16fdc1b81da, 0xdd94b7868e94050a},  /* 267 */
	{0x9b10a4e5e9913128, 0xca7cf2b4191c8326},  /* 268 */
	{0xc1d4ce1f63f57d72, 0xfd1c2f611f63a3f0},  /* 269 */
	{0xf24a01a73cf2dccf, 0xbc633b39673c8cec},  /* 270 */
	{0x976e41088617ca01, 0xd5be0503e085d813},  /* 271 */
	{0xbd49d14aa79dbc82, 0x4b2d8644d8a74e18},  /* 272 */
	{0xec9c459d51852ba2, 0xddf8e7d60ed1219e},  /* 273 */
	{0x93e1ab8252f33b45, 0xcabb90e5c942b503},  /* 274 */
	{0xb8da1662e7b00a17, 0x3d6a751f3b936243},  /* 275 */
	{0xe7109bfba19c0c9d, 0x0cc512670a783ad4},  /* 276 */
	{0x906a617d450187e2, 0x27fb2b80668b24c5},  /* 277 */
	{0xb484f9dc9641e9da, 0xb1f9f660802dedf6},  /* 278 */
	{0xe1a63853bbd26451, 0x5e7873f8a0396973},  /* 279 */
	{0x8d07e33455637eb2, 0xdb0b487b6423e1e8},  /* 280 */
	{0xb049dc016abc5e5f, 0x91ce1a9a3d2cda62},  /* 281 */
	{0xdc5c5301c56b75f7, 0x7641a140cc7810fb},  /* 282 */
	{0x89b9b3e11b6329ba, 0xa9e904c87fcb0a9d},  /* 283 */
	{0xac2820d9623bf429, 0x546345fa9fbdcd44},  /* 284 */
	{0xd732290fbacaf133, 0xa97c177947ad4095},  /* 285 */
	{0x867f59a9d4bed6c0, 0x49ed8eabcccc485d},  /* 286 */
	{0xa81f301449ee8c70, 0x5c68f256bfff5a74},  /* 287 */
	{0xd226fc195c6a2f8c, 0x73832eec6fff3111},  /* 288 */
	{0x83585d8fd9c25db7, 0xc831fd53c5ff7eab},  /* 289 */
	{0xa42e74f3d032f525, 0xba3e7ca8b77f5e55},  /* 290 */
	{0xcd3a1230c43fb26f, 0x28ce1bd2e55f35eb},  /* 291 */
	{0x80444b5e7aa7cf85, 0x7980d163cf5b81b3},  /* 292 */
	{0xa0555e361951c366, 0xd7e105bcc332621f},  /* 293 */
	{0xc86ab5c39fa63440, 0x8dd9472bf3fefaa7},  /* 294 */
	{0xfa856334878fc150, 0xb14f98f6f0feb951},  /* 295 */
	{0x9c935e00d4b9d8d2, 0x6ed1bf9a569f33d3},  /* 296 */
	{0xc3b8358109e84f07, 0x0a862f80ec4700c8},  /* 297 */
	{0xf4a642e14c6262c8, 0xcd27bb612758c0fa},  /* 298 */
	{0x98e7e9cccfbd7dbd, 0x8038d51cb897789c},  /* 299 */
	{0xbf21e44003acdd2c, 0xe0470a63e6bd56c3},  /* 300 */
	{0xeeea5d5004981478, 0x1858ccfce06cac74},  /* 301 */
	{0x95527a5202df0ccb, 0x0f37801e0c43ebc8},  /* 302 */
	{0xbaa718e68396cffd, 0xd30560258f54e6ba},  /* 303 */
	{0xe950df20247c83fd, 0x47c6b82ef32a2069},  /* 304 */
	{0x91d28b7416cdd27e, 0x4cdc331d57fa5441},  /* 305 */
	{0xb6472e511c81471d, 0xe0133fe4adf8e952},  /* 306 */
	{0xe3d8f9e563a198e5, 0x58180fddd97723a6},  /* 307 */
	{0x8e679c2f5e44ff8f, 0x570f09eaa7ea7648}   /* 308 */
};

static const uint64 (*powersoffive128)[2] = powersoffive128_;

