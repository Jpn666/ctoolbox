 /*
 * Copyright (C) 2014, jpn 
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
 * config.h
 * Configuration flags.
 */

/* ***************************************************************************
 * Processor specific
 *************************************************************************** */

/* Define one of them if endiannes auto detection fails. */
/* #undef CTB_CFG_LITTLEENDIAN */
/* #undef CTB_CFG_BIGENDIAN */


/* Define it if your target is a 64bit arch. */
/* #undef CTB_CFG_ENV64 */


/* ... */
/* #undef CTB_CFG_STRICTALIGNMENT */


/* Define it if unaligned memory access is efficient. */
/* #undef CTB_CFG_FASTUNALIGNED */


/* ***************************************************************************
 * Platform
 *************************************************************************** */

/* Define it if your compiler don't have support for 64 bits integers. */
/* #undef CTB_CFG_NOINT64 */


/* Compiler features. */
/* #undef CTB_CFG_NOINTTYPES */
/* #undef CTB_CFG_NOSTDBOOL */


/* In this way we don't have to deal with compiler definitions. */
/* #undef CTB_CFG_PLATFORM_UNIX */
/* #undef CTB_CFG_PLATFORM_WINDOWS */
/* #undef CTB_CFG_PLATFORM_BEOS */
/* #undef CTB_CFG_PLATFORM_DOS */
/* #undef CTB_CFG_PLATFORM_OS2 */



#endif
