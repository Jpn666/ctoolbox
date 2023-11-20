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

#ifndef c71a75a9_d526_4e90_a75a_d8189a1c3a3e
#define c71a75a9_d526_4e90_a75a_d8189a1c3a3e

/*
 * ctoolbox.h
 * ...
 */

#include <ctoolboxconfig.h>


#if !defined(__has_builtin)
	#define __has_builtin(x) 0
#endif

#if defined(NDEBUG)
	#define CTB_ASSERT(x) ((void) 0)
#else
	#if defined(CTB_CFG_NOSTDLIB)
		#define CTB_ASSERT(x) ((void) 0)
	#else
		#include <assert.h>
		#define CTB_ASSERT(x) assert(x)
	#endif

	#define CTB_DEBUG
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


#if defined(_MSC_VER)
	#define CTB_FORCEINLINE static __forceinline
#endif

#if !defined(CTB_FORCEINLINE)
	#if defined(__GNUC__)
		#define CTB_FORCEINLINE __attribute__((always_inline)) CTB_INLINE
	#else
		#define CTB_FORCEINLINE CTB_INLINE
	#endif
#endif


#if defined(__GNUC__)
	#define   CTB_LIKELY(X) __builtin_expect((X), 1)
	#define CTB_UNLIKELY(X) __builtin_expect((X), 0)
#else
	#if __has_builtin(__builtin_expect)
		#define   CTB_LIKELY(X) __builtin_expect((X), 1)
		#define CTB_UNLIKELY(X) __builtin_expect((X), 0)
	#endif
#endif

#if !defined(CTB_LIKELY)
	#define   CTB_LIKELY(X) (X)
	#define CTB_UNLIKELY(X) (X)
#endif

#define LIKELY   CTB_LIKELY
#define UNLIKELY CTB_UNLIKELY

/*
 * Internal Includes */

#define CTB_INTERNAL_INCLUDE_GUARD
	/* order matters */
	#include "private/platform.h"
	#include "private/types.h"
	#include "private/endianness.h"
#undef CTB_INTERNAL_INCLUDE_GUARD


/* Error values */
typedef enum {
	CTB_OK     =  0,
	CTB_EPARAM = -1,
	CTB_ERANGE = -2,
	CTB_EOOM   = -3,
	CTB_ENKEY  = -4
} eCTBError;


#endif
