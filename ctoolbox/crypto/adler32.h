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

#ifndef b32d1608_a8a0_4078_82a3_9f192e84bbab
#define b32d1608_a8a0_4078_82a3_9f192e84bbab

/*
 * adler32.h
 * Adler32 hash.
 */

#include "../ctoolbox.h"


#define ADLER32_INIT(A) ((A) = 1ul)


/*
 * */
CTB_INLINE uint32 adler32_update(uint32 adler, const uint8* data, uintxx size);

/*
 * */
uint32 adler32_slideby8(uint32 adler, const uint8* data, uintxx size);


/*
 * Inlines */

#if defined(ADLER32_CFG_EXTERNALASM)

uint32 adler32_updateASM(uint32, const uint8*, uintxx);

CTB_INLINE uint32
adler32_update(uint32 adler, const uint8* data, uintxx size)
{
	CTB_ASSERT(data);
	return adler32_updateASM(adler, data, size);
}

#else

CTB_INLINE uint32
adler32_update(uint32 adler, const uint8* data, uintxx size)
{
	CTB_ASSERT(data);
	return adler32_slideby8(adler, data, size);
}

#endif

#endif

