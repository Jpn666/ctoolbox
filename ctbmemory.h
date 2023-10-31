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

#ifndef f2ec55ae_87d4_4a1c_bb78_808b27454f0c
#define f2ec55ae_87d4_4a1c_bb78_808b27454f0c

/*
 * ctbmemory.h
 * Memory management routines.
 */

#include "ctoolbox.h"

/*
 * Memory allocation. */

#if !defined(CTB_RESERVE)
	#define CTB_RESERVE ctb_reserve
#endif
#if !defined(CTB_RELEASE)
	#define CTB_RELEASE ctb_release
#endif
#if !defined(CTB_REALLOC)
	#define CTB_REALLOC ctb_realloc
#endif


/*
 * Same as C "malloc". */
CTB_INLINE void* ctb_reserve(uintxx amount);

/*
 * Same as C "free". */
CTB_INLINE void ctb_release(void* memory);

/*
 * Same as C "realloc". */
CTB_INLINE void* ctb_realloc(void* memory, uintxx amount);


/*
 * Memory copy and set. */

/* Safe call to memset(destination, 0, size). */
extern void (*volatile ctb_memzero)(void*, uintxx);


/*
 * Same as C "memcpy". */
void ctb_memcpy(void* destination, void* source, uintxx size);

/*
 * Same as C "memset". */
void ctb_memset(void* destination, uintxx value, uintxx size);


/*
 * Inlines */

#if defined(CTB_CFG_NOSTDLIB)

CTB_INLINE void*
ctb_reserve(uintxx amount)
{
	(void) amount;
	return NULL;
}

CTB_INLINE void
ctb_release(void* memory)
{
	(void) memory;
}

CTB_INLINE void*
ctb_realloc(void* memory, uintxx amount)
{
	(void) memory, (void) amount;
	return NULL;
}

#else

#include <stdlib.h>


CTB_INLINE void*
ctb_reserve(uintxx amount)
{
	return malloc(amount);
}

CTB_INLINE void
ctb_release(void* memory)
{
	free(memory);
}

CTB_INLINE void*
ctb_realloc(void* memory, uintxx amount)
{
	return realloc(memory, amount);
}

#endif
#endif
