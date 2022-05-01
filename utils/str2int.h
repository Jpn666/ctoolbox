/*
 * Copyright (C) 2022, jpn
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

#ifndef f752343e_e974_48ea_95e7_288826a30056
#define f752343e_e974_48ea_95e7_288826a30056

/*
 * str2int.h
 * String to integer conversion.
 */

#include "../ctoolbox.h"


#if !CTB_HAVEINT64
#	error "Your compiler does not seems to support 64 bit integers"
#endif


/* Error codes */
typedef enum {
	STR2INT_OK        = 0,
	STR2INT_ENAN      = 1,
	STR2INT_EBADBASE  = 2,
	STR2INT_EOVERFLOW = 3
} eSTR2INTError;


#if defined(CTB_ENV64)

#define str2uxx str2u64
#define str2ixx str2i64

#else

#define str2uxx str2u32
#define str2ixx str2i32

#endif


/*
 * */
uintxx str2u32(const char* src, const char** end, uint32* result, uintxx base);
uintxx str2i32(const char* src, const char** end,  int32* result, uintxx base);

/*
 * */
uintxx str2u64(const char* src, const char** end, uint64* result, uintxx base);
uintxx str2i64(const char* src, const char** end,  int64* result, uintxx base);

#endif
