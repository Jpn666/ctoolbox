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

#include <ctoolbox/xoshiro.h>


CTB_INLINE uint64
splitmix64(uint64* x)
{
	uint64 z;

	x[0] += 0x9e3779b97f4a7c15ull;
	z = x[0];

	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
	return (z >> 31) ^ z;
}

CTB_INLINE uint32
splitmix32(uint32* x)
{
	uint64 z;

	x[0] += 2654435769u;
	z = x[0];

	z = (z << 32) ^ (z * 0xaf723597u);
	return (uint32) splitmix64(&z);
}


#define ROTL32(X, K) (((X) << (K)) | ((X) >> (32 - K)))
#define ROTL64(X, K) (((X) << (K)) | ((X) >> (64 - K)))


void
xoshiro128_seed(TXoshiro128* state, uint32 seed)
{
	CTB_ASSERT(state);

	state->s[0] = splitmix32(&seed);
	state->s[1] = splitmix32(&seed);
	state->s[2] = splitmix32(&seed);
	state->s[3] = splitmix32(&seed);
}

void
xoshiro128_ssjump(TXoshiro128* state) {
	uint32 s0;
	uint32 s1;
	uint32 s2;
	uint32 s3;
	uintxx i;
	static const uint32 jump[] = {
		0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b
	};
	CTB_ASSERT(state);

	s0 = 0;
	s1 = 0;
	s2 = 0;
	s3 = 0;
	for (i = 0; i < 4; i++) {
		uintxx j;

		for (j = 0; j < 32; j++) {
			if (jump[i] & ((uint32) 1 << j)) {
				s0 ^= state->s[0];
				s1 ^= state->s[1];
				s2 ^= state->s[2];
				s3 ^= state->s[3];
			}
			xoshiro128_ssnext(state);
		}
	}
	state->s[0] = s0;
	state->s[1] = s1;
	state->s[2] = s2;
	state->s[3] = s3;
}

uint32
xoshiro128_ssnext(TXoshiro128* state)
{
	uint32 r;
	uint32 t;
	CTB_ASSERT(state);

	r = ROTL32(state->s[1] * 5, 7) * 9;
	t = state->s[1] << 9;

	state->s[2] ^= state->s[0];
	state->s[3] ^= state->s[1];
	state->s[1] ^= state->s[2];
	state->s[0] ^= state->s[3];
	state->s[2] ^= t;

	state->s[3] = ROTL32(state->s[3], 11);
	return r;
}

void
xoshiro128_ppjump(TXoshiro128* state) {
	uint32 s0;
	uint32 s1;
	uint32 s2;
	uint32 s3;
	uintxx i;
	static const uint32 jump[] = {
		0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b
	};
	CTB_ASSERT(state);

	s0 = 0;
	s1 = 0;
	s2 = 0;
	s3 = 0;
	for (i = 0; i < 4; i++) {
		uintxx j;

		for (j = 0; j < 32; j++) {
			if (jump[i] & ((uint32) 1 << j)) {
				s0 ^= state->s[0];
				s1 ^= state->s[1];
				s2 ^= state->s[2];
				s3 ^= state->s[3];
			}
			xoshiro128_ppnext(state);
		}
	}
	state->s[0] = s0;
	state->s[1] = s1;
	state->s[2] = s2;
	state->s[3] = s3;
}

uint32
xoshiro128_ppnext(TXoshiro128* state)
{
	uint32 r;
	uint32 t;
	CTB_ASSERT(state);

	r = ROTL32(state->s[0] + state->s[3], 7) + state->s[0];
	t = state->s[1] << 9;

	state->s[2] ^= state->s[0];
	state->s[3] ^= state->s[1];
	state->s[1] ^= state->s[2];
	state->s[0] ^= state->s[3];
	state->s[2] ^= t;

	state->s[3] = ROTL32(state->s[3], 11);
	return r;
}


void
xoshiro256_seed(TXoshiro256* state, uint64 seed)
{
	CTB_ASSERT(state);

	state->s[0] = splitmix64(&seed);
	state->s[1] = splitmix64(&seed);
	state->s[2] = splitmix64(&seed);
	state->s[3] = splitmix64(&seed);
}

void
xoshiro256_ssjump(TXoshiro256* state) {
	uint64 s0;
	uint64 s1;
	uint64 s2;
	uint64 s3;
	uintxx i;
	static const uint64 jump[] = {
		0x180ec6d33cfd0aba, 0xd5a61266f0c9392c,
		0xa9582618e03fc9aa, 0x39abdc4529b1661c
	};
	CTB_ASSERT(state);

	s0 = 0;
	s1 = 0;
	s2 = 0;
	s3 = 0;
	for(i = 0; i < 4; i++) {
		uintxx j;

		for(j = 0; j < 64; j++) {
			if (jump[i] & ((uint64) 1 << j)) {
				s0 ^= state->s[0];
				s1 ^= state->s[1];
				s2 ^= state->s[2];
				s3 ^= state->s[3];
			}
			xoshiro256_ssnext(state);
		}
	}
	state->s[0] = s0;
	state->s[1] = s1;
	state->s[2] = s2;
	state->s[3] = s3;
}

uint64
xoshiro256_ssnext(TXoshiro256* state)
{
	uint64 r;
	uint64 t;
	CTB_ASSERT(state);

	r = ROTL64(state->s[1] * 5, 7) * 9;
	t = state->s[1] << 17;

	state->s[2] ^= state->s[0];
	state->s[3] ^= state->s[1];
	state->s[1] ^= state->s[2];
	state->s[0] ^= state->s[3];
	state->s[2] ^= t;

	state->s[3] = ROTL64(state->s[3], 45);
	return r;
}

void
xoshiro256_ppjump(TXoshiro256* state) {
	uint64 s0;
	uint64 s1;
	uint64 s2;
	uint64 s3;
	uintxx i;
	static const uint64 jump[] = {
		0x180ec6d33cfd0aba, 0xd5a61266f0c9392c,
		0xa9582618e03fc9aa, 0x39abdc4529b1661c
	};
	CTB_ASSERT(state);

	s0 = 0;
	s1 = 0;
	s2 = 0;
	s3 = 0;
	for(i = 0; i < 4; i++) {
		uintxx j;

		for(j = 0; j < 64; j++) {
			if (jump[i] & ((uint64) 1 << j)) {
				s0 ^= state->s[0];
				s1 ^= state->s[1];
				s2 ^= state->s[2];
				s3 ^= state->s[3];
			}
			xoshiro256_ppnext(state);
		}
	}
	state->s[0] = s0;
	state->s[1] = s1;
	state->s[2] = s2;
	state->s[3] = s3;
}

uint64
xoshiro256_ppnext(TXoshiro256* state)
{
	uint64 r;
	uint64 t;
	CTB_ASSERT(state);

	r = ROTL64(state->s[0] + state->s[3], 23) + state->s[0];
	t = state->s[1] << 17;

	state->s[2] ^= state->s[0];
	state->s[3] ^= state->s[1];
	state->s[1] ^= state->s[2];
	state->s[0] ^= state->s[3];
	state->s[2] ^= t;

	state->s[3] = ROTL64(state->s[3], 45);
	return r;
}


void
xoroshiro128_seed(TXoroshiro128* state, uint64 seed)
{
	CTB_ASSERT(state);

	state->s[0] = splitmix64(&seed);
	state->s[1] = splitmix64(&seed);
}

void
xoroshiro128_ssjump(TXoroshiro128* state)
{
	uint64 s0;
	uint64 s1;
	uintxx i;
	static const uint64 jump[] = {
		0xdf900294d8f554a5, 0x170865df4b3201fc
	};
	CTB_ASSERT(state);

	s0 = 0;
	s1 = 0;
	for (i = 0; i < 2; i++) {
		uintxx j;

		for (j = 0; j < 64; j++) {
			if (jump[i] & ((uint64) 1 << j)) {
				s0 ^= state->s[0];
				s1 ^= state->s[1];
			}
			xoroshiro128_ssnext(state);
		}
	}

	state->s[0] = s0;
	state->s[1] = s1;
}

uint64
xoroshiro128_ssnext(TXoroshiro128* state)
{
	uint64 s0;
	uint64 s1;
	uint64 r;
	CTB_ASSERT(state);

	s0 = state->s[0];
	s1 = state->s[1];
	r = ROTL64(s0 * 5, 7) * 9;

	s1 ^= s0;
	state->s[0] = ROTL64(s0, 24) ^ s1 ^ (s1 << 16);
	state->s[1] = ROTL64(s1, 37);
	return r;
}

void
xoroshiro128_ppjump(TXoroshiro128* state)
{
	uint64 s0;
	uint64 s1;
	uintxx i;
	static const uint64 jump[] = {
		0x2bd7a6a6e99c2ddc, 0x0992ccaf6a6fca05
	};
	CTB_ASSERT(state);

	s0 = 0;
	s1 = 0;
	for (i = 0; i < 2; i++) {
		uintxx j;

		for (j = 0; j < 64; j++) {
			if (jump[i] & ((uint64) 1 << j)) {
				s0 ^= state->s[0];
				s1 ^= state->s[1];
			}
			xoroshiro128_ppnext(state);
		}
	}

	state->s[0] = s0;
	state->s[1] = s1;
}

uint64
xoroshiro128_ppnext(TXoroshiro128* state)
{
	uint64 s0;
	uint64 s1;
	uint64 r;
	CTB_ASSERT(state);

	s0 = state->s[0];
	s1 = state->s[1];
	r = ROTL64(s0 + s1, 17) + s0;

	s1 ^= s0;
	state->s[0] = ROTL64(s0, 49) ^ s1 ^ (s1 << 21);
	state->s[1] = ROTL64(s1, 28);

	return r;
}

#undef ROTL32
#undef ROTL64
