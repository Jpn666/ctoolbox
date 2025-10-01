/*
 * Copyright (C) 2025, jpn
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

#include <ctoolbox/str2int.h>
#include <ctoolbox/ctype.h>
#include <ctoolbox/ckdint.h>


static const uint8 hexamap[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff
};


CTB_INLINE intxx
getbase(const uint8** src, intxx base)
{
	intxx c;
	intxx b;
	const uint8* s;

	if (base) {
		if (base < 2 || base > 16) {
			return 0;
		}
		if (base != 0x02 && base != 0x08 && base != 0x10) {
			return base;
		}
	}

	s = src[0];
	if (s[0] ^ 0x30) {
		if (base == 0) {
			return 10;
		}
		return base;
	}

	c = s[1] | 0x20;
	switch (c) {
		case 'x': b = 0x10; break;
		case 'o': b = 0x08; break;
		case 'b': b = 0x02; break;
		default:
			b = 0;
	}

	if (base) {
		if (b) {
			if (b == base) {
				src[0] += 2;
				return b;
			}
		}
		return base;
	}
	if (b) {
		c = s[2];
		if (b ^ 0x10) {
			if (c < 0x30 || c > (b + 0x30)) {
				return 10;
			}
			src[0] += 2;
			return b;
		}
		c = c | 0x20;
		if ((c < 0x30 || c > 0x39) == 0 && (c < 0x61 || c > (b + 0x61)) == 0) {
			return 10;
		}
		src[0] += 2;
		return b;
	}
	return 10;
}


static const uint8 maxdigits32[] = {
	0, 0, 32, 21, 16, 14, 13, 12, 11, 11, 10, 10, 9, 9, 9, 9, 8,
};

static eintxx
parseu32(const uint8* src, const uint8** end, intxx base, uint32* r)
{
	uintxx total;
	uintxx limit;
	uintxx j;
	uint32 n;
	uint32 m;
	uint32 b;
	uint32 v[1];
	uint32 c;
	const uint8* s;

	b = (uint32) base;
	if (b <= 10) {
		m = 0x30 + b - 1;
		for (s = src; s[0]; s++) {
			if (s[0] < 0x30 || s[0] > m) {
				break;
			}
		}
	}
	else {
		m = 0x61 + (b - 1 - 10);
		for (s = src; s[0]; s++) {
			c = s[0] | 0x20;
			if ((c < 0x30 || c > 0x39) == 0 && (c < 0x61 || c > m) == 0) {
				break;
			}
		}
	}

	if (CTB_EXPECT0(s == src)) {
		if (end)
			end[0] = src;
		r[0] = 0x00;
		return STR2INT_ENAN;
	}

	if (CTB_EXPECT0(src[0] == 0x30)) {
		while (src[0] == 0x30) {
			src++;
		}

		total = (uintxx) (s - src);
		if (total == 0) {
			r[0] = 0x00;
			if (end)
				end[0] = src;
			return 0;
		}
	}
	else {
		total = (uintxx) (s - src);
	}

	limit = maxdigits32[b];
	if (CTB_EXPECT1(total <= limit)) {
		if (b <= 10) {
			n = 0;
			for (j = 0, total--; j < total; j++) {
				n = n * b + (*src++ - 0x30);
			}
			total++;
			if (CTB_EXPECT0(total == limit)) {
				if (ckdu32_mul(n, b, v))
					goto L1;
				n = v[0];

				c = *src++ - 0x30;
				if (ckdu32_add(n, c, v))
					goto L1;
				n = v[0];
			}
			else {
				n = n * b + (*src++ - 0x30);
			}
		}
		else {
			n = 0;
			for (j = 0, total--; j < total; j++) {
				n = n * b + hexamap[*src++ - 0x30];
			}
			total++;
			if (CTB_EXPECT0(total == limit)) {
				if (ckdu32_mul(n, b, v))
					goto L1;
				n = v[0];

				c = hexamap[*src++ - 0x30];
				if (ckdu32_add(n, c, v))
					goto L1;
				n = v[0];
			}
			else {
				n = n * b + hexamap[*src++ - 0x30];
			}
		}

		r[0] = (uint32) n;
		if (end)
			end[0] = s;
		return 0;
	}
L1:
	r[0] = 0xffffffffu;
	if (end)
		end[0] = s;
	return STR2INT_ERANGE;
}

TToIntResult
strtou32(const uint8* src, const uint8** end, intxx base)
{
	const uint8* s;
	const uint8* e[1];
	int32 isnegative;
	uint32 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	s = src;
	while (ctb_isspace(s[0]))
		s++;

	isnegative = 0;
	for(; s[0]; s++) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; continue;
			case 0x2b: isnegative = 0; continue;
		}
		break;
	}

	result.value.asu32 = 0;
	base = getbase(&s, base);
	if (base == 0) {
		if (end)
			end[0] = src;
		result.error = STR2INT_EBASE;
		return result;
	}

	if (parseu32(s, e, base, &u) == STR2INT_ERANGE) {
		if (end)
			end[0] = e[0];

		result.value.asu32 = 0xffffffff;
		result.error = STR2INT_ERANGE;
		return result;
	}
	if (s == e[0]) {
		if (end)
			end[0] = src;
		result.error = STR2INT_ENAN;
		return result;
	}


	if (end)
		end[0] = e[0];

	if (isnegative) {
		u = (uint32) -((int32) u);
	}

	result.value.asu32 = u;
	result.error = 0;
	return result;
}

TToIntResult
strtoi32(const uint8* src, const uint8** end, intxx base)
{
	const uint8* s;
	const uint8* e[1];
	int32 isnegative;
	uint32 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	s = src;
	while (ctb_isspace(s[0]))
		s++;

	isnegative = 0;
	for(; s[0]; s++) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; continue;
			case 0x2b: isnegative = 0; continue;
		}
		break;
	}

	result.value.asi32 = 0;
	base = getbase(&s, base);
	if (base == 0) {
		if (end)
			end[0] = src;
		result.error = STR2INT_EBASE;
		return result;
	}

	if (parseu32(s, e, base, &u) == STR2INT_ERANGE) {
		u = 0xffffffff;
		goto L1;
	}
	if (s == e[0]) {
		if (end)
			end[0] = src;
		result.error = STR2INT_ENAN;
		return result;
	}

L1:
	if (end)
		end[0] = e[0];

	if (isnegative) {
		if (u > (uint32) INT32_MIN) {
			result.value.asi32 = INT32_MIN;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi32 = -((int32) u);
	}
	else {
		if (u > (uint32) INT32_MAX) {
			result.value.asi32 = INT32_MAX;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi32 = (int32) u;
	}

	result.error = 0;
	return result;
}


static const uint8 maxdigits64[] = {
	0, 0, 64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16
};

static eintxx
parseu64(const uint8* src, const uint8** end, intxx base, uint64* r)
{
	uintxx total;
	uintxx limit;
	uintxx j;
	uint64 n;
	uint32 m;
	uint32 b;
	uint64 v[1];
	uint64 c;
	const uint8* s;

	b = (uint32) base;
	if (b <= 10) {
		m = 0x30 + b - 1;
		for (s = src; s[0]; s++) {
			if (s[0] < 0x30 || s[0] > m) {
				break;
			}
		}
	}
	else {
		m = 0x61 + (b - 1 - 10);
		for (s = src; s[0]; s++) {
			c = s[0] | 0x20;
			if ((c < 0x30 || c > 0x39) == 0 && (c < 0x61 || c > m) == 0) {
				break;
			}
		}
	}

	if (CTB_EXPECT0(s == src)) {
		if (end)
			end[0] = src;
		r[0] = 0x00;
		return STR2INT_ENAN;
	}

	if (CTB_EXPECT0(src[0] == 0x30)) {
		while (src[0] == 0x30) {
			src++;
		}

		total = (uintxx) (s - src);
		if (total == 0) {
			r[0] = 0x00;
			if (end)
				end[0] = src;
			return 0;
		}
	}
	else {
		total = (uintxx) (s - src);
		if (total == 0) {
			return STR2INT_ENAN;
		}
	}

	limit = maxdigits64[b];
	if (CTB_EXPECT1(total <= limit)) {
		if (b <= 10) {
			n = 0;
			for (j = 0, total--; j < total; j++) {
				n = n * b + (*src++ - 0x30);
			}
			total++;
			if (CTB_EXPECT0(total == limit)) {
				if (ckdu64_mul(n, b, v))
					goto L1;
				n = v[0];

				c = *src++ - 0x30;
				if (ckdu64_add(n, c, v))
					goto L1;
				n = v[0];
			}
			else {
				n = n * b + (*src++ - 0x30);
			}
		}
		else {
			n = 0;
			for (j = 0, total--; j < total; j++) {
				n = n * b + hexamap[*src++ - 0x30];
			}
			total++;
			if (CTB_EXPECT0(total == limit)) {
				if (ckdu64_mul(n, b, v))
					goto L1;
				n = v[0];

				c = hexamap[*src++ - 0x30];
				if (ckdu64_add(n, c, v))
					goto L1;
				n = v[0];
			}
			else {
				n = n * b + hexamap[*src++ - 0x30];
			}
		}

		r[0] = (uint64) n;
		if (end)
			end[0] = s;
		return 0;
	}
L1:
	r[0] = 0xffffffffffffffffu;
	if (end)
		end[0] = s;
	return STR2INT_ERANGE;
}


TToIntResult
strtou64(const uint8* src, const uint8** end, intxx base)
{
	const uint8* s;
	const uint8* e[1];
	int32 isnegative;
	uint64 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	s = src;
	while (ctb_isspace(s[0]))
		s++;

	isnegative = 0;
	for(; s[0]; s++) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; continue;
			case 0x2b: isnegative = 0; continue;
		}
		break;
	}

	result.value.asu64 = 0;
	base = getbase(&s, base);
	if (base == 0) {
		if (end)
			end[0] = src;
		result.error = STR2INT_EBASE;
		return result;
	}

	if (parseu64(s, e, base, &u) == STR2INT_ERANGE) {
		if (end)
			end[0] = e[0];

		result.value.asu64 = 0xffffffffffffffff;
		result.error = STR2INT_ERANGE;
		return result;
	}
	if (s == e[0]) {
		if (end)
			end[0] = src;
		result.error = STR2INT_ENAN;
	}

	if (end)
		end[0] = e[0];

	if (isnegative) {
		u = (uint64) -((int64) u);
	}

	result.value.asu64 = u;
	result.error = 0;
	return result;
}

TToIntResult
strtoi64(const uint8* src, const uint8** end, intxx base)
{
	const uint8* s;
	const uint8* e[1];
	int32 isnegative;
	uint64 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	s = src;
	while (ctb_isspace(s[0]))
		s++;

	isnegative = 0;
	for(; s[0]; s++) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; continue;
			case 0x2b: isnegative = 0; continue;
		}
		break;
	}

	result.value.asi64 = 0;
	base = getbase(&s, base);
	if (base == 0) {
		if (end)
			end[0] = src;
		result.error = STR2INT_EBASE;
		return result;
	}

	if (parseu64(s, e, base, &u) == STR2INT_ERANGE) {
		u = 0xffffffffffffffff;
		goto L1;
	}
	if (s == e[0]) {
		if (end)
			end[0] = src;
		result.error = STR2INT_ENAN;
		return result;
	}

L1:
	if (end)
		end[0] = e[0];

	if (isnegative) {
		if (u > (uint64) INT64_MIN) {
			result.value.asi64 = INT64_MIN;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi64 = -((int64) u);
	}
	else {
		if (u > (uint64) INT64_MAX) {
			result.value.asi64 = INT64_MAX;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi64 = (int64) u;
	}

	result.error = 0;
	return result;
}


#if defined(__clang__) && defined(CTB_FASTUNALIGNED)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcast-align"
#endif

static eintxx
parsedecimal32(const uint8* src, uintxx total, const uint8** end, uint32* r)
{
	uint32 n;
	const uint8* s;
	const uint8* p;

	p = (s = src) + total;
#if defined(CTB_FASTUNALIGNED)
	for (; p - s >= 4; s += 4) {
		uint32 v;

		v = ((const uint32*) s)[0];
		if (((v & (v + 0x06060606u) & 0xf0f0f0f0u) == 0x30303030u) == 0) {
			if (s[0] < 0x30 || s[0] > 0x39) {
				goto L1;
			}
			s++;
			if (s[0] < 0x30 || s[0] > 0x39) {
				goto L1;
			}
			s++;
			if (s[0] < 0x30 || s[0] > 0x39) {
				goto L1;
			}
			s++;
			goto L1;
		}
	}
#endif

	for (; p - s >= 1; s += 1) {
		if (s[0] < 0x30 || s[0] > 0x39) {
			break;
		}
	}

#if defined(CTB_FASTUNALIGNED)
L1:
#endif
	if (CTB_EXPECT0(s == src)) {
		if (end)
			end[0] = src;
		r[0] = 0x00;
		return STR2INT_ENAN;
	}

	if (CTB_EXPECT0(src[0] == 0x30)) {
		while (src[0] == 0x30) {
			src++;
		}

		total = (uintxx) (s - src);
		if (total == 0) {
			r[0] = 0x00;
			if (end)
				end[0] = src;
			return 0;
		}
	}
	else {
		total = (uintxx) (s - src);
	}

	if (CTB_EXPECT1(total <= 10)) {
		uint32 a;
		uint32 b;
		uint32 c;
		uint32 d;
		uint32 v[1];

		n = 0;
		if (total >= 4) {
			a = (src[0] - 0x30) * 1000u;
			b = (src[1] - 0x30) * 100u;
			c = (src[2] - 0x30) * 10u;
			d = (src[3] - 0x30);
			n = (n * 10000) + (a + b + c + d);
			src += 4;

			if (total >= 8) {
				a = (src[0] - 0x30) * 1000u;
				b = (src[1] - 0x30) * 100u;
				c = (src[2] - 0x30) * 10u;
				d = (src[3] - 0x30);
				n = (n * 10000) + (a + b + c + d);
				src += 4;

				if (total == 10) {
					n = n * 10 + (*src++ - 0x30);

					c = *src++ - 0x30;
					if (ckdu32_mul(n, 10, v))
						goto L2;
					n = v[0];

					if (ckdu32_add(n, c, v))
						goto L2;
					n = v[0];
				}
				else {
					switch (s - src) {
						case 2: n = n * 10 + (*src++ - 0x30); /* fallthrough */
						case 1: n = n * 10 + (*src++ - 0x30);
					}
				}
			}
			else {
				switch (s - src) {
					case 3: n = n * 10 + (*src++ - 0x30); /* fallthrough */
					case 2: n = n * 10 + (*src++ - 0x30); /* fallthrough */
					case 1: n = n * 10 + (*src++ - 0x30);
				}
			}
		}
		else {
			switch (total) {
				case 3: n = n * 10 + (*src++ - 0x30); /* fallthrough */
				case 2: n = n * 10 + (*src++ - 0x30); /* fallthrough */
				case 1: n = n * 10 + (*src++ - 0x30);
			}
		}

		r[0] = (uint32) n;
		if (end)
			end[0] = s;
		return 0;
	}

L2:
	r[0] = 0xffffffffu;

	if (end)
		end[0] = s;
	return STR2INT_ERANGE;
}

TToIntResult
dcmltou32(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint32 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}

	result.error = parsedecimal32(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ENAN) {
			if (end)
				end[0] = src;
		}
		result.value.asu32 = u;
		return result;
	}

	if (isnegative) {
		u = (uint32) -((int32) u);
	}

	result.value.asu32 = u;
	return result;
}

TToIntResult
dcmltoi32(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint32 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}

	result.error = parsedecimal32(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ERANGE) {
			goto L1;
		}

		if (end)
			end[0] = src;
		result.value.asi32 = 0;
		return result;
	}

L1:
	if (isnegative) {
		if (u > (uint32) INT32_MIN) {
			result.value.asi32 = INT32_MIN;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi32 = -((int32) u);
	}
	else {
		if (u > (uint32) INT32_MAX) {
			result.value.asi32 = INT32_MAX;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi32 = +((int32) u);
	}

	return result;
}

static eintxx
parsedecimal64(const uint8* src, uintxx total, const uint8** end, uint64* r)
{
	uint64 n;
	const uint8* s;
	const uint8* p;

	p = (s = src) + total;
#if defined(CTB_FASTUNALIGNED)
	for (; p - s >= 4; s += 4) {
		uint32 v;

		v = ((const uint32*) s)[0];
		if (((v & (v + 0x06060606u) & 0xf0f0f0f0u) == 0x30303030u) == 0) {
			if (s[0] < 0x30 || s[0] > 0x39) {
				goto L1;
			}
			s++;
			if (s[0] < 0x30 || s[0] > 0x39) {
				goto L1;
			}
			s++;
			if (s[0] < 0x30 || s[0] > 0x39) {
				goto L1;
			}
			s++;
			goto L1;
		}
	}
#endif

	for (; p - s >= 1; s += 1) {
		if (s[0] < 0x30 || s[0] > 0x39) {
			break;
		}
	}

#if defined(CTB_FASTUNALIGNED)
L1:
#endif
	if (CTB_EXPECT0(s == src)) {
		if (end)
			end[0] = src;
		r[0] = 0x00;
		return STR2INT_ENAN;
	}

	if (CTB_EXPECT0(src[0] == 0x30)) {
		while (src[0] == 0x30) {
			src++;
		}

		total = (uintxx) (s - src);
		if (total == 0) {
			r[0] = 0x00;
			if (end)
				end[0] = src;
			return 0;
		}
	}
	else {
		total = (uintxx) (s - src);
	}

	if (CTB_EXPECT1(total <= 20)) {
		uint32 a;
		uint32 b;
		uint32 c;
		uint32 d;
		uint64 v[1];

		n = 0;
		if (total >= 8) {
			a = (src[0] - 0x30) * 1000u;
			b = (src[1] - 0x30) * 100u;
			c = (src[2] - 0x30) * 10u;
			d = (src[3] - 0x30);
			n = (n * 10000) + (a + b + c + d);
			src += 4;

			a = (src[0] - 0x30) * 1000u;
			b = (src[1] - 0x30) * 100u;
			c = (src[2] - 0x30) * 10u;
			d = (src[3] - 0x30);
			n = (n * 10000) + (a + b + c + d);
			src += 4;

			if (total >= 16) {
				a = (src[0] - 0x30) * 1000u;
				b = (src[1] - 0x30) * 100u;
				c = (src[2] - 0x30) * 10u;
				d = (src[3] - 0x30);
				n = (n * 10000) + (a + b + c + d);
				src += 4;

				a = (src[0] - 0x30) * 1000u;
				b = (src[1] - 0x30) * 100u;
				c = (src[2] - 0x30) * 10u;
				d = (src[3] - 0x30);
				n = (n * 10000) + (a + b + c + d);
				src += 4;

				if (total == 20) {
					n = n * 10 + (*src++ - 0x30);
					n = n * 10 + (*src++ - 0x30);
					n = n * 10 + (*src++ - 0x30);

					c = *src++ - 0x30;
					if (ckdu64_mul(n, 10, v))
						goto L2;
					n = v[0];

					if (ckdu64_add(n, c, v))
						goto L2;
					n = v[0];
				}
				else {
					switch (s - src) {
						case 3: n = n * 10 + (*src++ - 0x30); /* fallthrough */
						case 2: n = n * 10 + (*src++ - 0x30); /* fallthrough */
						case 1: n = n * 10 + (*src++ - 0x30);
					}
				}
			}
			else {
				if (total >= 12) {
					a = (src[0] - 0x30) * 1000u;
					b = (src[1] - 0x30) * 100u;
					c = (src[2] - 0x30) * 10u;
					d = (src[3] - 0x30);
					n = (n * 10000) + (a + b + c + d);
					src += 4;
				}

				switch (s - src) {
					case 3: n = n * 10 + (*src++ - 0x30); /* fallthrough */
					case 2: n = n * 10 + (*src++ - 0x30); /* fallthrough */
					case 1: n = n * 10 + (*src++ - 0x30);
				}
			}
		}
		else {
			if (total >= 4) {
				a = (src[0] - 0x30) * 1000u;
				b = (src[1] - 0x30) * 100u;
				c = (src[2] - 0x30) * 10u;
				d = (src[3] - 0x30);
				n = (n * 10000) + (a + b + c + d);
				src += 4;
			}

			switch (s - src) {
				case 3: n = n * 10 + (*src++ - 0x30); /* fallthrough */
				case 2: n = n * 10 + (*src++ - 0x30); /* fallthrough */
				case 1: n = n * 10 + (*src++ - 0x30);
			}
		}

		r[0] = n;
		if (end)
			end[0] = s;
		return 0;
	}

L2:
	r[0] = 0xffffffffffffffffull;

	if (end)
		end[0] = s;
	return STR2INT_ERANGE;
}

#if defined(__clang__) && defined(CTB_FASTUNALIGNED)
	#pragma clang diagnostic pop
#endif


TToIntResult
dcmltou64(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint64 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}

	result.error = parsedecimal64(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ENAN) {
			if (end)
				end[0] = src;
		}
		result.value.asu64 = u;
		return result;
	}

	if (isnegative) {
		u = (uint64) -((int64) u);
	}

	result.value.asu64 = u;
	return result;
}

TToIntResult
dcmltoi64(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint64 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}

	result.error = parsedecimal64(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ERANGE) {
			goto L1;
		}

		if (end)
			end[0] = src;
		result.value.asi64 = 0;
		return result;
	}

L1:
	if (isnegative) {
		if (u > (uint64) INT64_MIN) {
			result.value.asi64 = INT64_MIN;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi64 = -((int64) u);
	}
	else {
		if (u > (uint64) INT64_MAX) {
			result.value.asi64 = INT64_MAX;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi64 = +((int64) u);
	}

	return result;
}


static eintxx
parsehexa32(const uint8* src, uintxx total, const uint8** end, uint32* r)
{
	uint32 n;
	const uint8* s;
	const uint8* p;

	p = (s = src) + total;
	for (; p - s >= 1; s++) {
		uint32 c;

		c = s[0] | 0x20;
		if ((c < 0x30 || c > 0x39) == 0 && (c < 0x61 || c > 0x66) == 0) {
			break;
		}
	}

	if (CTB_EXPECT0(s == src)) {
		if (end)
			end[0] = src;
		r[0] = 0x00;
		return STR2INT_ENAN;
	}

	if (CTB_EXPECT0(src[0] == 0x30)) {
		while (src[0] == 0x30) {
			src++;
		}

		total = (uintxx) (s - src);
		if (total == 0) {
			r[0] = 0x00;
			if (end)
				end[0] = src;
			return 0;
		}
	}
	else {
		total = (uintxx) (s - src);
	}

	if (CTB_EXPECT1(total <= 8)) {
		uint32 a;
		uint32 b;
		uint32 c;
		uint32 d;

		n = 0;
		if (total >= 4) {
			a = (uint32) hexamap[src[0] - 0x30] << 014;
			b = (uint32) hexamap[src[1] - 0x30] << 010;
			c = (uint32) hexamap[src[2] - 0x30] << 004;
			d = (uint32) hexamap[src[3] - 0x30];
			n = a + b + c + d;
			src += 4;

			if (total == 8) {
				a = (uint32) hexamap[src[0] - 0x30] << 014;
				b = (uint32) hexamap[src[1] - 0x30] << 010;
				c = (uint32) hexamap[src[2] - 0x30] << 004;
				d = (uint32) hexamap[src[3] - 0x30];
				n = (n << 020) + a + b + c + d;
				src += 4;
			}
		}
		switch (s - src) {
			case 3: n = (n << 4) + hexamap[*src++ - 0x30];  /* fallthrough */
			case 2: n = (n << 4) + hexamap[*src++ - 0x30];  /* fallthrough */
			case 1: n = (n << 4) + hexamap[*src++ - 0x30];  /* fallthrough */
			case 0:
				break;
		}

		r[0] = (uint32) n;
		if (end)
			end[0] = s;
		return 0;
	}

	r[0] = 0xffffffffu;
	if (end)
		end[0] = s;
	return STR2INT_ERANGE;
}


TToIntResult
hexatou32(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint32 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}
	if (e - s >= 2) {
		if (s[0] == 0x30) {
			uint32 c;

			c = s[1] | 0x20;
			if (c == 0x78) {
				s++;
				s++;
			}
		}
	}

	result.error = parsehexa32(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ENAN) {
			if (end)
				end[0] = src;
		}
		result.value.asu32 = u;
		return result;
	}

	if (isnegative) {
		u = (uint32) -((int32) u);
	}

	result.value.asu32 = u;
	return result;
}

struct TToIntResult
hexatoi32(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint32 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}
	if (e - s >= 2) {
		if (s[0] == 0x30) {
			uint32 c;

			c = s[1] | 0x20;
			if (c == 0x78) {
				s++;
				s++;
			}
		}
	}

	result.error = parsehexa32(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ERANGE) {
			goto L1;
		}

		if (end)
			end[0] = src;
		result.value.asi32 = 0;
		return result;
	}

L1:
	if (isnegative) {
		if (u > (uint32) INT32_MIN) {
			result.value.asi32 = INT32_MIN;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi32 = -((int32) u);
	}
	else {
		if (u > (uint32) INT32_MAX) {
			result.value.asi32 = INT32_MAX;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi32 = +((int32) u);
	}

	result.error = 0;
	return result;
}


static eintxx
parsehexa64(const uint8* src, uintxx total, const uint8** end, uint64* r)
{
	uint64 n;
	const uint8* s;
	const uint8* p;

	p = (s = src) + total;
	for (; p - s >= 1; s++) {
		uint32 c;

		c = s[0] | 0x20;
		if ((c < 0x30 || c > 0x39) == 0 && (c < 0x61 || c > 0x66) == 0) {
			break;
		}
	}

	if (CTB_EXPECT0(s == src)) {
		if (end)
			end[0] = src;
		r[0] = 0x00;
		return STR2INT_ENAN;
	}

	if (CTB_EXPECT0(src[0] == 0x30)) {
		while (src[0] == 0x30) {
			src++;
		}

		total = (uintxx) (s - src);
		if (total == 0) {
			r[0] = 0x00;
			if (end)
				end[0] = src;
			return 0;
		}
	}
	else {
		total = (uintxx) (s - src);
	}

	if (CTB_EXPECT1(total <= 16)) {
		uint32 a;
		uint32 b;
		uint32 c;
		uint32 d;

		n = 0;
		if (total >= 8) {
			a = (uint32) hexamap[src[0] - 0x30] << 014;
			b = (uint32) hexamap[src[1] - 0x30] << 010;
			c = (uint32) hexamap[src[2] - 0x30] << 004;
			d = (uint32) hexamap[src[3] - 0x30];
			n = a + b + c + d;
			src += 4;

			a = (uint32) hexamap[src[0] - 0x30] << 014;
			b = (uint32) hexamap[src[1] - 0x30] << 010;
			c = (uint32) hexamap[src[2] - 0x30] << 004;
			d = (uint32) hexamap[src[3] - 0x30];
			n = (n << 020) + a + b + c + d;
			src += 4;

			if (total == 16) {
				a = (uint32) hexamap[src[0] - 0x30] << 014;
				b = (uint32) hexamap[src[1] - 0x30] << 010;
				c = (uint32) hexamap[src[2] - 0x30] << 004;
				d = (uint32) hexamap[src[3] - 0x30];
				n = (n << 020) + a + b + c + d;
				src += 4;

				a = (uint32) hexamap[src[0] - 0x30] << 014;
				b = (uint32) hexamap[src[1] - 0x30] << 010;
				c = (uint32) hexamap[src[2] - 0x30] << 004;
				d = (uint32) hexamap[src[3] - 0x30];
				n = (n << 020) + a + b + c + d;
				src += 4;
			}
			else {
				if (total >= 12) {
					a = (uint32) hexamap[src[0] - 0x30] << 014;
					b = (uint32) hexamap[src[1] - 0x30] << 010;
					c = (uint32) hexamap[src[2] - 0x30] << 004;
					d = (uint32) hexamap[src[3] - 0x30];
					n = (n << 020) + a + b + c + d;
					src += 4;
				}
			}
		}
		else {
			if (total >= 4) {
				a = (uint32) hexamap[src[0] - 0x30] << 014;
				b = (uint32) hexamap[src[1] - 0x30] << 010;
				c = (uint32) hexamap[src[2] - 0x30] << 004;
				d = (uint32) hexamap[src[3] - 0x30];
				n = a + b + c + d;
				src += 4;
			}
		}
		switch (s - src) {
			case 3: n = (n << 4) + hexamap[s[-3] - 0x30];  /* fallthrough */
			case 2: n = (n << 4) + hexamap[s[-2] - 0x30];  /* fallthrough */
			case 1: n = (n << 4) + hexamap[s[-1] - 0x30];  /* fallthrough */
			case 0:
				break;
		}

		r[0] = (uint64) n;
		if (end)
			end[0] = s;
		return 0;
	}

	r[0] = 0xffffffffffffffffull;
	if (end)
		end[0] = s;
	return STR2INT_ERANGE;
}

TToIntResult
hexatou64(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint64 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}
	if (e - s >= 2) {
		if (s[0] == 0x30) {
			uint32 c;

			c = s[1] | 0x20;
			if (c == 0x78) {
				s++;
				s++;
			}
		}
	}

	result.error = parsehexa64(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ENAN) {
			if (end)
				end[0] = src;
		}
		result.value.asu64 = u;
		return result;
	}

	if (isnegative) {
		u = (uint64) -((int64) u);
	}

	result.value.asu64 = u;
	return result;
}

TToIntResult
hexatoi64(const uint8* src, intxx total, const uint8** end)
{
	const uint8* s;
	const uint8* e;
	uintxx isnegative;
	uint64 u;
	struct TToIntResult result;
	CTB_ASSERT(src);

	isnegative = 0;
	e = (s = src) + total;
	if (e > s) {
		switch (s[0]) {
			case 0x2d: isnegative = 1; s++; break;
			case 0x2b: isnegative = 0; s++; break;
		}
	}
	if (e - s >= 2) {
		if (s[0] == 0x30) {
			uint32 c;

			c = s[1] | 0x20;
			if (c == 0x78) {
				s++;
				s++;
			}
		}
	}

	result.error = parsehexa64(s, (uintxx) (e - s), end, &u);
	if (CTB_EXPECT0(result.error != 0)) {
		if (result.error == STR2INT_ERANGE) {
			goto L1;
		}

		if (end)
			end[0] = src;
		result.value.asi64 = 0;
		return result;
	}

L1:
	if (isnegative) {
		if (u > (uint64) INT64_MIN) {
			result.value.asi64 = INT64_MIN;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi64 = -((int64) u);
	}
	else {
		if (u > (uint64) INT64_MAX) {
			result.value.asi64 = INT64_MAX;
			result.error = STR2INT_ERANGE;
			return result;
		}
		result.value.asi64 = +((int64) u);
	}

	result.error = 0;
	return result;
}
