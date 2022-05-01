/*
 * Copyright (C) 2022, jpn
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

#include "../../ctype.h"
#include "../str2int.h"



CTB_INLINE bool
checkbase(uintxx chr, uintxx base)
{
	static const uint8 chrbasemap[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
		0x1e, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00,
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
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	return (chrbasemap[chr] & base) != 0;
}


CTB_INLINE uintxx
getbase(const char** src, uintxx base)
{
	const char *s;

	s = src[0];
	if (s[0] == 0x30) {
		if (base == 0 || base == 0x10) {
			if ((s[1] | 0x20) == 0x78) {
				if (checkbase(s[2], 0x10)) {
					s += 2;
					base = 0x10;
				}
			}
		}
		if (base == 0 || base == 0x02) {
			if ((s[1] | 0x20) == 0x62) {
				if (checkbase(s[2], 0x02)) {
					s += 2;
					base = 0x02;
				}
			}
		}
		if (base == 0 || base == 0x08) {
			if ((s[1] | 0x20) == 0x6f) {
				if (checkbase(s[2], 0x08)) {
					s += 2;
					base = 0x08;
				}
			}
		}
		if (base == 0)
			base = 0x0a;
	}
	else {
		if (base == 0) {
			base = 0x0a;
		}
		else {
			if (base < 2 || base > 16) {
				base = 0;
			}
		}
	}
	src[0] = s;

	return base;
}


static bool
parseu32(const char* src, const char** end, uint32* result, uintxx b)
{
	const char* s;
	uint32 r;
	uint32 n;
	uint32 c;

	s = src;
	for (r = 0; (c = s[0]); s++) {
		uint64 j;
		if (ctb_isdigit(c)) {
			n = c - 0x30;
		}
		else {
			c = c | 0x20;
			if (ctb_isalpha(c)) {
				n = c - (0x61 - 10);
			}
			else {
				break;
			}
		}

		if (n >= b) {
			break;
		}

		j = (r * b) + n;
		if (UNLIKELY(j > UINT32_MAX)) {
			end[0]    = s;

			result[0] = UINT32_MAX;
			return 0;
		}
		r = (uint32) j;
	}

	end[0]    = s;
	result[0] = r;
	return 1;
}

uintxx
str2i32(const char* src, const char** end, int32* result, uintxx base)
{
	const char* s;
	const char* e[1];
	uintxx isnegative;
	uint32 r[1];
	ASSERT(src && result);

	s = src;
	while (ctb_isspace(s[0]))
		s++;

	for(isnegative = 0; s[0]; s++) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; continue;
			case 0x2b: isnegative = 0; continue;
		}
		break;
	}

	base = getbase(&s, base);
	if (base == 0) {
		if (end)
			end[0] = src;
		return STR2INT_EBADBASE;
	}

	parseu32(s, e, r, base);
	if (s == e[0]) {
		result[0] = 0;
		return STR2INT_ENAN;
	}
	else {
		if (end)
			end[0] = e[0];
	}

	if (isnegative) {
		if (r[0] > (uint32) INT32_MIN) {
			result[0] = INT32_MIN;
			return STR2INT_EOVERFLOW;
		}
		result[0] = -r[0];
	}
	else {
		if (r[0] > (uint32) INT32_MAX) {
			result[0] = INT32_MAX;
			return STR2INT_EOVERFLOW;
		}
		result[0] = +r[0];
	}
	return 0;
}

uintxx
str2u32(const char* src, const char** end, uint32* result, uintxx base)
{
	const char* s;
	const char* e[1];
	ASSERT(src && result);

	s = src;
	while (ctb_isspace(s[0]))
		s++;

	for(;s[0]; s++) {
		switch (s[0]) {
			case 0x2d: continue;
			case 0x2b: continue;
		}
		break;
	}

	base = getbase(&s, base);
	if (base == 0) {
		if (end)
			end[0] = s;
		return STR2INT_EBADBASE;
	}

	if (parseu32(s, e, result, base) == 0) {
		return STR2INT_EOVERFLOW;
	}
	if (s == e[0]) {
		return STR2INT_ENAN;
	}
	else {
		if (end)
			end[0] = e[0];
	}
	return 0;
}


#if defined(__has_builtin)
	#if __has_builtin(__builtin_mul_overflow)
		#define HAS_SAFE_MUL
	#endif
	#if __has_builtin(__builtin_add_overflow)
		#define HAS_SAFE_ADD
	#endif

	#if defined(HAS_SAFE_MUL) && defined(HAS_SAFE_ADD)
		#define HAS_SAFE_OPERATORS 1
	#endif

	#undef HAS_SAFE_MUL
	#undef HAS_SAFE_ADD
#endif

#if !defined(HAS_SAFE_OPERATORS)
	#if defined __GNUC__ && __GNUC__ >= 5
		#define HAS_SAFE_OPERATORS 1
	#else
		#define HAS_SAFE_OPERATORS 0
	#endif
#endif


#if HAS_SAFE_OPERATORS == 0

#define GENLIMITx1(A, D) ((A) / (D))
#define GENLIMITx2(A, D) GENLIMITx1(A, (D) + 0), GENLIMITx1(A, (D + 1))
#define GENLIMITx4(A, D) GENLIMITx2(A, (D) + 0), GENLIMITx2(A, (D + 2))
#define GENLIMITx8(A, D) GENLIMITx4(A, (D) + 0), GENLIMITx2(A, (D + 4))

static const uint64 limit64[] = {
	GENLIMITx8(UINT64_MAX, 2+(8*0)),
	GENLIMITx8(UINT64_MAX, 2+(8*1)),
	GENLIMITx8(UINT64_MAX, 2+(8*2))
};

#undef GENLIMITx1
#undef GENLIMITx2
#undef GENLIMITx4
#undef GENLIMITx8
#endif


static bool
parseu64(const char* src, const char** end, uint64* result, uintxx b)
{
	const char* s;
	bool overflow;
	uint64 r;
	uint64 n;
	uint64 c;
#if HAS_SAFE_OPERATORS == 0
	uint64 limit1;
	uint64 limit2;

	limit2 = UINT64_MAX - ((limit1 = limit64[b - 2]) * b);
#endif

	overflow = 0;
	s = src;
	for (r = 0; (c = s[0]); s++) {
		if (ctb_isdigit(c)) {
			n = c - 0x30;
		}
		else {
			c = c | 0x20;
			if (ctb_isalpha(c)) {
				n = c - (0x61 - 10);
			}
			else {
				break;
			}
		}

		if (n >= b) {
			break;
		}
#if HAS_SAFE_OPERATORS == 0
		if (r > limit1 || (r == limit1 && n > limit2)) {
			overflow = 1;
			break;
		}
		r = (r * b) + n;
#else
		if (__builtin_mul_overflow(r, b, &r)) {
			overflow = 1;
			break;
		}
		if (__builtin_add_overflow(r, n, &r)) {
			overflow = 1;
			break;
		}
#endif
	}

	if (end)
		end[0] = s;

	if (UNLIKELY(overflow)) {
		result[0] = UINT64_MAX;
		return 0;
	}
	result[0] = r;
	return 1;
}

#undef HAS_SAFE_OPERATORS


uintxx
str2i64(const char* src, const char** end, int64* result, uintxx base)
{
	uintxx isnegative;
	uint64 r[1];
	ASSERT(src && result);

	while (ctb_isspace(src[0]))
		src++;

	for(isnegative = 0; src[0]; src++) {
		switch (src[0]) {
			case 0x2d: isnegative = 1; continue;
			case 0x2b: isnegative = 0; continue;
		}
		break;
	}

	base = getbase(&src, base);
	if (base == 0) {
		if (end)
			end[0] = src;
		return STR2INT_EBADBASE;
	}

	parseu64(src, end, r, base);

	if (isnegative) {
		if (r[0] > (uint64) INT64_MIN) {
			result[0] = INT32_MIN;
			return STR2INT_EOVERFLOW;
		}
		result[0] = -r[0];
	}
	else {
		if (r[0] > (uint64) INT64_MAX) {
			result[0] = INT64_MAX;
			return STR2INT_EOVERFLOW;
		}
		result[0] = +r[0];
	}
	return 0;
}

uintxx
str2u64(const char* src, const char** end, uint64* result, uintxx base)
{
	while (ctb_isspace(src[0]))
		src++;

	for(;src[0]; src++) {
		switch (src[0]) {
			case 0x2d: continue;
			case 0x2b: continue;
		}
		break;
	}

	base = getbase(&src, base);
	if (base == 0) {
		if (end)
			end[0] = src;
		return STR2INT_EBADBASE;
	}

	if (parseu64(src, end, result, base) == 0) {
		return STR2INT_EOVERFLOW;
	}
	return 0;
}

