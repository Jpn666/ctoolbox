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

#ifndef f6fc5398_0274_42a2_9186_20cc03b729d0
#define f6fc5398_0274_42a2_9186_20cc03b729d0

/*
 * ctbconfig.h
 * Configuration flags.
 */

/* ***************************************************************************
 * Processor specific
 *************************************************************************** */

/* Define one of them if endiannes auto detection fails. */
#define CTB_CFG_LITTLEENDIAN
/*#define CTB_CFG_BIGENDIAN*/


/* Define it if your target is a 64bit arch. */
/* #define CTB_CFG_ENV64 */


/* ... */
/*#define CTB_CFG_STRICTALIGNMENT*/


/* Define it if unaligned memory access is efficient. */
#define CTB_CFG_FASTUNALIGNED


/* ***************************************************************************
 * Platform
 *************************************************************************** */

/* Compiler features. */
/* #undef CTB_CFG_NOSTDINT */
/* #undef CTB_CFG_NOSTDBOOL */

/* c23 checked int */
/* #undef CTB_CFG_HAS_STDCKDINT */
#define CTB_CFG_HAS_CKDINT_INTRINSICS

/* */
/* #undef CTB_CFG_NOSTDLIB */


/* In this way we don't have to deal with compiler definitions. */
#define CTB_CFG_PLATFORM_UNIX
/* #define CTB_CFG_PLATFORM_WINDOWS */
/* #undef CTB_CFG_PLATFORM_BEOS */


/* */
#define CTB_VERSION_MAJOR 0
#define CTB_VERSION_MINOR 0

/* revision */
#define CTB_VERSION_RPATH 0

#define CTB_VERSION_STRING "0.0.0"


#endif
