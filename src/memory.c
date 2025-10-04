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

#include <ctoolbox/memory.h>


#if defined(CTB_CFG_NOSTDLIB)

/*
 * Dummy functions */

static void*
localrequest(uintxx size, void* user)
{
	(void) size; (void) user;
	return NULL;
}

static void
localdispose(void* memory, uintxx size, void* user)
{
	(void) memory; (void) size; (void) user;
}

#else

#include <stdlib.h>

static void*
localrequest(uintxx size, void* user)
{
	(void) user;
	return malloc(size);
}

static void
localdispose(void* memory, uintxx size, void* user)
{
	(void) size; (void) user;
	free(memory);
}

#endif

/*
 * Fallback Allocator */

static struct TAllocator localallocator = {
	(TRequestFn) localrequest, (TDisposeFn) localdispose, NULL
};

static struct TAllocator* defaultallocator = &localallocator;


const TAllocator*
ctb_getdefaultallocator(void)
{
	return (void*) defaultallocator;
}

void
ctb_setdefaultallocator(TAllocator* allocator)
{
	if (allocator) {
		defaultallocator = allocator;
		return;
	}
	defaultallocator = &localallocator;
}


#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcast-align"
#endif

void
ctb_memcpy(void* destination, const void* source, uintxx size)
{
	const uint8* s;
	uint8* t;
	uintxx m;
	CTB_ASSERT(destination && source);

	s = source;
	t = destination;
	if (size < 8) {
		switch (size) {
			case 7: *t++ = *s++;  /* fallthrough */
			case 6: *t++ = *s++;  /* fallthrough */
			case 5: *t++ = *s++;  /* fallthrough */
			case 4: *t++ = *s++;  /* fallthrough */
			case 3: *t++ = *s++;  /* fallthrough */
			case 2: *t++ = *s++;  /* fallthrough */
			case 1: *t++ = *s++;  /* fallthrough */
			case 0:
				break;
		}
	}
	else {
#if defined(CTB_FASTUNALIGNED)
	#if defined(CTB_ENV64)
		const uint64* sxx;
		uint64* txx;
		uint64* exx;

		sxx = (const uint64*) (s);
		txx = (uint64*) (t);
		exx = (uint64*) (t + (m = (size >> 3) << 3));
		for (; exx > txx; txx += 1) {
			txx[0] = *sxx++;
		}
	#else
		const uint32* sxx;
		uint32* txx;
		uint32* exx;

		sxx = (const uint32*) (s);
		txx = (uint32*) (t);
		exx = (uint32*) (t + (m = (size >> 3) << 3));
		for (; exx > txx; txx += 2) {
			txx[0] = *sxx++;
			txx[1] = *sxx++;
		}
	#endif
#else
		uintxx rs;
		uintxx rt;

		rs = sizeof(uint32) - (((uintxx) s) & (sizeof(uint32) - 1));
		rt = sizeof(uint32) - (((uintxx) t) & (sizeof(uint32) - 1));
		if (rs ^ sizeof(uint32) || rt ^ sizeof(uint32)) {
			uint8* e;

			e = t + (m = (size >> 3) << 3);
			for (; e > t; t += 8) {
				t[0] = *s++;
				t[1] = *s++;
				t[2] = *s++;
				t[3] = *s++;

				t[4] = *s++;
				t[5] = *s++;
				t[6] = *s++;
				t[7] = *s++;
			}
		}
		else {
			const uint32* sxx;
			uint32* txx;
			uint32* exx;

			sxx = (const uint32*) (s);
			txx = (uint32*) (t);
			exx = (uint32*) (t + (m = (size >> 3) << 3));
			for (; exx > txx; txx += 2) {
				txx[0] = *sxx++;
				txx[1] = *sxx++;
			}
			s = (const uint8*) sxx;
			t = (uint8*) txx;
		}
#endif
		size -= m;
		if (size) {
#if defined(CTB_FASTUNALIGNED)
			s = (const uint8*) sxx;
			t = (uint8*) txx;
#endif
			switch (size) {
				case 7: *t++ = *s++;  /* fallthrough */
				case 6: *t++ = *s++;  /* fallthrough */
				case 5: *t++ = *s++;  /* fallthrough */
				case 4: *t++ = *s++;  /* fallthrough */
				case 3: *t++ = *s++;  /* fallthrough */
				case 2: *t++ = *s++;  /* fallthrough */
				case 1: *t++ = *s++;  /* fallthrough */
				case 0:
					break;
			}
		}
	}
}

void
ctb_memset(void* destination, uintxx value, uintxx size)
{
	uint8  s;
	uint8* t;
	uintxx m;
	CTB_ASSERT(destination);

	s = (uint8) value;
	t = destination;
	if (size < 8) {
		switch (size) {
			case 7: *t++ = s;  /* fallthrough */
			case 6: *t++ = s;  /* fallthrough */
			case 5: *t++ = s;  /* fallthrough */
			case 4: *t++ = s;  /* fallthrough */
			case 3: *t++ = s;  /* fallthrough */
			case 2: *t++ = s;  /* fallthrough */
			case 1: *t++ = s;  /* fallthrough */
			case 0:
				break;
		}
	}
	else {
#if defined(CTB_FASTUNALIGNED)
	#if defined(CTB_ENV64)
		uint64  sxx;
		uint64* txx;
		uint64* exx;

		sxx = s;
		sxx |= sxx << 010;
		sxx |= sxx << 020;
		sxx |= sxx << 040;
		txx = (uint64*) (t);
		exx = (uint64*) (t + (m = (size >> 3) << 3));
		for (; exx > txx; txx += 1) {
			txx[0] = sxx;
		}
	#else
		uint32  sxx;
		uint32* txx;
		uint32* exx;

		sxx = s;
		sxx |= sxx << 010;
		sxx |= sxx << 020;
		txx = (uint32*) (t);
		exx = (uint32*) (t + (m = (size >> 3) << 3));
		for (; exx > txx; txx += 2) {
			txx[0] = sxx;
			txx[1] = sxx;
		}
	#endif
#else
		uint32  sxx;
		uint32* txx;
		uint32* exx;
		uintxx rt;

		rt = sizeof(uint32) - (((uintxx) t) & (sizeof(uint32) - 1));
		if (rt ^ sizeof(uint32)) {
			switch (rt) {
				case 3: *t++ = s;  /* fallthrough */
				case 2: *t++ = s;  /* fallthrough */
				case 1: *t++ = s;  /* fallthrough */
				case 0:
					break;
			}
		}

		sxx = s;
		sxx |= sxx << 010;
		sxx |= sxx << 020;
		sxx |= sxx << 030;
		txx = (uint32*) (t);
		exx = (uint32*) (t + (m = (size >> 3) << 3));
		for (; exx > txx; txx += 2) {
			txx[0] = sxx;
			txx[1] = sxx;
		}
		t = (uint8*) txx;
#endif
		size -= m;
		if (size) {
#if defined(CTB_FASTUNALIGNED)
			t = (uint8*) txx;
#endif
			switch (size) {
				case 7: *t++ = s;  /* fallthrough */
				case 6: *t++ = s;  /* fallthrough */
				case 5: *t++ = s;  /* fallthrough */
				case 4: *t++ = s;  /* fallthrough */
				case 3: *t++ = s;  /* fallthrough */
				case 2: *t++ = s;  /* fallthrough */
				case 1: *t++ = s;  /* fallthrough */
				case 0:
					break;
			}
		}
	}
}

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif


static void
localmemzero(void* destination, uintxx size)
{
	ctb_memset(destination, 0, size);
}

void (*volatile ctb_memzerofn)(void*, uintxx) = localmemzero;
