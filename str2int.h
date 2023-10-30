/*
 * Copyright (C) 2023, jpn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.64
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

#include "ctoolbox.h"


 /* Error codes */
typedef enum {
	STR2INT_OK     = 0,
	STR2INT_ENAN   = 1,
	STR2INT_ERANGE = 2,
	STR2INT_EBASE  = 3
} eSTR2INTError;


/*
 * Converts an string (base 2 to base 16) to 32bit or 64bit integer. */
eintxx strtou32(const uint8* src, const uint8** end, intxx base, uint32* r);
eintxx strtou64(const uint8* src, const uint8** end, intxx base, uint64* r);

eintxx strtoi32(const uint8* src, const uint8** end, intxx base, int32* r);
eintxx strtoi64(const uint8* src, const uint8** end, intxx base, int64* r);

/*
 * Converts a decimal string (base 10) to 32bit or 64bit integer. */
eintxx dcmltou32(const uint8* src, intxx total, const uint8** end, uint32* r);
eintxx dcmltou64(const uint8* src, intxx total, const uint8** end, uint64* r);

eintxx dcmltoi32(const uint8* src, intxx total, const uint8** end, int32* r);
eintxx dcmltoi64(const uint8* src, intxx total, const uint8** end, int64* r);

/*
 * Converts a hexadecimal string (base 16) to integer. */
eintxx hexatou32(const uint8* src, intxx total, const uint8** end, uint32* r);
eintxx hexatou64(const uint8* src, intxx total, const uint8** end, uint64* r);

eintxx hexatoi32(const uint8* src, intxx total, const uint8** end, int32* r);
eintxx hexatoi64(const uint8* src, intxx total, const uint8** end, int64* r);


#if defined(CTB_ENV64)
	#define dcmltouxx dcmltou64
	#define dcmltoixx dcmltoi64
	#define heaxtouxx hexatou64
	#define heaxtoixx hexatoi64

	#define strtouxx strtou64
	#define strtoixx strtoi64
#else
	#define dcmltouxx dcmltou32
	#define dcmltoixx dcmltoi32
	#define heaxtouxx hexatou32
	#define heaxtoixx hexatoi32

	#define strtouxx strtou32
	#define strtoixx strtoi32
#endif


#endif
