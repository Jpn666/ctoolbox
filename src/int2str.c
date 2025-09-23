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

#include <ctoolbox/int2str.h>


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


/* */
struct TVal128 {
	uint64 hi;
	uint64 lo;
};

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

/* */
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


uintxx
u32tostr(uint32 number, uint8 r[16])
{
	union TZeroUnion z;
	uint8* s1;
	uint8* s2;
#if !defined(CTB_FASTUNALIGNED)
	uint8* m;
#endif
	uint32 total;
	CTB_ASSERT(r);

#if defined(CTB_ENV64)
	z.asuint[0] = 0x3030303030303030ull;
	z.asuint[1] = 0x3030303030303030ull;
#else
	z.asuint[0] = 0x30303030;
	z.asuint[1] = 0x30303030;
	z.asuint[2] = 0x30303030;
	z.asuint[3] = 0x30303030;
#endif

	s1 = z.buffer + 10;
	if (number < 1000000000) {
		s2 = todigits(number, s1);
	}
	else {
		uint64 a;
		uint64 b;

		a = mul128hi(19342813113834068ull, number) >> 20;
		b = number - (a * 1000000000);

		     todigits((uint32) b, s1 - 0);
		s2 = todigits((uint32) a, s1 - 9);
	}
	total = (uint32) (s1 - s2);

#if defined(CTB_FASTUNALIGNED)
	((uint32*) r)[0] = ((uint32*) s2)[0];
	((uint32*) r)[1] = ((uint32*) s2)[1];
	((uint32*) r)[2] = ((uint32*) s2)[2];

	r[total] = 0x00;
	return total;
#else
	for (m = r; total >= 4; total -= 4) {
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
	}
	switch (total) {
		case  3: *r++ = *s2++;  /* fallthrough */
		case  2: *r++ = *s2++;  /* fallthrough */
		case  1: *r++ = *s2++;  /* fallthrough */
		case  0:
			break;
	}
	r[0] = 0x00;
	return (uintxx) (r - m);
#endif
}

uintxx
u64tostr(uint64 number, uint8 r[24])
{
	union TZeroUnion z;
	uint8* s1;
	uint8* s2;
#if !defined(CTB_FASTUNALIGNED)
	uint8* m;
#endif
	uint32 total;
	CTB_ASSERT(r);

#if defined(CTB_ENV64)
	z.asuint[0] = 0x3030303030303030ull;
	z.asuint[1] = 0x3030303030303030ull;
	z.asuint[2] = 0x3030303030303030ull;
	z.asuint[3] = 0x3030303030303030ull;
#else
	z.asuint[0] = 0x30303030;
	z.asuint[1] = 0x30303030;
	z.asuint[2] = 0x30303030;
	z.asuint[3] = 0x30303030;
	z.asuint[4] = 0x30303030;
	z.asuint[5] = 0x30303030;
	z.asuint[6] = 0x30303030;
	z.asuint[7] = 0x30303030;
#endif

	s1 = z.buffer + 24;
	if (number < 1000000000) {
		s2 = todigits((uint32) number, s1);
	}
	else {
		uint64 a;
		uint64 b;
		uint64 c;

		a = mul128hi(19342813113834067ull, number >> 9) >> 11;
		b = number - (a * 1000000000);
		if (a > 1000000000) {
			number = a;
			c = b;

			a = mul128hi(19342813113834067ull, number >> 9) >> 11;
			b = number - (a * 1000000000);

			     todigits((uint32) c, s1 - 9 * 0);
			     todigits((uint32) b, s1 - 9 * 1);
			s2 = todigits((uint32) a, s1 - 9 * 2);
		}
		else {
			     todigits((uint32) b, s1 - 0);
			s2 = todigits((uint32) a, s1 - 9);
		}
	}
	total = (uint32) (s1 - s2);

#if defined(CTB_FASTUNALIGNED)
	((uint32*) r)[0] = ((uint32*) s2)[0];
	((uint32*) r)[1] = ((uint32*) s2)[1];
	((uint32*) r)[2] = ((uint32*) s2)[2];
	((uint32*) r)[3] = ((uint32*) s2)[3];
	((uint32*) r)[4] = ((uint32*) s2)[4];

	r[total] = 0x00;
	return total;
#else
	m = r;
	for (; total >= 8; total -= 8) {
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
		*r++ = *s2++;
	}
	switch (total) {
		case  7: *r++ = *s2++;  /* fallthrough */
		case  6: *r++ = *s2++;  /* fallthrough */
		case  5: *r++ = *s2++;  /* fallthrough */
		case  4: *r++ = *s2++;  /* fallthrough */
		case  3: *r++ = *s2++;  /* fallthrough */
		case  2: *r++ = *s2++;  /* fallthrough */
		case  1: *r++ = *s2++;  /* fallthrough */
		case  0:
			break;
	}
	r[0] = 0x00;
	return (uintxx) (r - m);
#endif
}


#if defined(__GNUC__)
	#pragma GCC diagnostic push
	#if !defined(__clang__)
		#pragma GCC diagnostic ignored "-Wstringop-overflow"
	#endif
#endif

uintxx
i32tostr(int32 number, uint8 r[16])
{
	uint8* s;
	CTB_ASSERT(r);

	s = r;
	if (number < 0) {
		number = -number;
		*s++ = '-';
	}
	s += u32tostr((uint32) number, s);

	return (uintxx) (s - r);
}

uintxx
i64tostr(int64 number, uint8 r[24])
{
	uint8* s;
	CTB_ASSERT(r);

	s = r;
	if (number < 0) {
		number = -number;
		*s++ = '-';
	}
	s += u64tostr((uint64) number, s);

	return (uintxx) (s - r);
}

#if defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif


uintxx
u32tohexa(uint32 number, intxx uppercase, uint8 r[16])
{
	uint8* s;
	uint32 b1;
	uint32 b2;
	uint32 b3;
	uint32 b4;
	uint32 n;
	const uint8* mode;
	const uint8 d1[] = "0123456789abcdef";
	const uint8 d2[] = "0123456789ABCDEF";
	CTB_ASSERT(r);

	if (number == 0) {
		r[0] = 0x30;
		r[1] = 0x00;
		return 1;
	}

	mode = d1;
	if (uppercase) {
		mode = d2;
	}
	s = r;

	b1 = (number >> 0x18) & 0xff;
	b2 = (number >> 0x10) & 0xff;
	b3 = (number >> 0x08) & 0xff;
	b4 = (number >> 0x00) & 0xff;
	if (b1) {
		n = (b1 >> 4);
		if (n) {
			*s++ = mode[n];
		}
		*s++ = mode[(b1 >> 0) & 0x0f];

		*s++ = mode[(b2 >> 4) & 0x0f];
		*s++ = mode[(b2 >> 0) & 0x0f];
		*s++ = mode[(b3 >> 4) & 0x0f];
		*s++ = mode[(b3 >> 0) & 0x0f];
		*s++ = mode[(b4 >> 4) & 0x0f];
		*s++ = mode[(b4 >> 0) & 0x0f];
	}
	else {
		if (b2) {
			n = (b2 >> 4);
			if (n) {
				*s++ = mode[n];
			}
			*s++ = mode[(b2 >> 0) & 0x0f];

			*s++ = mode[(b3 >> 4) & 0x0f];
			*s++ = mode[(b3 >> 0) & 0x0f];
			*s++ = mode[(b4 >> 4) & 0x0f];
			*s++ = mode[(b4 >> 0) & 0x0f];
		}
		else {
			if (b3) {
				n = (b3 >> 4);
				if (n) {
					*s++ = mode[n];
				}
				*s++ = mode[(b3 >> 0) & 0x0f];

				*s++ = mode[(b4 >> 4) & 0x0f];
				*s++ = mode[(b4 >> 0) & 0x0f];
			}
			else {
				if (b4) {
					n = (b4 >> 4);
					if (n) {
						*s++ = mode[n];
					}
					*s++ = mode[(b4 >> 0) & 0x0f];
				}
			}
		}
	}

	s[0] = 0;
	return (uintxx) (s - r);
}

uintxx
u64tohexa(uint64 number, intxx uppercase, uint8 r[24])
{
	uint32 a;
	uint32 b;
	uint8* s;
	const uint8* mode;
	const uint8 d1[] = "0123456789abcdef";
	const uint8 d2[] = "0123456789ABCDEF";
	CTB_ASSERT(r);

	if (number == 0) {
		r[0] = 0x30;
		r[1] = 0x00;
		return 1;
	}

	s = r;
	a = (uint32) (number >> 0x00);
	b = (uint32) (number >> 0x20);
	if (b) {
		uint32 b1;
		uint32 b2;
		uint32 b3;
		uint32 b4;

		mode = d1;
		if (uppercase) {
			mode = d2;
		}

		s += u32tohexa(b, uppercase, s);

		b1 = (a >> 0x18) & 0xff;
		b2 = (a >> 0x10) & 0xff;
		b3 = (a >> 0x08) & 0xff;
		b4 = (a >> 0x00) & 0xff;

		*s++ = mode[(b1 >> 4) & 0x0f];
		*s++ = mode[(b1 >> 0) & 0x0f];
		*s++ = mode[(b2 >> 4) & 0x0f];
		*s++ = mode[(b2 >> 0) & 0x0f];
		*s++ = mode[(b3 >> 4) & 0x0f];
		*s++ = mode[(b3 >> 0) & 0x0f];
		*s++ = mode[(b4 >> 4) & 0x0f];
		*s++ = mode[(b4 >> 0) & 0x0f];
		s[0] = 0;
	}
	else {
		s += u32tohexa(a, uppercase, s);
	}
	return (uintxx) (s - r);
}
