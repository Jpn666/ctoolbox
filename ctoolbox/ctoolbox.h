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

#ifndef c71a75a9_d526_4e90_a75a_d8189a1c3a3e
#define c71a75a9_d526_4e90_a75a_d8189a1c3a3e

/*
 * ctoolbox.h
 * ...
 */

#include <ctoolboxconfig.h>

#if !defined(CTB_CFG_NOSTDLIB)
	#include <assert.h>
#endif


#if !defined(__MSVC__) && defined(_MSC_VER)
	#define __MSVC__
#endif
#if !defined(__has_builtin)
	#define __has_builtin(x) 0
#endif


/*
 * Inline and force inline */

#if defined(__MSVC__)
	#define CTB_INLINE static __inline
#endif

#if !defined(CTB_INLINE)
	#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
		#define CTB_INLINE static inline
	#else
		/* ansi c */
		#if defined(__GNUC__)
			#define CTB_INLINE static __inline__
		#else
			#define CTB_INLINE static
		#endif
	#endif
#endif

#if defined(__MSVC__)
	#define CTB_FORCEINLINE static __forceinline
#endif

#if !defined(CTB_FORCEINLINE)
	#if defined(__GNUC__)
		#define CTB_FORCEINLINE __attribute__((always_inline)) CTB_INLINE
	#else
		#define CTB_FORCEINLINE CTB_INLINE
	#endif
#endif


#if defined(__GNUC__) || __has_builtin(__builtin_expect)
	#define CTB_EXPECT1(X) __builtin_expect(!!(X), 1)
	#define CTB_EXPECT0(X) __builtin_expect(!!(X), 0)
#else
	#define CTB_EXPECT1(X) (X)
	#define CTB_EXPECT0(X) (X)
#endif


/*
 * Internal Includes */

#define CTB_INTERNAL_INCLUDE_GUARD
	/* order matters */
	#include "private/platform.h"
	#include "private/types.h"
	#include "private/endianness.h"
#undef CTB_INTERNAL_INCLUDE_GUARD


/*
 * Assert */

#if defined(_DEBUG)
	#undef NDEBUG
#endif


#define CTB_ASSERT(C) { if (!(C)) { ctb_testfailed(#C, __FILE__, __LINE__); } }

#if defined(NDEBUG)
	#undef  CTB_ASSERT
	#define CTB_ASSERT(C) (void) (C)
#else
	#if !defined(CTB_CFG_NOSTDLIB)
		#undef  CTB_ASSERT
		#define CTB_ASSERT(A) assert(A)
	#endif
#endif


/* */
struct TAssertInfo {
	const char* filename;
	const char* cndtn;
	int line;
};

typedef struct TAssertInfo TAssertInfo;


/*
 * Set the assert function to be called when an assertion fails. If no
 * function is set, the default behavior is to loop infinitely. */
void ctb_setassertfn(void (*)(TAssertInfo));


extern void (*ctb_assertfn)(TAssertInfo);

CTB_INLINE void
ctb_testfailed(const char* cndtn, const char* filename, int line)
{
	struct TAssertInfo assertinfo;

	if (ctb_assertfn) {
		assertinfo = (struct TAssertInfo){
			.cndtn = cndtn, .filename = filename, .line = line
		};
		ctb_assertfn(assertinfo);
	}

	/*
	 * If no assert function is set, we just loop infinitely, this is useful
	 * for embedded systems where we can't print to a console or where we
	 * don't want to halt the system. In this case, the user should set a
	 * custom assert function to handle the assertion failure appropriately.
	 * */
	do {
		/* Not my problem... */
	} while(1);
}


#if defined(__GNUC__)

/* 
 * We use this to avoid compiler warnings when casting away constness, might
 * not be elegant but works. */

#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcast-qual"
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-qual"
	#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif

static inline __attribute__((always_inline)) void*
CTB_CONSTCAST(const void* a)
{
	return (void*) a;
}

#if defined(__clang__)
	#pragma clang diagnostic pop
#else
	#pragma GCC diagnostic pop
#endif

#else

#define CTB_CONSTCAST(a) ((void*) ((const void*) (a)))

#endif


#endif
