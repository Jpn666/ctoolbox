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

#ifndef becde648_275d_4d69_ba9a_79b96b78e2c9
#define becde648_275d_4d69_ba9a_79b96b78e2c9

/*
 * int2str.h
 * Integer to string conversion.
 */

#include "ctoolbox.h"


/*
 * 32 bit integer to decimal string. */
uintxx u32tostr(uint32 number, uint8 r[16]);
uintxx i32tostr( int32 number, uint8 r[16]);

/*
 * 64 bit integer to decimal string. */
uintxx u64tostr(uint64 number, uint8 r[24]);
uintxx i64tostr( int64 number, uint8 r[24]);

/*
 * Unsigned integer type to hexadecimal string. */
uintxx u32tohexa(uint32 number, intxx uppercase, uint8 r[16]);
uintxx u64tohexa(uint64 number, intxx uppercase, uint8 r[24]);


#if defined(CTB_ENV64)
	#define uxxtohexa u64tohexa

	#define uxxtostr u64tostr
	#define ixxtostr i64tostr
#else
	#define uxxtohexa u32tohexa

	#define uxxtostr u32tostr
	#define ixxtostr i32tostr
#endif


#endif
