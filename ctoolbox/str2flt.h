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

#ifndef a4fb3255_6736_4d17_85a2_1959d8f88ecd
#define a4fb3255_6736_4d17_85a2_1959d8f88ecd

/*
 * str2flt.h
 * String (decimal only) to float IEEE754 conversion.
 */

#include "ctoolbox.h"


/* Error codes */
typedef enum {
	STR2FLT_OK     = 0,
	STR2FLT_ENAN   = 1,
	STR2FLT_ERANGE = 2
} eSTR2FLTError;


/*
 * */
eintxx str2flt64(const uint8* src, const uint8** end, flt64* r);

/*
 * */
eintxx str2flt32(const uint8* src, const uint8** end, flt32* r);


#endif
