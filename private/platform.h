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

#ifndef c35bb22c_eedb_4b27_afbe_61bc41fe51c6
#define c35bb22c_eedb_4b27_afbe_61bc41fe51c6

/*
 * platform.h
 * Platform related macros/stuff.
 */

#if !defined(CTB_INTERNAL_INCLUDE_GUARD)
	#error "this file can't be included directly"
#endif


#define CTB_PLATFORM_UNIX     0x01
#define CTB_PLATFORM_BEOS     0x02
#define CTB_PLATFORM_WINDOWS  0x03
#define CTB_PLATFORM_UNKNOWN  0x00


/* */
#if !defined(CTB_PLATFORM) && defined(CTB_CFG_PLATFORM_UNIX)
	#define CTB_PLATFORM CTB_PLATFORM_UNIX
#endif

#if !defined(CTB_PLATFORM) && defined(CTB_CFG_PLATFORM_BEOS)
	#define CTB_PLATFORM CTB_PLATFORM_BEOS
#endif

#if !defined(CTB_PLATFORM) && defined(CTB_CFG_PLATFORM_WINDOWS)
	#define CTB_PLATFORM CTB_PLATFORM_WINDOWS
#endif

#if !defined(CTB_PLATFORM)
	#define CTB_PLATFORM CTB_PLATFORM_UNKNOWN
#endif


/*
 * Processor word size */

#if defined(CTB_CFG_ENV64)
	#define CTB_WORDSIZE 64

	#define CTB_ENV64
#else
	#define CTB_WORDSIZE 32
#endif


#if defined(CTB_CFG_STRICTALIGNMENT)
	#define CTB_STRICTALIGNMENT
#endif


#if defined(CTB_CFG_FASTUNALIGNED)
	#define CTB_FASTUNALIGNED
#endif

#endif

