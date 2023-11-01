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

#ifndef ada5cc64_bd20_4cca_8557_922147b04ad7
#define ada5cc64_bd20_4cca_8557_922147b04ad7

/*
 * crc32.h
 * Crc32 implementation.
 */

#include "../ctoolbox.h"


#define CRC32_POLYNOMIAL 0x04C11DB7UL

#define CRC32_INIT(A)     ((A) =       0xfffffffful)
#define CRC32_FINALIZE(A) ((A) = (A) ^ 0xfffffffful)


/*
 * Combines 2 crcs. */
uint32 crc32_ncombine(uint32 crc1, uint32 crc2, uint32 size2);

/*
 * ... */
uint32 crc32_sliceby4(uint32 crc, const uint8* data, uintxx size);

/*
 * ... */
uint32 crc32_sliceby8(uint32 crc, const uint8* data, uintxx size);

/*
 * Updates the crc. */
CTB_INLINE uint32 crc32_update(uint32 crc, const uint8* data, uintxx size);

/*
 * Gets the crc of a memory block. */
CTB_INLINE uint32 crc32_getcrc(const uint8* data, uintxx size);

/*
 * ... */
void crc32_createtable(uint32 table[8][256]);


/*
 * Inlines */

CTB_INLINE uint32
crc32_getcrc(const uint8* data, uintxx size)
{
	CTB_ASSERT(data);
	return crc32_update(0xfffffffful, data, size) ^ 0xfffffffful;
}

#if defined(CRC32_CFG_EXTERNALASM)

uint32 crc32_updateASM(uint32, const uint8*, uintxx);

CTB_INLINE uint32
crc32_update(uint32 crc, const uint8* data, uintxx size)
{
	CTB_ASSERT(data);
	return crc32_updateASM(crc, data, size);
}

#else

CTB_INLINE uint32
crc32_update(uint32 crc, const uint8* data, uintxx size)
{
	CTB_ASSERT(data);
#if defined(CTB_ENV64)
	return crc32_sliceby8(crc, data, size);
#else
	return crc32_sliceby4(crc, data, size);
#endif
}

#endif

#endif

