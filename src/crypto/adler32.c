/*
 * Copyright (C) 2014, jpn
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

#include <ctoolbox/crypto/adler32.h>


/* largest prime smaller than 65536 */
#define ADLER_BASE 65521

#define ADLER32_SLICEBY8 \
		b += (a += *data++); \
		b += (a += *data++); \
		b += (a += *data++); \
		b += (a += *data++); \
		b += (a += *data++); \
		b += (a += *data++); \
		b += (a += *data++); \
		b += (a += *data++);

uint32
adler32_slideby8(uint32 adler, const uint8* data, uintxx size)
{
	uint32 a;
	uint32 b;
	uint32 ra;
	uint32 rb;
	uintxx i;
	CTB_ASSERT(data);

	a = 0xffff & adler;
	b = 0xffff & adler >> 16;

	i = 32;
	for (; size >= 512; size -= 512) {
		do {
			ADLER32_SLICEBY8
			ADLER32_SLICEBY8
		} while(--i);
		i = 32;

		/* modulo reduction */
		ra = a >> 16;
		rb = b >> 16;
		a = (a & 0xffff) + ((ra << 4) - ra);
		b = (b & 0xffff) + ((rb << 4) - rb);
	}

	for (; size >= 16; size -= 16) {
		ADLER32_SLICEBY8
		ADLER32_SLICEBY8
	}

	while (size) {
		b += (a += *data++);
		size--;
	}

	/* modulo reduction */
	ra = a >> 16;
	rb = b >> 16;
	a = (a & 0xffff) + ((ra << 4) - ra);
	b = (b & 0xffff) + ((rb << 4) - rb);
	if (a >= ADLER_BASE)
		a -= ADLER_BASE;
	if (b >= ADLER_BASE)
		b -= ADLER_BASE;

	return (b << 16) | a;
}

#undef ADLER32_SLICEBY8
