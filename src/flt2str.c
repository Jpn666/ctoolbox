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

#include <ctoolbox/flt2str.h>

/*
 * Based on:
 * "The Schubfach way to render doubles" by Raffaello Giulietti
 * https://drive.google.com/open?id=1luHhyQF9zKlM8yJ1nebU0OgVYhfC6CBN

 * Ported from the reference implementation with some modifications taken
 * from https://github.com/abolz/Drachennest by Alexander Bolz
*/


#if defined(AUTOINCLUDE_1)


CTB_INLINE struct TVal128
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

CTB_INLINE uint64
mul128hi(uint64 lhs, uint64 rhs)
{
	return mul64to128(lhs, rhs).hi;
}

CTB_INLINE struct TVal128
getgs(int32 k)
{
	int32 index = (k - (-292));
	return gtable[index];
}

CTB_INLINE int32
floorlog2pow10(int32 e) {
	/* floor(e * log(10) div log(2)) */
	return ((((int32) e) * 1741647) >> 19);
}

CTB_INLINE int32
floorlog10pow2(int32 e) {
	/* floor(e * log(2) div log(10)) */
	return ((((int32) e) * 1262611) >> 22);
}

CTB_INLINE int32
floorlog10threequaterspow2(int32 e)
{
	/* floor((e * (log(2) div log(10)) + log10(3 div 4)) */
	return ((((int32) e) * 1262611 - 524031) >> 22);
}


/* */
struct TResult {
	int64 mantissa;
	int64 exponent;
};

CTB_INLINE uintxx
setnanorinf(uint8* s, uint64 mantissa)
{
	if (mantissa) {
		s[0] = 'n';
		s[1] = 'a';
		s[2] = 'n';
	}
	else {
		s[0] = 'i';
		s[1] = 'n';
		s[2] = 'f';
	}

	s[3] = 0x00;
	return 3;
}


#define MODE_FLT64 0
#define MODE_FLT32 1
#define FLT64_MAXDIGITS 0x11
#define FLT32_MAXDIGITS 0x09


static uint8* formatG(struct TResult, uintxx, uint8*);
static uint8* formatE(struct TResult, uintxx, uint8*, uintxx);
static uint8* formatD(struct TResult, uint8*, uintxx);


/*
 * Float 64 */

CTB_INLINE int64
roundtoodd64(struct TVal128 g, uint64 cp)
{
	struct TVal128 x;
	struct TVal128 y;

	/*
	 * Computes rop(cp g 2^(-127)), where g = g1 2^63 + g0
	 * See section 9.10 and figure 5 of [1]. */
	x = mul64to128(g.lo, cp);
	y = mul64to128(g.hi, cp);

	y.lo += x.hi;
	if (y.lo < x.hi) {
		y.hi++;
	}
	return (int64) (y.hi | (y.lo > 1));
}

static struct TResult
todecimal64(int32 q, uint64 c)
{
	int32 k;
	int64 h;
	uint64 cb, cbr, cbl;
	int64  vb, vbr, vbl;
	int64 s;
	uint64 lower;
	uint64 upper;
	uint64 m;
	bool iseven;
	bool win;
	bool uin;
	struct TVal128 g;

	/*
	 * The skeleton corresponds to figure 4 of [1].
	 * The efficient computations are those summarized in figure 7. */
	iseven = (c & 0x01) == 0;

	cbr = (cb = (c << 2)) + 2;
	if ((c != (1ull << 52)) || q == -1074) {
		/* Regular spacing */
		cbl = cb - 2;
		k = floorlog10pow2(q);
	}
	else {
		/* Irregular spacing */
		cbl = cb - 1;
		k = floorlog10threequaterspow2(q);
	}
	h = q + floorlog2pow10(-k) + 1;

	/* g1 and g0 are as in section 9.9.3 of [1], so g = g1 2^63 + g0 */
	g = getgs(-k);

	vbl = roundtoodd64(g, cbl << h);
	vb  = roundtoodd64(g, cb  << h);
	vbr = roundtoodd64(g, cbr << h);

	lower = (uint64) vbl;
	upper = (uint64) vbr;
	if (iseven == 0) {
		lower += 1;
		upper -= 1;
	}

	s = vb >> 2;
	if (s >= 10) {
		uint64 sp;
		bool upin;
		bool wpin;

		/* See section 9.4 of [1]. */
		sp = mul128hi(7378697629483820647ull, (uint64) s) >> 2;  /* s div 10 */

		upin = lower          <= (40 * sp);
		wpin = (40 * sp + 40) <= upper;
		if (upin != wpin) {
			return (struct TResult) { (int64) sp + wpin, k + 1};
		}
	}

	/* See section 9.4 of [1]. */
	uin = lower              <= 4 * (uint64) s;
	win = 4 * (uint64) s + 4 <= upper;
	if (uin != win) {
		return (struct TResult) { s + win, k};
	}

	/*
	 * Both u and w lie in Rv: determine the one closest to v.
	 * See section 9.4 of [1]. */
	m = 4 * (uint64) s + 2;
	if ((uint64) vb > m || ((uint64) vb == m && (s & 1) != 0)) {
		s++;
	}
	return (struct TResult) { s, k};
}

static struct TResult
schubfach64(uint64 mantissa, int32 exponent)
{
	if (exponent != 0) {
		mantissa = mantissa | (1ull << 52);
		exponent = exponent - 1075;

		if (0 <= -exponent && - exponent < 53) {
			uint64 f;

			f = mantissa >> -exponent;
			if ((f << -exponent) == mantissa) {
				return (struct TResult) { (int64) f, 0};
			}
		}
	}
	else {
		exponent = -1074;
	}
	return todecimal64(exponent, mantissa);
}

uintxx
f64tostr(flt64 number, eFLTFormatMode m, uintxx precision, uint8 r[24])
{
	union TBinary64 {
		int64 i;
		flt64 f;
	}
	f;
	int64 mantissa;
	int64 exponent;
	uint8* s;
	struct TResult result;

	CTB_ASSERT(r);
	f.f = number;
	exponent = ((uint64) f.i >> 52) & 0x00000000000007ffull;
	mantissa = ((uint64) f.i >> 00) & 0x000fffffffffffffull;

	s = r;
	if (exponent == 2047) {
		if (f.i >> 63)
			*s++ = '-';
		s = s + setnanorinf(s, (uint64) mantissa);
		return (uintxx) (s - r);
	}
	else {
		if (exponent == 0 && mantissa == 0) {
			if (f.i >> 63)
				*s++ = '-';
			s[0] = 0x30;
			s[1] = 0x00;
			s += 1;
			return (uintxx) (s - r);
		}
	}

	result = schubfach64((uint64) mantissa, (int32) exponent);

	/* Ajust the precision parameter */
	if (precision > FLT64_MAXDIGITS)
		precision = FLT64_MAXDIGITS;

	switch (m) {
		case FLTF_MODEG:
			if (f.i >> 63)
				*s++ = '-';
			s = formatG(result, precision, s);
			break;
		case FLTF_MODEE:
			if (f.i >> 63)
				*s++ = '-';
			s = formatE(result, precision, s, MODE_FLT64);
			break;
		case FLTF_MODED:
			if (f.i >> 63)
				*s++ = '-';
			s = formatD(result, s, MODE_FLT64);
			break;
	}

	s[0] = 0x00;
	return (uintxx) (s - r);
}

/*
 * Float 32 */

CTB_INLINE int64
roundtoodd32(struct TVal128 g, uint64 cp)
{
	struct TVal128 m;
	uint32 y0;
	uint32 y1;

	/*
	* Computes rop(cp g 2^(-95))
	* See appendix and figure 8 of [1]. */
	m = mul64to128(g.hi, cp);

	y0 = (uint32) (m.lo >> 0x20);
	y1 = (uint32) (m.hi >> 0x00);
	return (int64) (y1 | (y0 > 1));
}

static struct TResult
todecimal32(int32 q, uint64 c)
{
	int32 k;
	int64 h;
	uint64 cb, cbr, cbl;
	int64  vb, vbr, vbl;
	uint32 s;
	uint64 lower;
	uint64 upper;
	uint64 m;
	bool iseven;
	bool win;
	bool uin;
	struct TVal128 g;

	/*
	 * The skeleton corresponds to figure 4 of [1].
	 * The efficient computations are those summarized in figure 7. */
	iseven = (c & 0x01) == 0;

	cbr = (cb = (c << 2)) + 2;
	if ((c != (1ull << 23)) || q == -149) {
		/* Regular spacing */
		cbl = cb - 2;
		k = floorlog10pow2(q);
	}
	else {
		/* Irregular spacing */
		cbl = cb - 1;
		k = floorlog10threequaterspow2(q);
	}
	h = q + floorlog2pow10(-k) + 1;

	/* g is as in the appendix */
	g = getgs(-k);
	g.hi++;

	vbl = roundtoodd32(g, cbl << h);
	vb  = roundtoodd32(g, cb  << h);
	vbr = roundtoodd32(g, cbr << h);

	lower = (uint64) vbl;
	upper = (uint64) vbr;
	if (iseven == 0) {
		lower += 1;
		upper -= 1;
	}

	s = (uint32) (vb >> 2);
	if (s >= 10) {
		uint64 sp;
		bool upin;
		bool wpin;

		/* See section 9.4 of [1]. */
		sp = mul128hi(7378697629483820647ull, s) >> 2;  /* s div 10 */

		upin = lower          <= (40 * sp);
		wpin = (40 * sp + 40) <= upper;
		if (upin != wpin) {
			return (struct TResult) { (int64) sp + wpin, k + 1};
		}
	}

	/* See section 9.4 of [1]. */
	uin = lower     <= 4 * s;
	win = 4 * s + 4 <= upper;
	if (uin != win) {
		return (struct TResult) {s + win, k};
	}

	/*
	 * Both u and w lie in Rv: determine the one closest to v.
	 * See section 9.4 of [1]. */
	m = 4 * s + 2;
	if ((uint64) vb > m || ((uint64) vb == m && (s & 1) != 0)) {
		s++;
	}
	return (struct TResult) { s, k};
}

static struct TResult
schubfach32(uint64 mantissa, int32 exponent)
{
	if (exponent != 0) {
		mantissa = mantissa | (1ull << 23);
		exponent = exponent - 150;
		if (0 <= -exponent && -exponent < 24) {
			uint64 f;

			f = mantissa >> -exponent;
			if ((f << -exponent) == mantissa) {
				return (struct TResult) { (int64) f, 0};
			}
		}
	}
	else {
		exponent = -149;
	}
	return todecimal32(exponent, mantissa);
}

uintxx
f32tostr(flt32 number, eFLTFormatMode m, uintxx precision, uint8 r[24])
{
	/* */
	union TBinary32 {
		int32 i;
		flt32 f;
	}
	f;
	int32 mantissa;
	int32 exponent;
	uint8* s;
	struct TResult result;

	CTB_ASSERT(r);
	f.f = number;
	exponent = ((uint32) f.i >> 23) & 0x000000ffull;
	mantissa = ((uint32) f.i >> 00) & 0x007fffffull;

	s = r;
	if (exponent == 255) {
		if (f.i >> 31)
			*s++ = '-';
		s = s + setnanorinf(r, (uint64) mantissa);
		return (uintxx) (s - r);
	}
	else {
		if (exponent == 0 && mantissa == 0) {
			if (f.i >> 31)
				*s++ = '-';
			s[0] = 0x30;
			s[1] = 0x00;
			s += 1;
			return (uintxx) (s - r);
		}
	}

	result = schubfach32((uint64) mantissa, exponent);

	/* Ajust the precision parameter */
	if (precision > FLT32_MAXDIGITS)
		precision = FLT32_MAXDIGITS;

	switch (m) {
		case FLTF_MODEG:
			if (f.i >> 31)
				*s++ = '-';
			s = formatG(result, precision, s);
			break;
		case FLTF_MODEE:
			if (f.i >> 31)
				*s++ = '-';
			s = formatE(result, precision, s, MODE_FLT32);
			break;
		case FLTF_MODED:
			if (f.i >> 31)
				*s++ = '-';
			s = formatD(result, s, MODE_FLT32);
			break;
	}

	s[0] = 0x00;
	return (uintxx) (s - r);
}


/* ****************************************************************************
 * Formatting
 *************************************************************************** */

CTB_INLINE bool
shouldroundup(uint8* p1, uint8* p2)
{
	if (p1 == p2) {
		return 0;
	}

	if (p1[0] >= 0x35) {
		if (p1[0] ^ 0x35) {
			return 1;
		}
	}
	else {
		return 0;
	}

	p1++;
	for (; p1 < p2; p1++) {
		if (p1[0] ^ 0x30) {
			return 1;
		}
	}
	return 0;
}

CTB_INLINE int32
incrementrepresentation(uint8* p1, uint8* p2, int32 e10)
{
	uint8* p3;

	for (p3 = p2 - 1; p3 > p1; p3--) {
		if (p3[0] != 0x39) {
			break;
		}
	}

	if (p1 == p3) {
		if (p1[0] == 0x39) {
			p1[0] = 0x31;
			p1++;
			while (p1 < p2) {
				*p1++ = 0x30;
			}
			return e10 + 1;
		}
	}

	p3[0] += 1;
	p3++;
	while (p3 < p2) {
		*p3++ = 0x30;
	}
	return e10;
}

static uint8*
appendexponent(uint8* s, int32 exponent)
{
	int32 a;
	int32 b;
	int32 c;

	*s++ = 'e';
	if (exponent < 0) {
		*s++ = '-';
		exponent = -exponent;
	}
	else {
		*s++ = '+';
	}

	c = exponent;
	a = ( 656 * c) >> 16;
	b = (6554 * c) >> 16;

	c -= b * 10;
	b -= a * 10;
	if (a) {
		*s++ = (uint8) a + 0x30;
		*s++ = (uint8) b + 0x30;
	}
	else {
		*s++ = (uint8) b + 0x30;
	}
	*s++ = (uint8) c + 0x30;

	s[0] = 0x00;
	return s;
}


#if defined(CTB_ENV64)

union TZeroUnion {
	uint64 asuint[ 5];
	uint8  buffer[40];
};

#else

union TZeroUnion {
	uint32 asuint[10];
	uint8  buffer[40];
};

#endif


static const uint8 radix100[] = {
	'0', '0', '0', '1', '0', '2', '0', '3', '0', '4',
	'0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
	'1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
	'1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
	'2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
	'2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
	'3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
	'3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
	'4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
	'4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
	'5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
	'5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
	'6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
	'6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
	'7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
	'7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
	'8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
	'8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
	'9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
	'9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
};

static uint8*
todigits(uint32 number, uint8* buffer)
{
	uint64 c;
	uint32 r;
	uint64 n;
	uint8* p;

	p = buffer;
	for (c = number; c >= 100; ) {
		p -= 2;
		r = (uint32) (c - ((n = ((42949673 * c) >> 32)) * 100));
		c = n;
		p[0] = radix100[(r << 1) + 0];
		p[1] = radix100[(r << 1) + 1];
	}

	if (c >= 10) {
		p -= 2;
		p[0] = radix100[(c << 1) + 0];
		p[1] = radix100[(c << 1) + 1];
	}
	else {
		p -= 1;
		p[0] = (uint8) (0x30 + c);
	}

	return p;
}

static uint8*
i64tostr(int64 n, union TZeroUnion* z, int32* magnitude)
{
	uint8* s1;
	uint8* s2;
	uint64 a;
	uint64 b;

	/* */
#if defined(CTB_ENV64)
	z[0].asuint[1] = 0x3030303030303030ull;
	z[0].asuint[2] = 0x3030303030303030ull;
	z[0].asuint[3] = 0x3030303030303030ull;
	z[0].asuint[4] = 0x3030303030303030ull;
#else
	z[0].asuint[2] = 0x30303030ul;
	z[0].asuint[3] = 0x30303030ul;
	z[0].asuint[4] = 0x30303030ul;
	z[0].asuint[5] = 0x30303030ul;
	z[0].asuint[6] = 0x30303030ul;
	z[0].asuint[7] = 0x30303030ul;
	z[0].asuint[8] = 0x30303030ul;
	z[0].asuint[9] = 0x30303030ul;
#endif

	s1 = z[0].buffer + 20;
	if (n < 1000000000u) {
		s2 = todigits((uint32) n, s1);

		magnitude[0] = (int32) (s1 - s2);
		return s2;
	}

	a = mul128hi(19342813113834068ull, (uint64) n) >> 20;
	b = (uint64) n - (a * 1000000000u);

	     todigits((uint32) b, s1 - 0);
	s2 = todigits((uint32) a, s1 - 9);

	magnitude[0] = (int32) (s1 - s2);
	return s2;
}


#if defined(__clang__) && defined(CTB_FASTUNALIGNED)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcast-align"
#endif

static uint8*
formatD(struct TResult result, uint8* s, uintxx mode)
{
	int32 magnitude;
	uint8* pb1;
	union TZeroUnion z;
#if !defined(CTB_FASTUNALIGNED)
	uintxx j;
	uintxx m;
#endif

	pb1 = i64tostr(result.mantissa, &z, &magnitude);
	*s++ = *pb1++;
	*s++ = '.';

#if defined(CTB_FASTUNALIGNED)
#if defined(CTB_ENV64)
	if (mode == MODE_FLT64) {
		((uint64*) s)[0] = ((uint64*) pb1)[0];
		((uint64*) s)[1] = ((uint64*) pb1)[1];
		s += 0x10;
	}
	else {
		((uint64*) s)[0] = ((uint64*) pb1)[0];
		s += 0x08;
	}
#else
	if (mode == MODE_FLT64) {
		((uint32*) s)[0] = ((uint32*) pb1)[0];
		((uint32*) s)[1] = ((uint32*) pb1)[1];
		((uint32*) s)[2] = ((uint32*) pb1)[2];
		((uint32*) s)[3] = ((uint32*) pb1)[3];
		s += 0x10;
	}
	else {
		((uint32*) s)[0] = ((uint32*) pb1)[0];
		((uint32*) s)[1] = ((uint32*) pb1)[1];
		s += 0x08;
	}
#endif

#else
	m = 8;
	if (mode == MODE_FLT64)
		m += 8;
	for (j = 0; j < m; j++) {
		*s++ = pb1[j];
	}
#endif

	s = appendexponent(s, (int32) result.exponent + (magnitude - 1));
	return s;
}

#if defined(__clang__) && defined(CTB_FASTUNALIGNED)
	#pragma clang diagnostic pop
#endif


static uint8*
formatE(struct TResult result, uintxx precision, uint8* s, uintxx mode)
{
	int32 e10;
	uint8* pb1;
	uint8* pb2;
	uint8* end;
	int32 magnitude;
	union TZeroUnion z;

	switch (mode) {
		case MODE_FLT64:
			if (precision == FLT64_MAXDIGITS)
				return formatD(result, s, mode);
			break;
		case MODE_FLT32:
			if (precision == FLT32_MAXDIGITS)
				return formatD(result, s, mode);
			break;
	}

	pb1 = i64tostr(result.mantissa, &z, &magnitude);
	pb2 = pb1;
	end = pb1 + magnitude;

	e10 = (int32) result.exponent + (magnitude - 1);
	precision++;

	pb2 += precision;
	if (shouldroundup(pb2, end)) {
		e10 = incrementrepresentation(pb1, pb2, e10);
	}

	*s++ = pb1[0];
	if (pb2 - pb1 >= 1 && precision > 1) {
		*s++ = '.';

		pb1++;
		while (pb2 > pb1) {
			*s++ = *pb1++;
		}
	}

	s = appendexponent(s, e10);
	return s;
}

CTB_INLINE uint8*
removetrailingzeros(uint8* s)
{
	while (s[-1] == 0x30) {
		s--;
	}
	return s;
}

static uint8*
formatG(struct TResult result, uintxx precision, uint8* s)
{
	int32 e10;
	int32 j;
	int32 p;
	uint8* pb1;
	uint8* pb2;
	uint8* end;
	int32 magnitude;
	union TZeroUnion z;

	if (precision == 0)
		precision = 6;
	p = (int32) precision;


	pb1 = i64tostr(result.mantissa, &z, &magnitude);
	pb2 = pb1;
	end = pb1 + magnitude;

	e10 = (int32) result.exponent + (magnitude - 1);
	if (e10 >= -4 && (int32) precision > e10) {
		/* F mode */
		int32 decimalpoint;

		decimalpoint = magnitude + (int32) (result.exponent - 1);
		p -= e10 - 1;
		if (decimalpoint >= 0) {
			int32 r;

			r = (int32) precision;
			if (p == 0)
				r = decimalpoint + 1;

			pb2 += r;
			if (shouldroundup(pb2, end)) {
				e10 = incrementrepresentation(pb1, pb2, e10);
			}

			for (j = 0; j <= decimalpoint; j++) {
				*s++ = *pb1++;
			}
			if (p != 0) {
				*s++ = '.';
				while (pb2 > pb1) {
					*s++ = *pb1++;
				}
			}
		}
		else {
			/* Negative exponent */
			pb2 += precision;
			if (shouldroundup(pb2, end)) {
				e10 = incrementrepresentation(pb1, pb2, e10);
			}

			*s++ = 0x30;
			*s++ = '.';
			for (j = -decimalpoint - 1; j; j--) {
				*s++ = 0x30;
			}
			while (pb1 < pb2) {
				*s++ = *pb1++;
			}
		}

		s = removetrailingzeros(s);
		if (s[-1] == '.') {
			s--;
		}
	}
	else {
		/* E mode */
		if (end - pb1 > p) {
			pb2 += p + 1;
			if (shouldroundup(pb2, end)) {
				e10 = incrementrepresentation(pb1, pb2, e10);
			}
		}
		else {
			pb2 = end;
		}

		*s++ = *pb1++;
		if (pb2 - pb1 > 0) {
			*s++ = '.';
			while (pb1 < pb2) {
				*s++ = *pb1++;
			}
		}

		s = removetrailingzeros(s);
		if (s[-1] == '.') {
			s--;
		}

		s = appendexponent(s, e10);
	}
	return s;
}

#else

/* */
struct TVal128 {
	uint64 hi;
	uint64 lo;
};


/* ****************************************************************************
 * Tables
 *************************************************************************** */

/*
* The precomputed values for g1(int) and g0(int).
* The first entry must be for an exponent of K_MIN or less.
* The last entry must be for an exponent of K_MAX or more. */

static const struct TVal128 gtable[] = {
	{0xFF77B1FCBEBCDC4Full, 0x25E8E89C13BB0F7Bull}, /* -292 */
	{0x9FAACF3DF73609B1ull, 0x77B191618C54E9ADull}, /* -291 */
	{0xC795830D75038C1Dull, 0xD59DF5B9EF6A2418ull}, /* -290 */
	{0xF97AE3D0D2446F25ull, 0x4B0573286B44AD1Eull}, /* -289 */
	{0x9BECCE62836AC577ull, 0x4EE367F9430AEC33ull}, /* -288 */
	{0xC2E801FB244576D5ull, 0x229C41F793CDA740ull}, /* -287 */
	{0xF3A20279ED56D48Aull, 0x6B43527578C11110ull}, /* -286 */
	{0x9845418C345644D6ull, 0x830A13896B78AAAAull}, /* -285 */
	{0xBE5691EF416BD60Cull, 0x23CC986BC656D554ull}, /* -284 */
	{0xEDEC366B11C6CB8Full, 0x2CBFBE86B7EC8AA9ull}, /* -283 */
	{0x94B3A202EB1C3F39ull, 0x7BF7D71432F3D6AAull}, /* -282 */
	{0xB9E08A83A5E34F07ull, 0xDAF5CCD93FB0CC54ull}, /* -281 */
	{0xE858AD248F5C22C9ull, 0xD1B3400F8F9CFF69ull}, /* -280 */
	{0x91376C36D99995BEull, 0x23100809B9C21FA2ull}, /* -279 */
	{0xB58547448FFFFB2Dull, 0xABD40A0C2832A78Bull}, /* -278 */
	{0xE2E69915B3FFF9F9ull, 0x16C90C8F323F516Dull}, /* -277 */
	{0x8DD01FAD907FFC3Bull, 0xAE3DA7D97F6792E4ull}, /* -276 */
	{0xB1442798F49FFB4Aull, 0x99CD11CFDF41779Dull}, /* -275 */
	{0xDD95317F31C7FA1Dull, 0x40405643D711D584ull}, /* -274 */
	{0x8A7D3EEF7F1CFC52ull, 0x482835EA666B2573ull}, /* -273 */
	{0xAD1C8EAB5EE43B66ull, 0xDA3243650005EED0ull}, /* -272 */
	{0xD863B256369D4A40ull, 0x90BED43E40076A83ull}, /* -271 */
	{0x873E4F75E2224E68ull, 0x5A7744A6E804A292ull}, /* -270 */
	{0xA90DE3535AAAE202ull, 0x711515D0A205CB37ull}, /* -269 */
	{0xD3515C2831559A83ull, 0x0D5A5B44CA873E04ull}, /* -268 */
	{0x8412D9991ED58091ull, 0xE858790AFE9486C3ull}, /* -267 */
	{0xA5178FFF668AE0B6ull, 0x626E974DBE39A873ull}, /* -266 */
	{0xCE5D73FF402D98E3ull, 0xFB0A3D212DC81290ull}, /* -265 */
	{0x80FA687F881C7F8Eull, 0x7CE66634BC9D0B9Aull}, /* -264 */
	{0xA139029F6A239F72ull, 0x1C1FFFC1EBC44E81ull}, /* -263 */
	{0xC987434744AC874Eull, 0xA327FFB266B56221ull}, /* -262 */
	{0xFBE9141915D7A922ull, 0x4BF1FF9F0062BAA9ull}, /* -261 */
	{0x9D71AC8FADA6C9B5ull, 0x6F773FC3603DB4AAull}, /* -260 */
	{0xC4CE17B399107C22ull, 0xCB550FB4384D21D4ull}, /* -259 */
	{0xF6019DA07F549B2Bull, 0x7E2A53A146606A49ull}, /* -258 */
	{0x99C102844F94E0FBull, 0x2EDA7444CBFC426Eull}, /* -257 */
	{0xC0314325637A1939ull, 0xFA911155FEFB5309ull}, /* -256 */
	{0xF03D93EEBC589F88ull, 0x793555AB7EBA27CBull}, /* -255 */
	{0x96267C7535B763B5ull, 0x4BC1558B2F3458DFull}, /* -254 */
	{0xBBB01B9283253CA2ull, 0x9EB1AAEDFB016F17ull}, /* -253 */
	{0xEA9C227723EE8BCBull, 0x465E15A979C1CADDull}, /* -252 */
	{0x92A1958A7675175Full, 0x0BFACD89EC191ECAull}, /* -251 */
	{0xB749FAED14125D36ull, 0xCEF980EC671F667Cull}, /* -250 */
	{0xE51C79A85916F484ull, 0x82B7E12780E7401Bull}, /* -249 */
	{0x8F31CC0937AE58D2ull, 0xD1B2ECB8B0908811ull}, /* -248 */
	{0xB2FE3F0B8599EF07ull, 0x861FA7E6DCB4AA16ull}, /* -247 */
	{0xDFBDCECE67006AC9ull, 0x67A791E093E1D49Bull}, /* -246 */
	{0x8BD6A141006042BDull, 0xE0C8BB2C5C6D24E1ull}, /* -245 */
	{0xAECC49914078536Dull, 0x58FAE9F773886E19ull}, /* -244 */
	{0xDA7F5BF590966848ull, 0xAF39A475506A899Full}, /* -243 */
	{0x888F99797A5E012Dull, 0x6D8406C952429604ull}, /* -242 */
	{0xAAB37FD7D8F58178ull, 0xC8E5087BA6D33B84ull}, /* -241 */
	{0xD5605FCDCF32E1D6ull, 0xFB1E4A9A90880A65ull}, /* -240 */
	{0x855C3BE0A17FCD26ull, 0x5CF2EEA09A550680ull}, /* -239 */
	{0xA6B34AD8C9DFC06Full, 0xF42FAA48C0EA481Full}, /* -238 */
	{0xD0601D8EFC57B08Bull, 0xF13B94DAF124DA27ull}, /* -237 */
	{0x823C12795DB6CE57ull, 0x76C53D08D6B70859ull}, /* -236 */
	{0xA2CB1717B52481EDull, 0x54768C4B0C64CA6Full}, /* -235 */
	{0xCB7DDCDDA26DA268ull, 0xA9942F5DCF7DFD0Aull}, /* -234 */
	{0xFE5D54150B090B02ull, 0xD3F93B35435D7C4Dull}, /* -233 */
	{0x9EFA548D26E5A6E1ull, 0xC47BC5014A1A6DB0ull}, /* -232 */
	{0xC6B8E9B0709F109Aull, 0x359AB6419CA1091Cull}, /* -231 */
	{0xF867241C8CC6D4C0ull, 0xC30163D203C94B63ull}, /* -230 */
	{0x9B407691D7FC44F8ull, 0x79E0DE63425DCF1Eull}, /* -229 */
	{0xC21094364DFB5636ull, 0x985915FC12F542E5ull}, /* -228 */
	{0xF294B943E17A2BC4ull, 0x3E6F5B7B17B2939Eull}, /* -227 */
	{0x979CF3CA6CEC5B5Aull, 0xA705992CEECF9C43ull}, /* -226 */
	{0xBD8430BD08277231ull, 0x50C6FF782A838354ull}, /* -225 */
	{0xECE53CEC4A314EBDull, 0xA4F8BF5635246429ull}, /* -224 */
	{0x940F4613AE5ED136ull, 0x871B7795E136BE9Aull}, /* -223 */
	{0xB913179899F68584ull, 0x28E2557B59846E40ull}, /* -222 */
	{0xE757DD7EC07426E5ull, 0x331AEADA2FE589D0ull}, /* -221 */
	{0x9096EA6F3848984Full, 0x3FF0D2C85DEF7622ull}, /* -220 */
	{0xB4BCA50B065ABE63ull, 0x0FED077A756B53AAull}, /* -219 */
	{0xE1EBCE4DC7F16DFBull, 0xD3E8495912C62895ull}, /* -218 */
	{0x8D3360F09CF6E4BDull, 0x64712DD7ABBBD95Dull}, /* -217 */
	{0xB080392CC4349DECull, 0xBD8D794D96AACFB4ull}, /* -216 */
	{0xDCA04777F541C567ull, 0xECF0D7A0FC5583A1ull}, /* -215 */
	{0x89E42CAAF9491B60ull, 0xF41686C49DB57245ull}, /* -214 */
	{0xAC5D37D5B79B6239ull, 0x311C2875C522CED6ull}, /* -213 */
	{0xD77485CB25823AC7ull, 0x7D633293366B828Cull}, /* -212 */
	{0x86A8D39EF77164BCull, 0xAE5DFF9C02033198ull}, /* -211 */
	{0xA8530886B54DBDEBull, 0xD9F57F830283FDFDull}, /* -210 */
	{0xD267CAA862A12D66ull, 0xD072DF63C324FD7Cull}, /* -209 */
	{0x8380DEA93DA4BC60ull, 0x4247CB9E59F71E6Eull}, /* -208 */
	{0xA46116538D0DEB78ull, 0x52D9BE85F074E609ull}, /* -207 */
	{0xCD795BE870516656ull, 0x67902E276C921F8Cull}, /* -206 */
	{0x806BD9714632DFF6ull, 0x00BA1CD8A3DB53B7ull}, /* -205 */
	{0xA086CFCD97BF97F3ull, 0x80E8A40ECCD228A5ull}, /* -204 */
	{0xC8A883C0FDAF7DF0ull, 0x6122CD128006B2CEull}, /* -203 */
	{0xFAD2A4B13D1B5D6Cull, 0x796B805720085F82ull}, /* -202 */
	{0x9CC3A6EEC6311A63ull, 0xCBE3303674053BB1ull}, /* -201 */
	{0xC3F490AA77BD60FCull, 0xBEDBFC4411068A9Dull}, /* -200 */
	{0xF4F1B4D515ACB93Bull, 0xEE92FB5515482D45ull}, /* -199 */
	{0x991711052D8BF3C5ull, 0x751BDD152D4D1C4Bull}, /* -198 */
	{0xBF5CD54678EEF0B6ull, 0xD262D45A78A0635Eull}, /* -197 */
	{0xEF340A98172AACE4ull, 0x86FB897116C87C35ull}, /* -196 */
	{0x9580869F0E7AAC0Eull, 0xD45D35E6AE3D4DA1ull}, /* -195 */
	{0xBAE0A846D2195712ull, 0x8974836059CCA10Aull}, /* -194 */
	{0xE998D258869FACD7ull, 0x2BD1A438703FC94Cull}, /* -193 */
	{0x91FF83775423CC06ull, 0x7B6306A34627DDD0ull}, /* -192 */
	{0xB67F6455292CBF08ull, 0x1A3BC84C17B1D543ull}, /* -191 */
	{0xE41F3D6A7377EECAull, 0x20CABA5F1D9E4A94ull}, /* -190 */
	{0x8E938662882AF53Eull, 0x547EB47B7282EE9Dull}, /* -189 */
	{0xB23867FB2A35B28Dull, 0xE99E619A4F23AA44ull}, /* -188 */
	{0xDEC681F9F4C31F31ull, 0x6405FA00E2EC94D5ull}, /* -187 */
	{0x8B3C113C38F9F37Eull, 0xDE83BC408DD3DD05ull}, /* -186 */
	{0xAE0B158B4738705Eull, 0x9624AB50B148D446ull}, /* -185 */
	{0xD98DDAEE19068C76ull, 0x3BADD624DD9B0958ull}, /* -184 */
	{0x87F8A8D4CFA417C9ull, 0xE54CA5D70A80E5D7ull}, /* -183 */
	{0xA9F6D30A038D1DBCull, 0x5E9FCF4CCD211F4Dull}, /* -182 */
	{0xD47487CC8470652Bull, 0x7647C32000696720ull}, /* -181 */
	{0x84C8D4DFD2C63F3Bull, 0x29ECD9F40041E074ull}, /* -180 */
	{0xA5FB0A17C777CF09ull, 0xF468107100525891ull}, /* -179 */
	{0xCF79CC9DB955C2CCull, 0x7182148D4066EEB5ull}, /* -178 */
	{0x81AC1FE293D599BFull, 0xC6F14CD848405531ull}, /* -177 */
	{0xA21727DB38CB002Full, 0xB8ADA00E5A506A7Dull}, /* -176 */
	{0xCA9CF1D206FDC03Bull, 0xA6D90811F0E4851Dull}, /* -175 */
	{0xFD442E4688BD304Aull, 0x908F4A166D1DA664ull}, /* -174 */
	{0x9E4A9CEC15763E2Eull, 0x9A598E4E043287FFull}, /* -173 */
	{0xC5DD44271AD3CDBAull, 0x40EFF1E1853F29FEull}, /* -172 */
	{0xF7549530E188C128ull, 0xD12BEE59E68EF47Dull}, /* -171 */
	{0x9A94DD3E8CF578B9ull, 0x82BB74F8301958CFull}, /* -170 */
	{0xC13A148E3032D6E7ull, 0xE36A52363C1FAF02ull}, /* -169 */
	{0xF18899B1BC3F8CA1ull, 0xDC44E6C3CB279AC2ull}, /* -168 */
	{0x96F5600F15A7B7E5ull, 0x29AB103A5EF8C0BAull}, /* -167 */
	{0xBCB2B812DB11A5DEull, 0x7415D448F6B6F0E8ull}, /* -166 */
	{0xEBDF661791D60F56ull, 0x111B495B3464AD22ull}, /* -165 */
	{0x936B9FCEBB25C995ull, 0xCAB10DD900BEEC35ull}, /* -164 */
	{0xB84687C269EF3BFBull, 0x3D5D514F40EEA743ull}, /* -163 */
	{0xE65829B3046B0AFAull, 0x0CB4A5A3112A5113ull}, /* -162 */
	{0x8FF71A0FE2C2E6DCull, 0x47F0E785EABA72ACull}, /* -161 */
	{0xB3F4E093DB73A093ull, 0x59ED216765690F57ull}, /* -160 */
	{0xE0F218B8D25088B8ull, 0x306869C13EC3532Dull}, /* -159 */
	{0x8C974F7383725573ull, 0x1E414218C73A13FCull}, /* -158 */
	{0xAFBD2350644EEACFull, 0xE5D1929EF90898FBull}, /* -157 */
	{0xDBAC6C247D62A583ull, 0xDF45F746B74ABF3Aull}, /* -156 */
	{0x894BC396CE5DA772ull, 0x6B8BBA8C328EB784ull}, /* -155 */
	{0xAB9EB47C81F5114Full, 0x066EA92F3F326565ull}, /* -154 */
	{0xD686619BA27255A2ull, 0xC80A537B0EFEFEBEull}, /* -153 */
	{0x8613FD0145877585ull, 0xBD06742CE95F5F37ull}, /* -152 */
	{0xA798FC4196E952E7ull, 0x2C48113823B73705ull}, /* -151 */
	{0xD17F3B51FCA3A7A0ull, 0xF75A15862CA504C6ull}, /* -150 */
	{0x82EF85133DE648C4ull, 0x9A984D73DBE722FCull}, /* -149 */
	{0xA3AB66580D5FDAF5ull, 0xC13E60D0D2E0EBBBull}, /* -148 */
	{0xCC963FEE10B7D1B3ull, 0x318DF905079926A9ull}, /* -147 */
	{0xFFBBCFE994E5C61Full, 0xFDF17746497F7053ull}, /* -146 */
	{0x9FD561F1FD0F9BD3ull, 0xFEB6EA8BEDEFA634ull}, /* -145 */
	{0xC7CABA6E7C5382C8ull, 0xFE64A52EE96B8FC1ull}, /* -144 */
	{0xF9BD690A1B68637Bull, 0x3DFDCE7AA3C673B1ull}, /* -143 */
	{0x9C1661A651213E2Dull, 0x06BEA10CA65C084Full}, /* -142 */
	{0xC31BFA0FE5698DB8ull, 0x486E494FCFF30A63ull}, /* -141 */
	{0xF3E2F893DEC3F126ull, 0x5A89DBA3C3EFCCFBull}, /* -140 */
	{0x986DDB5C6B3A76B7ull, 0xF89629465A75E01Dull}, /* -139 */
	{0xBE89523386091465ull, 0xF6BBB397F1135824ull}, /* -138 */
	{0xEE2BA6C0678B597Full, 0x746AA07DED582E2Dull}, /* -137 */
	{0x94DB483840B717EFull, 0xA8C2A44EB4571CDDull}, /* -136 */
	{0xBA121A4650E4DDEBull, 0x92F34D62616CE414ull}, /* -135 */
	{0xE896A0D7E51E1566ull, 0x77B020BAF9C81D18ull}, /* -134 */
	{0x915E2486EF32CD60ull, 0x0ACE1474DC1D122Full}, /* -133 */
	{0xB5B5ADA8AAFF80B8ull, 0x0D819992132456BBull}, /* -132 */
	{0xE3231912D5BF60E6ull, 0x10E1FFF697ED6C6Aull}, /* -131 */
	{0x8DF5EFABC5979C8Full, 0xCA8D3FFA1EF463C2ull}, /* -130 */
	{0xB1736B96B6FD83B3ull, 0xBD308FF8A6B17CB3ull}, /* -129 */
	{0xDDD0467C64BCE4A0ull, 0xAC7CB3F6D05DDBDFull}, /* -128 */
	{0x8AA22C0DBEF60EE4ull, 0x6BCDF07A423AA96Cull}, /* -127 */
	{0xAD4AB7112EB3929Dull, 0x86C16C98D2C953C7ull}, /* -126 */
	{0xD89D64D57A607744ull, 0xE871C7BF077BA8B8ull}, /* -125 */
	{0x87625F056C7C4A8Bull, 0x11471CD764AD4973ull}, /* -124 */
	{0xA93AF6C6C79B5D2Dull, 0xD598E40D3DD89BD0ull}, /* -123 */
	{0xD389B47879823479ull, 0x4AFF1D108D4EC2C4ull}, /* -122 */
	{0x843610CB4BF160CBull, 0xCEDF722A585139BBull}, /* -121 */
	{0xA54394FE1EEDB8FEull, 0xC2974EB4EE658829ull}, /* -120 */
	{0xCE947A3DA6A9273Eull, 0x733D226229FEEA33ull}, /* -119 */
	{0x811CCC668829B887ull, 0x0806357D5A3F5260ull}, /* -118 */
	{0xA163FF802A3426A8ull, 0xCA07C2DCB0CF26F8ull}, /* -117 */
	{0xC9BCFF6034C13052ull, 0xFC89B393DD02F0B6ull}, /* -116 */
	{0xFC2C3F3841F17C67ull, 0xBBAC2078D443ACE3ull}, /* -115 */
	{0x9D9BA7832936EDC0ull, 0xD54B944B84AA4C0Eull}, /* -114 */
	{0xC5029163F384A931ull, 0x0A9E795E65D4DF12ull}, /* -113 */
	{0xF64335BCF065D37Dull, 0x4D4617B5FF4A16D6ull}, /* -112 */
	{0x99EA0196163FA42Eull, 0x504BCED1BF8E4E46ull}, /* -111 */
	{0xC06481FB9BCF8D39ull, 0xE45EC2862F71E1D7ull}, /* -110 */
	{0xF07DA27A82C37088ull, 0x5D767327BB4E5A4Dull}, /* -109 */
	{0x964E858C91BA2655ull, 0x3A6A07F8D510F870ull}, /* -108 */
	{0xBBE226EFB628AFEAull, 0x890489F70A55368Cull}, /* -107 */
	{0xEADAB0ABA3B2DBE5ull, 0x2B45AC74CCEA842Full}, /* -106 */
	{0x92C8AE6B464FC96Full, 0x3B0B8BC90012929Eull}, /* -105 */
	{0xB77ADA0617E3BBCBull, 0x09CE6EBB40173745ull}, /* -104 */
	{0xE55990879DDCAABDull, 0xCC420A6A101D0516ull}, /* -103 */
	{0x8F57FA54C2A9EAB6ull, 0x9FA946824A12232Eull}, /* -102 */
	{0xB32DF8E9F3546564ull, 0x47939822DC96ABFAull}, /* -101 */
	{0xDFF9772470297EBDull, 0x59787E2B93BC56F8ull}, /* -100 */
	{0x8BFBEA76C619EF36ull, 0x57EB4EDB3C55B65Bull}, /*  -99 */
	{0xAEFAE51477A06B03ull, 0xEDE622920B6B23F2ull}, /*  -98 */
	{0xDAB99E59958885C4ull, 0xE95FAB368E45ECEEull}, /*  -97 */
	{0x88B402F7FD75539Bull, 0x11DBCB0218EBB415ull}, /*  -96 */
	{0xAAE103B5FCD2A881ull, 0xD652BDC29F26A11Aull}, /*  -95 */
	{0xD59944A37C0752A2ull, 0x4BE76D3346F04960ull}, /*  -94 */
	{0x857FCAE62D8493A5ull, 0x6F70A4400C562DDCull}, /*  -93 */
	{0xA6DFBD9FB8E5B88Eull, 0xCB4CCD500F6BB953ull}, /*  -92 */
	{0xD097AD07A71F26B2ull, 0x7E2000A41346A7A8ull}, /*  -91 */
	{0x825ECC24C873782Full, 0x8ED400668C0C28C9ull}, /*  -90 */
	{0xA2F67F2DFA90563Bull, 0x728900802F0F32FBull}, /*  -89 */
	{0xCBB41EF979346BCAull, 0x4F2B40A03AD2FFBAull}, /*  -88 */
	{0xFEA126B7D78186BCull, 0xE2F610C84987BFA9ull}, /*  -87 */
	{0x9F24B832E6B0F436ull, 0x0DD9CA7D2DF4D7CAull}, /*  -86 */
	{0xC6EDE63FA05D3143ull, 0x91503D1C79720DBCull}, /*  -85 */
	{0xF8A95FCF88747D94ull, 0x75A44C6397CE912Bull}, /*  -84 */
	{0x9B69DBE1B548CE7Cull, 0xC986AFBE3EE11ABBull}, /*  -83 */
	{0xC24452DA229B021Bull, 0xFBE85BADCE996169ull}, /*  -82 */
	{0xF2D56790AB41C2A2ull, 0xFAE27299423FB9C4ull}, /*  -81 */
	{0x97C560BA6B0919A5ull, 0xDCCD879FC967D41Bull}, /*  -80 */
	{0xBDB6B8E905CB600Full, 0x5400E987BBC1C921ull}, /*  -79 */
	{0xED246723473E3813ull, 0x290123E9AAB23B69ull}, /*  -78 */
	{0x9436C0760C86E30Bull, 0xF9A0B6720AAF6522ull}, /*  -77 */
	{0xB94470938FA89BCEull, 0xF808E40E8D5B3E6Aull}, /*  -76 */
	{0xE7958CB87392C2C2ull, 0xB60B1D1230B20E05ull}, /*  -75 */
	{0x90BD77F3483BB9B9ull, 0xB1C6F22B5E6F48C3ull}, /*  -74 */
	{0xB4ECD5F01A4AA828ull, 0x1E38AEB6360B1AF4ull}, /*  -73 */
	{0xE2280B6C20DD5232ull, 0x25C6DA63C38DE1B1ull}, /*  -72 */
	{0x8D590723948A535Full, 0x579C487E5A38AD0Full}, /*  -71 */
	{0xB0AF48EC79ACE837ull, 0x2D835A9DF0C6D852ull}, /*  -70 */
	{0xDCDB1B2798182244ull, 0xF8E431456CF88E66ull}, /*  -69 */
	{0x8A08F0F8BF0F156Bull, 0x1B8E9ECB641B5900ull}, /*  -68 */
	{0xAC8B2D36EED2DAC5ull, 0xE272467E3D222F40ull}, /*  -67 */
	{0xD7ADF884AA879177ull, 0x5B0ED81DCC6ABB10ull}, /*  -66 */
	{0x86CCBB52EA94BAEAull, 0x98E947129FC2B4EAull}, /*  -65 */
	{0xA87FEA27A539E9A5ull, 0x3F2398D747B36225ull}, /*  -64 */
	{0xD29FE4B18E88640Eull, 0x8EEC7F0D19A03AAEull}, /*  -63 */
	{0x83A3EEEEF9153E89ull, 0x1953CF68300424ADull}, /*  -62 */
	{0xA48CEAAAB75A8E2Bull, 0x5FA8C3423C052DD8ull}, /*  -61 */
	{0xCDB02555653131B6ull, 0x3792F412CB06794Eull}, /*  -60 */
	{0x808E17555F3EBF11ull, 0xE2BBD88BBEE40BD1ull}, /*  -59 */
	{0xA0B19D2AB70E6ED6ull, 0x5B6ACEAEAE9D0EC5ull}, /*  -58 */
	{0xC8DE047564D20A8Bull, 0xF245825A5A445276ull}, /*  -57 */
	{0xFB158592BE068D2Eull, 0xEED6E2F0F0D56713ull}, /*  -56 */
	{0x9CED737BB6C4183Dull, 0x55464DD69685606Cull}, /*  -55 */
	{0xC428D05AA4751E4Cull, 0xAA97E14C3C26B887ull}, /*  -54 */
	{0xF53304714D9265DFull, 0xD53DD99F4B3066A9ull}, /*  -53 */
	{0x993FE2C6D07B7FABull, 0xE546A8038EFE402Aull}, /*  -52 */
	{0xBF8FDB78849A5F96ull, 0xDE98520472BDD034ull}, /*  -51 */
	{0xEF73D256A5C0F77Cull, 0x963E66858F6D4441ull}, /*  -50 */
	{0x95A8637627989AADull, 0xDDE7001379A44AA9ull}, /*  -49 */
	{0xBB127C53B17EC159ull, 0x5560C018580D5D53ull}, /*  -48 */
	{0xE9D71B689DDE71AFull, 0xAAB8F01E6E10B4A7ull}, /*  -47 */
	{0x9226712162AB070Dull, 0xCAB3961304CA70E9ull}, /*  -46 */
	{0xB6B00D69BB55C8D1ull, 0x3D607B97C5FD0D23ull}, /*  -45 */
	{0xE45C10C42A2B3B05ull, 0x8CB89A7DB77C506Bull}, /*  -44 */
	{0x8EB98A7A9A5B04E3ull, 0x77F3608E92ADB243ull}, /*  -43 */
	{0xB267ED1940F1C61Cull, 0x55F038B237591ED4ull}, /*  -42 */
	{0xDF01E85F912E37A3ull, 0x6B6C46DEC52F6689ull}, /*  -41 */
	{0x8B61313BBABCE2C6ull, 0x2323AC4B3B3DA016ull}, /*  -40 */
	{0xAE397D8AA96C1B77ull, 0xABEC975E0A0D081Bull}, /*  -39 */
	{0xD9C7DCED53C72255ull, 0x96E7BD358C904A22ull}, /*  -38 */
	{0x881CEA14545C7575ull, 0x7E50D64177DA2E55ull}, /*  -37 */
	{0xAA242499697392D2ull, 0xDDE50BD1D5D0B9EAull}, /*  -36 */
	{0xD4AD2DBFC3D07787ull, 0x955E4EC64B44E865ull}, /*  -35 */
	{0x84EC3C97DA624AB4ull, 0xBD5AF13BEF0B113Full}, /*  -34 */
	{0xA6274BBDD0FADD61ull, 0xECB1AD8AEACDD58Full}, /*  -33 */
	{0xCFB11EAD453994BAull, 0x67DE18EDA5814AF3ull}, /*  -32 */
	{0x81CEB32C4B43FCF4ull, 0x80EACF948770CED8ull}, /*  -31 */
	{0xA2425FF75E14FC31ull, 0xA1258379A94D028Eull}, /*  -30 */
	{0xCAD2F7F5359A3B3Eull, 0x096EE45813A04331ull}, /*  -29 */
	{0xFD87B5F28300CA0Dull, 0x8BCA9D6E188853FDull}, /*  -28 */
	{0x9E74D1B791E07E48ull, 0x775EA264CF55347Eull}, /*  -27 */
	{0xC612062576589DDAull, 0x95364AFE032A819Eull}, /*  -26 */
	{0xF79687AED3EEC551ull, 0x3A83DDBD83F52205ull}, /*  -25 */
	{0x9ABE14CD44753B52ull, 0xC4926A9672793543ull}, /*  -24 */
	{0xC16D9A0095928A27ull, 0x75B7053C0F178294ull}, /*  -23 */
	{0xF1C90080BAF72CB1ull, 0x5324C68B12DD6339ull}, /*  -22 */
	{0x971DA05074DA7BEEull, 0xD3F6FC16EBCA5E04ull}, /*  -21 */
	{0xBCE5086492111AEAull, 0x88F4BB1CA6BCF585ull}, /*  -20 */
	{0xEC1E4A7DB69561A5ull, 0x2B31E9E3D06C32E6ull}, /*  -19 */
	{0x9392EE8E921D5D07ull, 0x3AFF322E62439FD0ull}, /*  -18 */
	{0xB877AA3236A4B449ull, 0x09BEFEB9FAD487C3ull}, /*  -17 */
	{0xE69594BEC44DE15Bull, 0x4C2EBE687989A9B4ull}, /*  -16 */
	{0x901D7CF73AB0ACD9ull, 0x0F9D37014BF60A11ull}, /*  -15 */
	{0xB424DC35095CD80Full, 0x538484C19EF38C95ull}, /*  -14 */
	{0xE12E13424BB40E13ull, 0x2865A5F206B06FBAull}, /*  -13 */
	{0x8CBCCC096F5088CBull, 0xF93F87B7442E45D4ull}, /*  -12 */
	{0xAFEBFF0BCB24AAFEull, 0xF78F69A51539D749ull}, /*  -11 */
	{0xDBE6FECEBDEDD5BEull, 0xB573440E5A884D1Cull}, /*  -10 */
	{0x89705F4136B4A597ull, 0x31680A88F8953031ull}, /*   -9 */
	{0xABCC77118461CEFCull, 0xFDC20D2B36BA7C3Eull}, /*   -8 */
	{0xD6BF94D5E57A42BCull, 0x3D32907604691B4Dull}, /*   -7 */
	{0x8637BD05AF6C69B5ull, 0xA63F9A49C2C1B110ull}, /*   -6 */
	{0xA7C5AC471B478423ull, 0x0FCF80DC33721D54ull}, /*   -5 */
	{0xD1B71758E219652Bull, 0xD3C36113404EA4A9ull}, /*   -4 */
	{0x83126E978D4FDF3Bull, 0x645A1CAC083126EAull}, /*   -3 */
	{0xA3D70A3D70A3D70Aull, 0x3D70A3D70A3D70A4ull}, /*   -2 */
	{0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCDull}, /*   -1 */
	{0x8000000000000000ull, 0x0000000000000000ull}, /*    0 */
	{0xA000000000000000ull, 0x0000000000000000ull}, /*    1 */
	{0xC800000000000000ull, 0x0000000000000000ull}, /*    2 */
	{0xFA00000000000000ull, 0x0000000000000000ull}, /*    3 */
	{0x9C40000000000000ull, 0x0000000000000000ull}, /*    4 */
	{0xC350000000000000ull, 0x0000000000000000ull}, /*    5 */
	{0xF424000000000000ull, 0x0000000000000000ull}, /*    6 */
	{0x9896800000000000ull, 0x0000000000000000ull}, /*    7 */
	{0xBEBC200000000000ull, 0x0000000000000000ull}, /*    8 */
	{0xEE6B280000000000ull, 0x0000000000000000ull}, /*    9 */
	{0x9502F90000000000ull, 0x0000000000000000ull}, /*   10 */
	{0xBA43B74000000000ull, 0x0000000000000000ull}, /*   11 */
	{0xE8D4A51000000000ull, 0x0000000000000000ull}, /*   12 */
	{0x9184E72A00000000ull, 0x0000000000000000ull}, /*   13 */
	{0xB5E620F480000000ull, 0x0000000000000000ull}, /*   14 */
	{0xE35FA931A0000000ull, 0x0000000000000000ull}, /*   15 */
	{0x8E1BC9BF04000000ull, 0x0000000000000000ull}, /*   16 */
	{0xB1A2BC2EC5000000ull, 0x0000000000000000ull}, /*   17 */
	{0xDE0B6B3A76400000ull, 0x0000000000000000ull}, /*   18 */
	{0x8AC7230489E80000ull, 0x0000000000000000ull}, /*   19 */
	{0xAD78EBC5AC620000ull, 0x0000000000000000ull}, /*   20 */
	{0xD8D726B7177A8000ull, 0x0000000000000000ull}, /*   21 */
	{0x878678326EAC9000ull, 0x0000000000000000ull}, /*   22 */
	{0xA968163F0A57B400ull, 0x0000000000000000ull}, /*   23 */
	{0xD3C21BCECCEDA100ull, 0x0000000000000000ull}, /*   24 */
	{0x84595161401484A0ull, 0x0000000000000000ull}, /*   25 */
	{0xA56FA5B99019A5C8ull, 0x0000000000000000ull}, /*   26 */
	{0xCECB8F27F4200F3Aull, 0x0000000000000000ull}, /*   27 */
	{0x813F3978F8940984ull, 0x4000000000000000ull}, /*   28 */
	{0xA18F07D736B90BE5ull, 0x5000000000000000ull}, /*   29 */
	{0xC9F2C9CD04674EDEull, 0xA400000000000000ull}, /*   30 */
	{0xFC6F7C4045812296ull, 0x4D00000000000000ull}, /*   31 */
	{0x9DC5ADA82B70B59Dull, 0xF020000000000000ull}, /*   32 */
	{0xC5371912364CE305ull, 0x6C28000000000000ull}, /*   33 */
	{0xF684DF56C3E01BC6ull, 0xC732000000000000ull}, /*   34 */
	{0x9A130B963A6C115Cull, 0x3C7F400000000000ull}, /*   35 */
	{0xC097CE7BC90715B3ull, 0x4B9F100000000000ull}, /*   36 */
	{0xF0BDC21ABB48DB20ull, 0x1E86D40000000000ull}, /*   37 */
	{0x96769950B50D88F4ull, 0x1314448000000000ull}, /*   38 */
	{0xBC143FA4E250EB31ull, 0x17D955A000000000ull}, /*   39 */
	{0xEB194F8E1AE525FDull, 0x5DCFAB0800000000ull}, /*   40 */
	{0x92EFD1B8D0CF37BEull, 0x5AA1CAE500000000ull}, /*   41 */
	{0xB7ABC627050305ADull, 0xF14A3D9E40000000ull}, /*   42 */
	{0xE596B7B0C643C719ull, 0x6D9CCD05D0000000ull}, /*   43 */
	{0x8F7E32CE7BEA5C6Full, 0xE4820023A2000000ull}, /*   44 */
	{0xB35DBF821AE4F38Bull, 0xDDA2802C8A800000ull}, /*   45 */
	{0xE0352F62A19E306Eull, 0xD50B2037AD200000ull}, /*   46 */
	{0x8C213D9DA502DE45ull, 0x4526F422CC340000ull}, /*   47 */
	{0xAF298D050E4395D6ull, 0x9670B12B7F410000ull}, /*   48 */
	{0xDAF3F04651D47B4Cull, 0x3C0CDD765F114000ull}, /*   49 */
	{0x88D8762BF324CD0Full, 0xA5880A69FB6AC800ull}, /*   50 */
	{0xAB0E93B6EFEE0053ull, 0x8EEA0D047A457A00ull}, /*   51 */
	{0xD5D238A4ABE98068ull, 0x72A4904598D6D880ull}, /*   52 */
	{0x85A36366EB71F041ull, 0x47A6DA2B7F864750ull}, /*   53 */
	{0xA70C3C40A64E6C51ull, 0x999090B65F67D924ull}, /*   54 */
	{0xD0CF4B50CFE20765ull, 0xFFF4B4E3F741CF6Dull}, /*   55 */
	{0x82818F1281ED449Full, 0xBFF8F10E7A8921A5ull}, /*   56 */
	{0xA321F2D7226895C7ull, 0xAFF72D52192B6A0Eull}, /*   57 */
	{0xCBEA6F8CEB02BB39ull, 0x9BF4F8A69F764491ull}, /*   58 */
	{0xFEE50B7025C36A08ull, 0x02F236D04753D5B5ull}, /*   59 */
	{0x9F4F2726179A2245ull, 0x01D762422C946591ull}, /*   60 */
	{0xC722F0EF9D80AAD6ull, 0x424D3AD2B7B97EF6ull}, /*   61 */
	{0xF8EBAD2B84E0D58Bull, 0xD2E0898765A7DEB3ull}, /*   62 */
	{0x9B934C3B330C8577ull, 0x63CC55F49F88EB30ull}, /*   63 */
	{0xC2781F49FFCFA6D5ull, 0x3CBF6B71C76B25FCull}, /*   64 */
	{0xF316271C7FC3908Aull, 0x8BEF464E3945EF7Bull}, /*   65 */
	{0x97EDD871CFDA3A56ull, 0x97758BF0E3CBB5ADull}, /*   66 */
	{0xBDE94E8E43D0C8ECull, 0x3D52EEED1CBEA318ull}, /*   67 */
	{0xED63A231D4C4FB27ull, 0x4CA7AAA863EE4BDEull}, /*   68 */
	{0x945E455F24FB1CF8ull, 0x8FE8CAA93E74EF6Bull}, /*   69 */
	{0xB975D6B6EE39E436ull, 0xB3E2FD538E122B45ull}, /*   70 */
	{0xE7D34C64A9C85D44ull, 0x60DBBCA87196B617ull}, /*   71 */
	{0x90E40FBEEA1D3A4Aull, 0xBC8955E946FE31CEull}, /*   72 */
	{0xB51D13AEA4A488DDull, 0x6BABAB6398BDBE42ull}, /*   73 */
	{0xE264589A4DCDAB14ull, 0xC696963C7EED2DD2ull}, /*   74 */
	{0x8D7EB76070A08AECull, 0xFC1E1DE5CF543CA3ull}, /*   75 */
	{0xB0DE65388CC8ADA8ull, 0x3B25A55F43294BCCull}, /*   76 */
	{0xDD15FE86AFFAD912ull, 0x49EF0EB713F39EBFull}, /*   77 */
	{0x8A2DBF142DFCC7ABull, 0x6E3569326C784338ull}, /*   78 */
	{0xACB92ED9397BF996ull, 0x49C2C37F07965405ull}, /*   79 */
	{0xD7E77A8F87DAF7FBull, 0xDC33745EC97BE907ull}, /*   80 */
	{0x86F0AC99B4E8DAFDull, 0x69A028BB3DED71A4ull}, /*   81 */
	{0xA8ACD7C0222311BCull, 0xC40832EA0D68CE0Dull}, /*   82 */
	{0xD2D80DB02AABD62Bull, 0xF50A3FA490C30191ull}, /*   83 */
	{0x83C7088E1AAB65DBull, 0x792667C6DA79E0FBull}, /*   84 */
	{0xA4B8CAB1A1563F52ull, 0x577001B891185939ull}, /*   85 */
	{0xCDE6FD5E09ABCF26ull, 0xED4C0226B55E6F87ull}, /*   86 */
	{0x80B05E5AC60B6178ull, 0x544F8158315B05B5ull}, /*   87 */
	{0xA0DC75F1778E39D6ull, 0x696361AE3DB1C722ull}, /*   88 */
	{0xC913936DD571C84Cull, 0x03BC3A19CD1E38EAull}, /*   89 */
	{0xFB5878494ACE3A5Full, 0x04AB48A04065C724ull}, /*   90 */
	{0x9D174B2DCEC0E47Bull, 0x62EB0D64283F9C77ull}, /*   91 */
	{0xC45D1DF942711D9Aull, 0x3BA5D0BD324F8395ull}, /*   92 */
	{0xF5746577930D6500ull, 0xCA8F44EC7EE3647Aull}, /*   93 */
	{0x9968BF6ABBE85F20ull, 0x7E998B13CF4E1ECCull}, /*   94 */
	{0xBFC2EF456AE276E8ull, 0x9E3FEDD8C321A67Full}, /*   95 */
	{0xEFB3AB16C59B14A2ull, 0xC5CFE94EF3EA101Full}, /*   96 */
	{0x95D04AEE3B80ECE5ull, 0xBBA1F1D158724A13ull}, /*   97 */
	{0xBB445DA9CA61281Full, 0x2A8A6E45AE8EDC98ull}, /*   98 */
	{0xEA1575143CF97226ull, 0xF52D09D71A3293BEull}, /*   99 */
	{0x924D692CA61BE758ull, 0x593C2626705F9C57ull}, /*  100 */
	{0xB6E0C377CFA2E12Eull, 0x6F8B2FB00C77836Dull}, /*  101 */
	{0xE498F455C38B997Aull, 0x0B6DFB9C0F956448ull}, /*  102 */
	{0x8EDF98B59A373FECull, 0x4724BD4189BD5EADull}, /*  103 */
	{0xB2977EE300C50FE7ull, 0x58EDEC91EC2CB658ull}, /*  104 */
	{0xDF3D5E9BC0F653E1ull, 0x2F2967B66737E3EEull}, /*  105 */
	{0x8B865B215899F46Cull, 0xBD79E0D20082EE75ull}, /*  106 */
	{0xAE67F1E9AEC07187ull, 0xECD8590680A3AA12ull}, /*  107 */
	{0xDA01EE641A708DE9ull, 0xE80E6F4820CC9496ull}, /*  108 */
	{0x884134FE908658B2ull, 0x3109058D147FDCDEull}, /*  109 */
	{0xAA51823E34A7EEDEull, 0xBD4B46F0599FD416ull}, /*  110 */
	{0xD4E5E2CDC1D1EA96ull, 0x6C9E18AC7007C91Bull}, /*  111 */
	{0x850FADC09923329Eull, 0x03E2CF6BC604DDB1ull}, /*  112 */
	{0xA6539930BF6BFF45ull, 0x84DB8346B786151Dull}, /*  113 */
	{0xCFE87F7CEF46FF16ull, 0xE612641865679A64ull}, /*  114 */
	{0x81F14FAE158C5F6Eull, 0x4FCB7E8F3F60C07Full}, /*  115 */
	{0xA26DA3999AEF7749ull, 0xE3BE5E330F38F09Eull}, /*  116 */
	{0xCB090C8001AB551Cull, 0x5CADF5BFD3072CC6ull}, /*  117 */
	{0xFDCB4FA002162A63ull, 0x73D9732FC7C8F7F7ull}, /*  118 */
	{0x9E9F11C4014DDA7Eull, 0x2867E7FDDCDD9AFBull}, /*  119 */
	{0xC646D63501A1511Dull, 0xB281E1FD541501B9ull}, /*  120 */
	{0xF7D88BC24209A565ull, 0x1F225A7CA91A4227ull}, /*  121 */
	{0x9AE757596946075Full, 0x3375788DE9B06959ull}, /*  122 */
	{0xC1A12D2FC3978937ull, 0x0052D6B1641C83AFull}, /*  123 */
	{0xF209787BB47D6B84ull, 0xC0678C5DBD23A49Bull}, /*  124 */
	{0x9745EB4D50CE6332ull, 0xF840B7BA963646E1ull}, /*  125 */
	{0xBD176620A501FBFFull, 0xB650E5A93BC3D899ull}, /*  126 */
	{0xEC5D3FA8CE427AFFull, 0xA3E51F138AB4CEBFull}, /*  127 */
	{0x93BA47C980E98CDFull, 0xC66F336C36B10138ull}, /*  128 */
	{0xB8A8D9BBE123F017ull, 0xB80B0047445D4185ull}, /*  129 */
	{0xE6D3102AD96CEC1Dull, 0xA60DC059157491E6ull}, /*  130 */
	{0x9043EA1AC7E41392ull, 0x87C89837AD68DB30ull}, /*  131 */
	{0xB454E4A179DD1877ull, 0x29BABE4598C311FCull}, /*  132 */
	{0xE16A1DC9D8545E94ull, 0xF4296DD6FEF3D67Bull}, /*  133 */
	{0x8CE2529E2734BB1Dull, 0x1899E4A65F58660Dull}, /*  134 */
	{0xB01AE745B101E9E4ull, 0x5EC05DCFF72E7F90ull}, /*  135 */
	{0xDC21A1171D42645Dull, 0x76707543F4FA1F74ull}, /*  136 */
	{0x899504AE72497EBAull, 0x6A06494A791C53A9ull}, /*  137 */
	{0xABFA45DA0EDBDE69ull, 0x0487DB9D17636893ull}, /*  138 */
	{0xD6F8D7509292D603ull, 0x45A9D2845D3C42B7ull}, /*  139 */
	{0x865B86925B9BC5C2ull, 0x0B8A2392BA45A9B3ull}, /*  140 */
	{0xA7F26836F282B732ull, 0x8E6CAC7768D7141Full}, /*  141 */
	{0xD1EF0244AF2364FFull, 0x3207D795430CD927ull}, /*  142 */
	{0x8335616AED761F1Full, 0x7F44E6BD49E807B9ull}, /*  143 */
	{0xA402B9C5A8D3A6E7ull, 0x5F16206C9C6209A7ull}, /*  144 */
	{0xCD036837130890A1ull, 0x36DBA887C37A8C10ull}, /*  145 */
	{0x802221226BE55A64ull, 0xC2494954DA2C978Aull}, /*  146 */
	{0xA02AA96B06DEB0FDull, 0xF2DB9BAA10B7BD6Dull}, /*  147 */
	{0xC83553C5C8965D3Dull, 0x6F92829494E5ACC8ull}, /*  148 */
	{0xFA42A8B73ABBF48Cull, 0xCB772339BA1F17FAull}, /*  149 */
	{0x9C69A97284B578D7ull, 0xFF2A760414536EFCull}, /*  150 */
	{0xC38413CF25E2D70Dull, 0xFEF5138519684ABBull}, /*  151 */
	{0xF46518C2EF5B8CD1ull, 0x7EB258665FC25D6Aull}, /*  152 */
	{0x98BF2F79D5993802ull, 0xEF2F773FFBD97A62ull}, /*  153 */
	{0xBEEEFB584AFF8603ull, 0xAAFB550FFACFD8FBull}, /*  154 */
	{0xEEAABA2E5DBF6784ull, 0x95BA2A53F983CF39ull}, /*  155 */
	{0x952AB45CFA97A0B2ull, 0xDD945A747BF26184ull}, /*  156 */
	{0xBA756174393D88DFull, 0x94F971119AEEF9E5ull}, /*  157 */
	{0xE912B9D1478CEB17ull, 0x7A37CD5601AAB85Eull}, /*  158 */
	{0x91ABB422CCB812EEull, 0xAC62E055C10AB33Bull}, /*  159 */
	{0xB616A12B7FE617AAull, 0x577B986B314D600Aull}, /*  160 */
	{0xE39C49765FDF9D94ull, 0xED5A7E85FDA0B80Cull}, /*  161 */
	{0x8E41ADE9FBEBC27Dull, 0x14588F13BE847308ull}, /*  162 */
	{0xB1D219647AE6B31Cull, 0x596EB2D8AE258FC9ull}, /*  163 */
	{0xDE469FBD99A05FE3ull, 0x6FCA5F8ED9AEF3BCull}, /*  164 */
	{0x8AEC23D680043BEEull, 0x25DE7BB9480D5855ull}, /*  165 */
	{0xADA72CCC20054AE9ull, 0xAF561AA79A10AE6Bull}, /*  166 */
	{0xD910F7FF28069DA4ull, 0x1B2BA1518094DA05ull}, /*  167 */
	{0x87AA9AFF79042286ull, 0x90FB44D2F05D0843ull}, /*  168 */
	{0xA99541BF57452B28ull, 0x353A1607AC744A54ull}, /*  169 */
	{0xD3FA922F2D1675F2ull, 0x42889B8997915CE9ull}, /*  170 */
	{0x847C9B5D7C2E09B7ull, 0x69956135FEBADA12ull}, /*  171 */
	{0xA59BC234DB398C25ull, 0x43FAB9837E699096ull}, /*  172 */
	{0xCF02B2C21207EF2Eull, 0x94F967E45E03F4BCull}, /*  173 */
	{0x8161AFB94B44F57Dull, 0x1D1BE0EEBAC278F6ull}, /*  174 */
	{0xA1BA1BA79E1632DCull, 0x6462D92A69731733ull}, /*  175 */
	{0xCA28A291859BBF93ull, 0x7D7B8F7503CFDCFFull}, /*  176 */
	{0xFCB2CB35E702AF78ull, 0x5CDA735244C3D43Full}, /*  177 */
	{0x9DEFBF01B061ADABull, 0x3A0888136AFA64A8ull}, /*  178 */
	{0xC56BAEC21C7A1916ull, 0x088AAA1845B8FDD1ull}, /*  179 */
	{0xF6C69A72A3989F5Bull, 0x8AAD549E57273D46ull}, /*  180 */
	{0x9A3C2087A63F6399ull, 0x36AC54E2F678864Cull}, /*  181 */
	{0xC0CB28A98FCF3C7Full, 0x84576A1BB416A7DEull}, /*  182 */
	{0xF0FDF2D3F3C30B9Full, 0x656D44A2A11C51D6ull}, /*  183 */
	{0x969EB7C47859E743ull, 0x9F644AE5A4B1B326ull}, /*  184 */
	{0xBC4665B596706114ull, 0x873D5D9F0DDE1FEFull}, /*  185 */
	{0xEB57FF22FC0C7959ull, 0xA90CB506D155A7EBull}, /*  186 */
	{0x9316FF75DD87CBD8ull, 0x09A7F12442D588F3ull}, /*  187 */
	{0xB7DCBF5354E9BECEull, 0x0C11ED6D538AEB30ull}, /*  188 */
	{0xE5D3EF282A242E81ull, 0x8F1668C8A86DA5FBull}, /*  189 */
	{0x8FA475791A569D10ull, 0xF96E017D694487BDull}, /*  190 */
	{0xB38D92D760EC4455ull, 0x37C981DCC395A9ADull}, /*  191 */
	{0xE070F78D3927556Aull, 0x85BBE253F47B1418ull}, /*  192 */
	{0x8C469AB843B89562ull, 0x93956D7478CCEC8Full}, /*  193 */
	{0xAF58416654A6BABBull, 0x387AC8D1970027B3ull}, /*  194 */
	{0xDB2E51BFE9D0696Aull, 0x06997B05FCC0319Full}, /*  195 */
	{0x88FCF317F22241E2ull, 0x441FECE3BDF81F04ull}, /*  196 */
	{0xAB3C2FDDEEAAD25Aull, 0xD527E81CAD7626C4ull}, /*  197 */
	{0xD60B3BD56A5586F1ull, 0x8A71E223D8D3B075ull}, /*  198 */
	{0x85C7056562757456ull, 0xF6872D5667844E4Aull}, /*  199 */
	{0xA738C6BEBB12D16Cull, 0xB428F8AC016561DCull}, /*  200 */
	{0xD106F86E69D785C7ull, 0xE13336D701BEBA53ull}, /*  201 */
	{0x82A45B450226B39Cull, 0xECC0024661173474ull}, /*  202 */
	{0xA34D721642B06084ull, 0x27F002D7F95D0191ull}, /*  203 */
	{0xCC20CE9BD35C78A5ull, 0x31EC038DF7B441F5ull}, /*  204 */
	{0xFF290242C83396CEull, 0x7E67047175A15272ull}, /*  205 */
	{0x9F79A169BD203E41ull, 0x0F0062C6E984D387ull}, /*  206 */
	{0xC75809C42C684DD1ull, 0x52C07B78A3E60869ull}, /*  207 */
	{0xF92E0C3537826145ull, 0xA7709A56CCDF8A83ull}, /*  208 */
	{0x9BBCC7A142B17CCBull, 0x88A66076400BB692ull}, /*  209 */
	{0xC2ABF989935DDBFEull, 0x6ACFF893D00EA436ull}, /*  210 */
	{0xF356F7EBF83552FEull, 0x0583F6B8C4124D44ull}, /*  211 */
	{0x98165AF37B2153DEull, 0xC3727A337A8B704Bull}, /*  212 */
	{0xBE1BF1B059E9A8D6ull, 0x744F18C0592E4C5Dull}, /*  213 */
	{0xEDA2EE1C7064130Cull, 0x1162DEF06F79DF74ull}, /*  214 */
	{0x9485D4D1C63E8BE7ull, 0x8ADDCB5645AC2BA9ull}, /*  215 */
	{0xB9A74A0637CE2EE1ull, 0x6D953E2BD7173693ull}, /*  216 */
	{0xE8111C87C5C1BA99ull, 0xC8FA8DB6CCDD0438ull}, /*  217 */
	{0x910AB1D4DB9914A0ull, 0x1D9C9892400A22A3ull}, /*  218 */
	{0xB54D5E4A127F59C8ull, 0x2503BEB6D00CAB4Cull}, /*  219 */
	{0xE2A0B5DC971F303Aull, 0x2E44AE64840FD61Eull}, /*  220 */
	{0x8DA471A9DE737E24ull, 0x5CEAECFED289E5D3ull}, /*  221 */
	{0xB10D8E1456105DADull, 0x7425A83E872C5F48ull}, /*  222 */
	{0xDD50F1996B947518ull, 0xD12F124E28F7771Aull}, /*  223 */
	{0x8A5296FFE33CC92Full, 0x82BD6B70D99AAA70ull}, /*  224 */
	{0xACE73CBFDC0BFB7Bull, 0x636CC64D1001550Cull}, /*  225 */
	{0xD8210BEFD30EFA5Aull, 0x3C47F7E05401AA4Full}, /*  226 */
	{0x8714A775E3E95C78ull, 0x65ACFAEC34810A72ull}, /*  227 */
	{0xA8D9D1535CE3B396ull, 0x7F1839A741A14D0Eull}, /*  228 */
	{0xD31045A8341CA07Cull, 0x1EDE48111209A051ull}, /*  229 */
	{0x83EA2B892091E44Dull, 0x934AED0AAB460433ull}, /*  230 */
	{0xA4E4B66B68B65D60ull, 0xF81DA84D56178540ull}, /*  231 */
	{0xCE1DE40642E3F4B9ull, 0x36251260AB9D668Full}, /*  232 */
	{0x80D2AE83E9CE78F3ull, 0xC1D72B7C6B42601Aull}, /*  233 */
	{0xA1075A24E4421730ull, 0xB24CF65B8612F820ull}, /*  234 */
	{0xC94930AE1D529CFCull, 0xDEE033F26797B628ull}, /*  235 */
	{0xFB9B7CD9A4A7443Cull, 0x169840EF017DA3B2ull}, /*  236 */
	{0x9D412E0806E88AA5ull, 0x8E1F289560EE864Full}, /*  237 */
	{0xC491798A08A2AD4Eull, 0xF1A6F2BAB92A27E3ull}, /*  238 */
	{0xF5B5D7EC8ACB58A2ull, 0xAE10AF696774B1DCull}, /*  239 */
	{0x9991A6F3D6BF1765ull, 0xACCA6DA1E0A8EF2Aull}, /*  240 */
	{0xBFF610B0CC6EDD3Full, 0x17FD090A58D32AF4ull}, /*  241 */
	{0xEFF394DCFF8A948Eull, 0xDDFC4B4CEF07F5B1ull}, /*  242 */
	{0x95F83D0A1FB69CD9ull, 0x4ABDAF101564F98Full}, /*  243 */
	{0xBB764C4CA7A4440Full, 0x9D6D1AD41ABE37F2ull}, /*  244 */
	{0xEA53DF5FD18D5513ull, 0x84C86189216DC5EEull}, /*  245 */
	{0x92746B9BE2F8552Cull, 0x32FD3CF5B4E49BB5ull}, /*  246 */
	{0xB7118682DBB66A77ull, 0x3FBC8C33221DC2A2ull}, /*  247 */
	{0xE4D5E82392A40515ull, 0x0FABAF3FEAA5334Bull}, /*  248 */
	{0x8F05B1163BA6832Dull, 0x29CB4D87F2A7400Full}, /*  249 */
	{0xB2C71D5BCA9023F8ull, 0x743E20E9EF511013ull}, /*  250 */
	{0xDF78E4B2BD342CF6ull, 0x914DA9246B255417ull}, /*  251 */
	{0x8BAB8EEFB6409C1Aull, 0x1AD089B6C2F7548Full}, /*  252 */
	{0xAE9672ABA3D0C320ull, 0xA184AC2473B529B2ull}, /*  253 */
	{0xDA3C0F568CC4F3E8ull, 0xC9E5D72D90A2741Full}, /*  254 */
	{0x8865899617FB1871ull, 0x7E2FA67C7A658893ull}, /*  255 */
	{0xAA7EEBFB9DF9DE8Dull, 0xDDBB901B98FEEAB8ull}, /*  256 */
	{0xD51EA6FA85785631ull, 0x552A74227F3EA566ull}, /*  257 */
	{0x8533285C936B35DEull, 0xD53A88958F872760ull}, /*  258 */
	{0xA67FF273B8460356ull, 0x8A892ABAF368F138ull}, /*  259 */
	{0xD01FEF10A657842Cull, 0x2D2B7569B0432D86ull}, /*  260 */
	{0x8213F56A67F6B29Bull, 0x9C3B29620E29FC74ull}, /*  261 */
	{0xA298F2C501F45F42ull, 0x8349F3BA91B47B90ull}, /*  262 */
	{0xCB3F2F7642717713ull, 0x241C70A936219A74ull}, /*  263 */
	{0xFE0EFB53D30DD4D7ull, 0xED238CD383AA0111ull}, /*  264 */
	{0x9EC95D1463E8A506ull, 0xF4363804324A40ABull}, /*  265 */
	{0xC67BB4597CE2CE48ull, 0xB143C6053EDCD0D6ull}, /*  266 */
	{0xF81AA16FDC1B81DAull, 0xDD94B7868E94050Bull}, /*  267 */
	{0x9B10A4E5E9913128ull, 0xCA7CF2B4191C8327ull}, /*  268 */
	{0xC1D4CE1F63F57D72ull, 0xFD1C2F611F63A3F1ull}, /*  269 */
	{0xF24A01A73CF2DCCFull, 0xBC633B39673C8CEDull}, /*  270 */
	{0x976E41088617CA01ull, 0xD5BE0503E085D814ull}, /*  271 */
	{0xBD49D14AA79DBC82ull, 0x4B2D8644D8A74E19ull}, /*  272 */
	{0xEC9C459D51852BA2ull, 0xDDF8E7D60ED1219Full}, /*  273 */
	{0x93E1AB8252F33B45ull, 0xCABB90E5C942B504ull}, /*  274 */
	{0xB8DA1662E7B00A17ull, 0x3D6A751F3B936244ull}, /*  275 */
	{0xE7109BFBA19C0C9Dull, 0x0CC512670A783AD5ull}, /*  276 */
	{0x906A617D450187E2ull, 0x27FB2B80668B24C6ull}, /*  277 */
	{0xB484F9DC9641E9DAull, 0xB1F9F660802DEDF7ull}, /*  278 */
	{0xE1A63853BBD26451ull, 0x5E7873F8A0396974ull}, /*  279 */
	{0x8D07E33455637EB2ull, 0xDB0B487B6423E1E9ull}, /*  280 */
	{0xB049DC016ABC5E5Full, 0x91CE1A9A3D2CDA63ull}, /*  281 */
	{0xDC5C5301C56B75F7ull, 0x7641A140CC7810FCull}, /*  282 */
	{0x89B9B3E11B6329BAull, 0xA9E904C87FCB0A9Eull}, /*  283 */
	{0xAC2820D9623BF429ull, 0x546345FA9FBDCD45ull}, /*  284 */
	{0xD732290FBACAF133ull, 0xA97C177947AD4096ull}, /*  285 */
	{0x867F59A9D4BED6C0ull, 0x49ED8EABCCCC485Eull}, /*  286 */
	{0xA81F301449EE8C70ull, 0x5C68F256BFFF5A75ull}, /*  287 */
	{0xD226FC195C6A2F8Cull, 0x73832EEC6FFF3112ull}, /*  288 */
	{0x83585D8FD9C25DB7ull, 0xC831FD53C5FF7EACull}, /*  289 */
	{0xA42E74F3D032F525ull, 0xBA3E7CA8B77F5E56ull}, /*  290 */
	{0xCD3A1230C43FB26Full, 0x28CE1BD2E55F35ECull}, /*  291 */
	{0x80444B5E7AA7CF85ull, 0x7980D163CF5B81B4ull}, /*  292 */
	{0xA0555E361951C366ull, 0xD7E105BCC3326220ull}, /*  293 */
	{0xC86AB5C39FA63440ull, 0x8DD9472BF3FEFAA8ull}, /*  294 */
	{0xFA856334878FC150ull, 0xB14F98F6F0FEB952ull}, /*  295 */
	{0x9C935E00D4B9D8D2ull, 0x6ED1BF9A569F33D4ull}, /*  296 */
	{0xC3B8358109E84F07ull, 0x0A862F80EC4700C9ull}, /*  297 */
	{0xF4A642E14C6262C8ull, 0xCD27BB612758C0FBull}, /*  298 */
	{0x98E7E9CCCFBD7DBDull, 0x8038D51CB897789Dull}, /*  299 */
	{0xBF21E44003ACDD2Cull, 0xE0470A63E6BD56C4ull}, /*  300 */
	{0xEEEA5D5004981478ull, 0x1858CCFCE06CAC75ull}, /*  301 */
	{0x95527A5202DF0CCBull, 0x0F37801E0C43EBC9ull}, /*  302 */
	{0xBAA718E68396CFFDull, 0xD30560258F54E6BBull}, /*  303 */
	{0xE950DF20247C83FDull, 0x47C6B82EF32A206Aull}, /*  304 */
	{0x91D28B7416CDD27Eull, 0x4CDC331D57FA5442ull}, /*  305 */
	{0xB6472E511C81471Dull, 0xE0133FE4ADF8E953ull}, /*  306 */
	{0xE3D8F9E563A198E5ull, 0x58180FDDD97723A7ull}, /*  307 */
	{0x8E679C2F5E44FF8Full, 0x570F09EAA7EA7649ull}, /*  308 */
	{0xB201833B35D63F73ull, 0x2CD2CC6551E513DBull}, /*  309 */
	{0xDE81E40A034BCF4Full, 0xF8077F7EA65E58D2ull}, /*  310 */
	{0x8B112E86420F6191ull, 0xFB04AFAF27FAF783ull}, /*  311 */
	{0xADD57A27D29339F6ull, 0x79C5DB9AF1F9B564ull}, /*  312 */
	{0xD94AD8B1C7380874ull, 0x18375281AE7822BDull}, /*  313 */
	{0x87CEC76F1C830548ull, 0x8F2293910D0B15B6ull}, /*  314 */
	{0xA9C2794AE3A3C69Aull, 0xB2EB3875504DDB23ull}, /*  315 */
	{0xD433179D9C8CB841ull, 0x5FA60692A46151ECull}, /*  316 */
	{0x849FEEC281D7F328ull, 0xDBC7C41BA6BCD334ull}, /*  317 */
	{0xA5C7EA73224DEFF3ull, 0x12B9B522906C0801ull}, /*  318 */
	{0xCF39E50FEAE16BEFull, 0xD768226B34870A01ull}, /*  319 */
	{0x81842F29F2CCE375ull, 0xE6A1158300D46641ull}, /*  320 */
	{0xA1E53AF46F801C53ull, 0x60495AE3C1097FD1ull}, /*  321 */
	{0xCA5E89B18B602368ull, 0x385BB19CB14BDFC5ull}, /*  322 */
	{0xFCF62C1DEE382C42ull, 0x46729E03DD9ED7B6ull}, /*  323 */
	{0x9E19DB92B4E31BA9ull, 0x6C07A2C26A8346D2ull}  /*  324 */
};


#define AUTOINCLUDE_1
	#include "flt2str.c"
#undef  AUTOINCLUDE_1

#endif
