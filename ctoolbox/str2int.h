/*
 * Copyright (C) 2025, jpn
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


/* */
struct TToIntResult {
	eintxx error;

	union TIntValue {
		int32  asi32;
		uint32 asu32;
		int64  asi64;
		uint64 asu64;
	} value;
};

typedef struct TToIntResult TToIntResult;


/*
 * Converts an string (base 2 to base 16) to 32bit or 64bit integer. */
CTOOLBOX_API
TToIntResult strtou32(const uint8* src, const uint8** end, intxx base);

CTOOLBOX_API
TToIntResult strtou64(const uint8* src, const uint8** end, intxx base);

CTOOLBOX_API
TToIntResult strtoi32(const uint8* src, const uint8** end, intxx base);

CTOOLBOX_API
TToIntResult strtoi64(const uint8* src, const uint8** end, intxx base);


/*
 * Converts a decimal string (base 10) to 32bit or 64bit integer. */
CTOOLBOX_API
TToIntResult dcmltou32(const uint8* src, intxx total, const uint8** end);

CTOOLBOX_API
TToIntResult dcmltou64(const uint8* src, intxx total, const uint8** end);

CTOOLBOX_API
TToIntResult dcmltoi32(const uint8* src, intxx total, const uint8** end);

CTOOLBOX_API
TToIntResult dcmltoi64(const uint8* src, intxx total, const uint8** end);


/*
 * Converts a hexadecimal string (base 16) to integer. */
CTOOLBOX_API
TToIntResult hexatou32(const uint8* src, intxx total, const uint8** end);

CTOOLBOX_API
TToIntResult hexatou64(const uint8* src, intxx total, const uint8** end);

CTOOLBOX_API
TToIntResult hexatoi32(const uint8* src, intxx total, const uint8** end);

CTOOLBOX_API
TToIntResult hexatoi64(const uint8* src, intxx total, const uint8** end);


#endif
