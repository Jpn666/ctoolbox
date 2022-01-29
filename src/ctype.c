/*
 * Copyright (C) 2017, jpn 
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

#include "../ctype.h"


/*
  0 -   8 cntrl
  9 -  13 cntrl-space
 14 -  31 cntrl
 32 -  32 space-print
 33 -  47 punct-print
 48 -  57 digit-print
 58 -  64 punct-print
 65 -  90 upper-print
 91 -  96 punct-print
 97 - 102 lower-print
123 - 126 punct-print
127 - 127 cntrl
*/

const uint8 ctb_ctypeproperties[256] = 
{
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  /*   7 */
	0x10, 0x14, 0x14, 0x14, 0x14, 0x14, 0x10, 0x10,  /*   8 */
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  /*  16 */
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  /*  24 */
	0x0c, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,  /*  32 */
	0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,  /*  40 */
	0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,  /*  48 */
	0x48, 0x48, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,  /*  56 */
	0x28, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,  /*  64 */
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,  /*  72 */
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,  /*  80 */
	0x09, 0x09, 0x09, 0x28, 0x28, 0x28, 0x28, 0x28,  /*  88 */
	0x28, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,  /*  96 */
	0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,  /* 104 */
	0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,  /* 112 */
	0x0a, 0x0a, 0x0a, 0x28, 0x28, 0x28, 0x28, 0x10,  /* 120 */
	
	/* 128 - 255 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};