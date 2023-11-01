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

#ifndef fceb37d0_f06e_4d01_a070_0f8c481d6196
#define fceb37d0_f06e_4d01_a070_0f8c481d6196

/*
 * types.h
 * Portable data types definitions.
 */

#if !defined(CTB_INTERNAL_INCLUDE_GUARD)
	#error "this file can't be included directly"
#endif

#include <stddef.h>
#include <limits.h>


#ifndef NULL
	#define NULL ((*void) 0)
#endif


/*
 * Boolean */

#if defined(CTB_CFG_NOSTDBOOL)
	typedef int ctb_bool;

	#if !defined(bool)
		#define bool ctb_bool
	#endif
#else
	#include <stdbool.h>
#endif


/*
 * Integers */

#if defined(CTB_CFG_NOSTDINT)
	#define ADD_TYPEDEF(A, B) typedef A B
#else
	#include <stdint.h>

	#define ADD_TYPEDEF(A, B) typedef B##_t B
#endif


ADD_TYPEDEF(  signed int,  int32);
ADD_TYPEDEF(unsigned int, uint32);
ADD_TYPEDEF(  signed short int,  int16);
ADD_TYPEDEF(unsigned short int, uint16);
ADD_TYPEDEF(  signed char,  int8);
ADD_TYPEDEF(unsigned char, uint8);


#if defined(_MSC_VER)
	typedef   signed __int64  int64;
	typedef unsigned __int64 uint64;
#else
	ADD_TYPEDEF(  signed long long int,  int64);
	ADD_TYPEDEF(unsigned long long int, uint64);
#endif

#undef ADD_TYPEDEF


/* produce compile errors if the sizes aren't right */
typedef union
{
	char i1_incorrect[-1 + (sizeof(  int8) == 1) * 2];
	char u1_incorrect[-1 + (sizeof( uint8) == 1) * 2];
	char i2_incorrect[-1 + (sizeof( int16) == 2) * 2];
	char u2_incorrect[-1 + (sizeof(uint16) == 2) * 2];
	char i4_incorrect[-1 + (sizeof( int32) == 4) * 2];
	char u4_incorrect[-1 + (sizeof(uint32) == 4) * 2];
	char i8_incorrect[-1 + (sizeof( int64) == 8) * 2];
	char u8_incorrect[-1 + (sizeof(uint64) == 8) * 2];
} TTypeStaticAssert;


/* fast (target platform word size) integer */
#if defined(CTB_ENV64)
	typedef  int64  intxx;
	typedef uint64 uintxx;
#else
	typedef  int32  intxx;
	typedef uint32 uintxx;
#endif


/*
 * Extra types */

/* error code */
typedef intxx eintxx;

/* added to ensure portability */
typedef  float float32;
typedef double float64;

typedef float32 flt32;
typedef float64 flt64;


/*
 * Funtion types */

/* used for comparison */
typedef intxx (*TCmpFn)(const void*, const void*);

/* to get a hash value */
typedef uintxx (*THashFn)(void*);

/* to check for equality */
typedef bool (*TEqualFn)(const void*, const void*);

/* */
typedef void (*TFreeFn)(void*);

/* */
typedef void (*TUnaryFn)(void*);


/*
 * Custom allocator */

/* allocator function */
typedef void* (*TReserveFn)(void* user, uintxx amount);

/* ... */
typedef void* (*TReallocFn)(void* user, void* memory, uintxx amount);

/* deallocator function */
typedef void (*TReleaseFn)(void* user, void* memory);


/* ... */
struct TAllocator {
	/* */
	TReserveFn reserve;
	TReallocFn realloc;
	TReleaseFn release;

	/* user data */
	void* user;
};

typedef struct TAllocator TAllocator;


/* signed limits */
#ifndef INT8_MIN
	#define  INT8_MIN 0x00000080L
#endif
#ifndef INT8_MAX
	#define  INT8_MAX 0x0000007FL
#endif

#ifndef INT16_MIN
	#define INT16_MIN 0x00008000L
#endif
#ifndef INT16_MAX
	#define INT16_MAX 0x00007FFFL
#endif

#ifndef INT32_MIN
	#define INT32_MIN 0x80000000L
#endif
#ifndef INT32_MAX
	#define INT32_MAX 0x7FFFFFFFL
#endif

#ifndef INT64_MIN
	#define INT64_MIN 0x8000000000000000LL
#endif
#ifndef INT64_MAX
	#define INT64_MAX 0x7FFFFFFFFFFFFFFFLL
#endif


/* unsigned limits */
#ifndef UINT8_MAX
	#define  UINT8_MAX 0x000000FFUL
#endif
#ifndef UINT32_MAX
	#define UINT32_MAX 0xFFFFFFFFUL
#endif
#ifndef UINT16_MAX
	#define UINT16_MAX 0x0000FFFFUL
#endif
#ifndef UINT64_MAX
	#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
#endif


#if defined(CTB_ENV64)
	/* 64 bits */
	#define INTXX_MIN INT64_MIN
	#define INTXX_MAX INT64_MAX

	#define UINTXX_MAX UINT64_MAX
#else
	/* 32 bits */
	#define INTXX_MIN INT32_MIN
	#define INTXX_MAX INT32_MAX

	#define UINTXX_MAX UINT32_MAX
#endif


#endif
