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

#ifndef fee95e12_79bc_407a_a207_787da31265e4
#define fee95e12_79bc_407a_a207_787da31265e4

/*
 * endianness.h
 * Endianness stuff.
 */

#if !defined(CTB_INTERNAL_INCLUDE_GUARD)
	#error "this file can't be included directly"
#endif

#include "platform.h"
#include "types.h"


#define CTB_LITTLEENDIAN 1234
#define CTB_BIGENDIAN    4321


 /* configuration flags */
#if defined(CTB_CFG_LITTLEENDIAN) && defined(CTB_CFG_BIGENDIAN)
	#error "pick just one endianess"
#endif

#if defined(CTB_CFG_LITTLEENDIAN)
	#define CTB_BYTEORDER CTB_LITTLEENDIAN
#endif

#if defined(CTB_CFG_BIGENDIAN)
	#define CTB_BYTEORDER CTB_BIGENDIAN
#endif


#if !defined(CTB_BYTEORDER)
	#error "can't determine the correct endiannes"
#endif

/*
 * Swap macros/functions */

/* little endian */
#if CTB_BYTEORDER == CTB_LITTLEENDIAN
	#define CTB_IS_LITTLEENDIAN 1
	#define CTB_IS_BIGENDIAN    0

	#define CTB_SWAP64ONLE(x) (ctb_swap64(x))
	#define CTB_SWAP64ONBE(x) (x)

	#define CTB_SWAP32ONLE(x) (ctb_swap32(x))
	#define CTB_SWAP16ONLE(x) (ctb_swap16(x))
	#define CTB_SWAP32ONBE(x) (x)
	#define CTB_SWAP16ONBE(x) (x)
#endif

/* big endian */
#if CTB_BYTEORDER == CTB_BIGENDIAN
	#define CTB_IS_LITTLEENDIAN 0
	#define CTB_IS_BIGENDIAN    1

	#define CTB_SWAP64ONBE(x) (ctb_swap64(x))
	#define CTB_SWAP64ONLE(x) (x)

	#define CTB_SWAP32ONBE(x) (ctb_swap32(x))
	#define CTB_SWAP16ONBE(x) (ctb_swap16(x))
	#define CTB_SWAP32ONLE(x) (x)
	#define CTB_SWAP16ONLE(x) (x)
#endif


#if defined(__clang__)
	#if __has_builtin(__builtin_bswap16)
		#define ctb_swap16(n) (__builtin_bswap16((n)))
		#define ctb_swap32(n) (__builtin_bswap32((n)))
		#define ctb_swap64(n) (__builtin_bswap64((n)))
	#endif
#else
	#if defined(__GNUC__)
		#define ctb_swap16(n) (__builtin_bswap16((n)))
		#define ctb_swap32(n) (__builtin_bswap32((n)))
		#define ctb_swap64(n) (__builtin_bswap64((n)))
	#endif
#endif


/*
 * Inlines */

#if !defined(ctb_swap16)

CTB_INLINE uint16
ctb_swap16(uint16 n)
{
	return ((n & 0x00FF) << 010) | ((n & 0xFF00) >> 010);
}

CTB_INLINE uint32
ctb_swap32(uint32 n)
{
	return ((n & 0x000000FFul) << 030) |
	       ((n & 0x0000FF00ul) << 010) |
	       ((n & 0x00FF0000ul) >> 010) |
	       ((n & 0xFF000000ul) >> 030);
}

CTB_INLINE uint64
ctb_swap64(uint64 n)
{
	return ((n & 0x00000000000000FFull) << 070) |
	       ((n & 0x000000000000FF00ull) << 050) |
	       ((n & 0x0000000000FF0000ull) << 030) |
	       ((n & 0x00000000FF000000ull) << 010) |
	       ((n & 0x000000FF00000000ull) >> 010) |
	       ((n & 0x0000FF0000000000ull) >> 030) |
	       ((n & 0x00FF000000000000ull) >> 050) |
	       ((n & 0xFF00000000000000ull) >> 070);
}

#endif

#endif
