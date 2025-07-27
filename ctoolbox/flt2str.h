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

#ifndef ccb4a9e2_6f3e_4dbb_847b_a53f3a92007b
#define ccb4a9e2_6f3e_4dbb_847b_a53f3a92007b

/*
 * flt2str.h
 * Float IEEE754 to decimal string conversion.
 */

#include "ctoolbox.h"


/* Float format mode */
typedef enum {
	FLTF_MODEG = 0,  /* same as libc "%.<precision>g" */
	FLTF_MODEE = 1,  /* same as libc "%.<precision>e" */
	FLTF_MODED = 2   /* like "%.16e" using all the significant digits */
} eFLTFormatMode;


/*
 * Convert a float to a string. The precision is the number of digits after
 * the decimal point. */
uintxx f64tostr(flt64 number, eFLTFormatMode m, uintxx precision, uint8 r[24]);

/*
 * */
uintxx f32tostr(flt32 number, eFLTFormatMode m, uintxx precision, uint8 r[24]);


#endif
