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

#ifndef f2ec55ae_87d4_4a1c_bb78_808b27454f0c
#define f2ec55ae_87d4_4a1c_bb78_808b27454f0c

/*
 * memory.h
 * Memory management routines.
 */

#include <ctoolbox/ctoolbox.h>


/*
 * Allocator interface */

/* allocator function */
typedef void* (*TRequestFn)(uintxx size, void* user);

/* deallocator function */
typedef void  (*TDisposeFn)(void* memory, uintxx size, void* user);


/* ... */
struct TAllocator {
	/* */
	TRequestFn request;
	TDisposeFn dispose;

	/* user data */
	void* user;
};

typedef struct TAllocator TAllocator;


/*
 * Get the default allocator. */
const TAllocator* ctb_getdefaultallocator(void);

/*
 * Set the default allocator. If the allocator is NULL, the default
 * allocator will be used. */
void ctb_setdefaultallocator(TAllocator* allctr);


/*
 * Memory copy and set. */

/*
 * Same as C "memcpy". */
void ctb_memcpy(void* destination, const void* source, uintxx size);

/*
 * Same as C "memset". */
void ctb_memset(void* destination, uintxx value, uintxx size);


/*
 * A safe memset to zero */

extern void (*volatile ctb_memzerofn)(void*, uintxx);

CTB_INLINE void
ctb_memzero(void* destination, uintxx size)
{
	ctb_memzerofn(destination, size);
}


#endif
