/*
 * Copyright (C) 2025, jpn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.64
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

#ifndef f97bfda1_f6fe_4f84_bf7a_18aebe994cc5
#define f97bfda1_f6fe_4f84_bf7a_18aebe994cc5

/*
 * ckdint.h
 * Safe integer operations.
 */

#include "ctoolbox.h"


#if defined(CTB_CFG_HAS_CKDINT_INTRINSICS)

#define ckdi32_add(a, b, r) __builtin_add_overflow(( int32) a, ( int32) b, r)
#define ckdi64_add(a, b, r) __builtin_add_overflow(( int64) a, ( int64) b, r)
#define ckdi32_sub(a, b, r) __builtin_sub_overflow(( int32) a, ( int32) b, r)
#define ckdi64_sub(a, b, r) __builtin_sub_overflow(( int64) a, ( int64) b, r)
#define ckdi32_mul(a, b, r) __builtin_mul_overflow(( int32) a, ( int32) b, r)
#define ckdi64_mul(a, b, r) __builtin_mul_overflow(( int64) a, ( int64) b, r)

#define ckdu32_add(a, b, r) __builtin_add_overflow((uint32) a, (uint32) b, r)
#define ckdu64_add(a, b, r) __builtin_add_overflow((uint64) a, (uint64) b, r)
#define ckdu32_mul(a, b, r) __builtin_mul_overflow((uint32) a, (uint32) b, r)
#define ckdu64_mul(a, b, r) __builtin_mul_overflow((uint64) a, (uint64) b, r)

#else
	#if defined(CTB_CFG_HAS_STDCKDINT)
		#include <stdckdint.h>

		#define ckdi32_add(a, b, r) ckd_add(r, ( int32) (a), ( int32) (b))
		#define ckdi64_add(a, b, r) ckd_add(r, ( int64) (a), ( int64) (b))
		#define ckdi32_sub(a, b, r) ckd_sub(r, ( int32) (a), ( int32) (b))
		#define ckdi64_sub(a, b, r) ckd_sub(r, ( int64) (a), ( int64) (b))
		#define ckdi32_mul(a, b, r) ckd_mul(r, ( int32) (a), ( int32) (b))
		#define ckdi64_mul(a, b, r) ckd_mul(r, ( int64) (a), ( int64) (b))

		#define ckdu32_add(a, b, r) ckd_add(r, (uint32) (a), (uint32) (b))
		#define ckdu64_add(a, b, r) ckd_add(r, (uint64) (a), (uint64) (b))
		#define ckdu32_mul(a, b, r) ckd_mul(r, (uint32) (a), (uint32) (b))
		#define ckdu64_mul(a, b, r) ckd_mul(r, (uint64) (a), (uint64) (b))
	#else
		#define CTB_CKDINT_FALLBACK
	#endif
#endif


#if defined(CTB_CKDINT_FALLBACK)


/*
 * Unsigned integer operations */
CTB_INLINE bool ckdu32_add(uint32 lhs, uint32 rhs, uint32* result);
CTB_INLINE bool ckdu64_add(uint64 lhs, uint64 rhs, uint64* result);
CTB_INLINE bool ckdu32_mul(uint32 lhs, uint32 rhs, uint32* result);
CTB_INLINE bool ckdu64_mul(uint64 lhs, uint64 rhs, uint64* result);

/*
 * Signed integer operations */
CTB_INLINE bool ckdi32_add(int32 lhs, int32 rhs, int32* result);
CTB_INLINE bool ckdi64_add(int64 lhs, int64 rhs, int64* result);
CTB_INLINE bool ckdi32_sub(int32 lhs, int32 rhs, int32* result);
CTB_INLINE bool ckdi64_sub(int64 lhs, int64 rhs, int64* result);
CTB_INLINE bool ckdi32_mul(int32 lhs, int32 rhs, int32* result);
CTB_INLINE bool ckdi64_mul(int64 lhs, int64 rhs, int64* result);


/*
 * Inlines */

#define SIGNSHIFT32 ((sizeof(uint32) << 3) - 1)
#define SIGNSHIFT64 ((sizeof(uint64) << 3) - 1)


CTB_INLINE bool
ckdu32_add(uint32 lhs, uint32 rhs, uint32* result)
{
#if defined(CTB_ENV64)
	uint64 c;

	result[0] = (uint32) (c = ((uint64) lhs + (uint64) rhs));
	c >>= 32;
#else
	uint32 c;

	c = (((lhs & rhs) & 1) + (lhs >> 1) + (rhs >> 1)) >> SIGNSHIFT32;
	result[0] = lhs + rhs;
#endif
	if (c)
		return 1;
	return 0;
}

CTB_INLINE bool
ckdu64_add(uint64 lhs, uint64 rhs, uint64* result)
{
	uint64 c;

	c = (((lhs & rhs) & 1) + (lhs >> 1) + (rhs >> 1)) >> SIGNSHIFT64;
	result[0] = lhs + rhs;
	if (c)
		return 1;
	return 0;
}

CTB_INLINE bool
ckdu32_mul(uint32 lhs, uint32 rhs, uint32* result)
{
	uint64 r;

	result[0] = (uint32) (r = ((uint64) lhs * (uint64) rhs));
	if (r > UINT32_MAX)
		return 1;
	return 0;
}

CTB_INLINE bool
ckdu64_mul(uint64 lhs, uint64 rhs, uint64* result)
{
	uint64 lo;
	uint64 hi;

#if defined(__GNUC__) && defined(__SIZEOF_INT128__)
	__uint128_t product;

	product = ((__uint128_t) lhs) * ((__uint128_t) rhs);
	lo = (uint64) (product >> 0x00);
	hi = (uint64) (product >> 0x40);

	result[0] = lo;
#else
	/*
	 * Taken from xxHash3
	 * See also: https://stackoverflow.com/a/58381061 */
	uint64 loxlo;
	uint64 loxhi;
	uint64 hixlo;
	uint64 hixhi;
	uint64 cross;
	uint32 lhh;
	uint32 rhh;

	lhh = (lhs >> 32);
	rhh = (rhs >> 32);
	if ((lhh | rhh) == 0) {
		result[0] = lhs * rhs;
		return 0;
	}

	/* first calculate all of the cross products. */
	loxlo = (lhs & 0xffffffff) * (rhs & 0xffffffff);
	hixlo = (lhh)              * (rhs & 0xffffffff);
	loxhi = (lhs & 0xffffffff) * rhh;
	hixhi = (lhh)              * rhh;

	/* now add the products together. These will never overflow. */
	cross = (loxlo >> 0x20) + (hixlo & 0xffffffff) + loxhi;
	hi = (hixlo >> 0x20) + (cross >> 0x20) + hixhi;
	lo = (cross << 0x20) | (loxlo & 0xffffffff);

	result[0] = lo;
#endif
	if (hi)
		return 1;
	return 0;
}

CTB_INLINE bool
ckdi32_add(int32 lhs, int32 rhs, int32* result)
{
#if defined(CTB_ENV64)
	int64 r;

	r = (int64) lhs + (int64) rhs;
	result[0] = (int32) r;
	if (r < (int64) INT32_MIN || r > (int64) INT32_MAX)
		return 1;
	return 0;
#else
	uint32 r;

	r = (uint32) lhs + (uint32) rhs;
	result[0] = (int32) r;
	if ((~(lhs ^ rhs) & (lhs ^ r)) & (((uint32) 1) << SIGNSHIFT32))
		return 1;
	return 0;
#endif
}

CTB_INLINE bool
ckdi64_add(int64 lhs, int64 rhs, int64* result)
{
	uint64 r;

	r = (uint64) lhs + (uint64) rhs;
	result[0] = (int64) r;
	if ((~(lhs ^ rhs) & (lhs ^ r)) & (((uint64) 1) << SIGNSHIFT64))
		return 1;
	return 0;
}

CTB_INLINE bool
ckdi32_sub(int32 lhs, int32 rhs, int32* result)
{
#if defined(CTB_ENV64)
	int64 r;

	r = (int64) lhs - (int64) rhs;
	result[0] = (int32) r;
	if (r < (int64) INT32_MIN || r > (int64) INT32_MAX)
		return 1;
	return 0;
#else
	uint32 r;

	r = (uint32) lhs - (uint32) rhs;
	result[0] = (int32) r;
	if (((lhs ^ rhs) & (lhs ^ r)) & (((uint32) 1) << SIGNSHIFT32))
		return 1;
	return 0;
#endif
}

CTB_INLINE bool
ckdi64_sub(int64 lhs, int64 rhs, int64* result)
{
	uint64 r;

	r = (uint64) lhs - (uint64) rhs;
	result[0] = (int64) r;
	if (((lhs ^ rhs) & (lhs ^ r)) & (((uint64) 1) << SIGNSHIFT64))
		return 1;
	return 0;
}

CTB_INLINE bool
ckdi32_mul(int32 lhs, int32 rhs, int32* result)
{
	int64 r;

	result[0] = (int32) (r = ((int64) lhs * (int64) rhs));
	if (r < (int64) INT32_MIN || r > (int64) INT32_MAX)
		return 1;
	return 0;
}

CTB_INLINE bool
ckdi64_mul(int64 lhs, int64 rhs, int64* result)
{
	uint64 u;
	uint64 r[1];

	u = ckdu64_mul((uint64) lhs, (uint64) rhs, r);
	result[0] = (int64) r[0];
	if (u || r[0] > (INT64_MIN - ((uint64) (lhs ^ rhs) >> SIGNSHIFT64))) {
		return 1;
	}
	return 0;
}

#undef SIGNSHIFT32
#undef SIGNSHIFT64

#undef CTB_CKDINT_FALLBACK
#endif

#endif
