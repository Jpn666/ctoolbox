/*
 * Copyright (C) 2025, jpn
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

#ifndef a76a09c0_0683_4275_84cd_e42df64c63e1
#define a76a09c0_0683_4275_84cd_e42df64c63e1

/*
 * xoshiro.h
 * Xoshiro prng, also includes Xoroshiro128.
 * See more at: https://prng.di.unimi.it
 */

#include "ctoolbox.h"


/*
 * Xoshiro128 */
struct TXoshiro128 {
	uint32 s[4];
};

typedef struct TXoshiro128 TXoshiro128;


/*
 * Sets the seed for the generator. */
CTOOLBOX_API
void xoshiro128_seed(TXoshiro128*, uint32 seed);

/*
 * Xoshiro-128 start-start and plus-plus functions. */
CTOOLBOX_API
uint32 xoshiro128_ssnext(TXoshiro128*);

CTOOLBOX_API
uint32 xoshiro128_ppnext(TXoshiro128*);

/*
 * This is the jump function for the generator. It is equivalent
 * to 2^64 calls to next(); it can be used to generate 2^64
 * non-overlapping subsequences for parallel computations. */
CTOOLBOX_API
void xoshiro128_ssjump(TXoshiro128*);

CTOOLBOX_API
void xoshiro128_ppjump(TXoshiro128*);


/*
 * Xoshiro256 */
struct TXoshiro256 {
	uint64 s[4];
};

typedef struct TXoshiro256 TXoshiro256;


/*
 * Sets the seed for the generator. */
CTOOLBOX_API
void xoshiro256_seed(TXoshiro256*, uint64 seed);

/*
 * Xoshiro-256 start-start and plus-plus functions. */
CTOOLBOX_API
uint64 xoshiro256_ssnext(TXoshiro256*);

CTOOLBOX_API
uint64 xoshiro256_ppnext(TXoshiro256*);

/*
 * This is the jump function for the generator. It is equivalent
 * to 2^128 calls to next(); it can be used to generate 2^128
 * non-overlapping subsequences for parallel computations. */
CTOOLBOX_API
void xoshiro256_ssjump(TXoshiro256*);

CTOOLBOX_API
void xoshiro256_ppjump(TXoshiro256*);


/*
 * Xoroshiro128 */
struct TXoroshiro128 {
	uint64 s[2];
};

typedef struct TXoroshiro128 TXoroshiro128;


/*
 * Sets the seed for the generator. */
CTOOLBOX_API
void xoroshiro128_seed(TXoroshiro128*, uint64 seed);

/*
 * Xoroshiro-128 start-start and plus-plus functions. */
CTOOLBOX_API
uint64 xoroshiro128_ssnext(TXoroshiro128*);

CTOOLBOX_API
uint64 xoroshiro128_ppnext(TXoroshiro128*);

/*
 * This is the jump function for the generator. It is equivalent
 * to 2^64 calls to next(); it can be used to generate 2^64 non-overlapping
 * subsequences for parallel computations. */
CTOOLBOX_API
void xoroshiro128_ssjump(TXoroshiro128*);

CTOOLBOX_API
void xoroshiro128_ppjump(TXoroshiro128*);


#endif
