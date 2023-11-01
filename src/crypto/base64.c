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

#include <ctoolbox/crypto/base64.h>


/* stream mode */
#define B64STRM_MODEENC 1
#define B64STRM_MODEDEC 2


static const uint8 b64enctable[] = {
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
	0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
	0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f
};


uintxx
b64strm_encode(TBase64Strm* strm, uintxx final)
{
	uintxx j;
	uint32 w;
	uintxx stotal;
	uintxx ttotal;
	uint32 a;
	uint32 b;
	uint32 c;
	uint32 d;
	uint8* s;
	uint8* t;
	uint8* send;
	uint8* tend;
	CTB_ASSERT(strm);

	if (UNLIKELY(strm->mode ^ B64STRM_MODEENC)) {
		if (strm->mode == 0) {
			strm->mode = B64STRM_MODEENC;
		}
		else {
			strm->state = B64STRM_ERROR;
			strm->mode  = B64STRM_ERROR;
			return strm->state;
		}
	}

	if (UNLIKELY(strm->state == B64STRM_SRCEXHSTD)) {
		if (strm->rcount == 3) {
			goto L1;
		}
		while (strm->send > strm->source) {
			strm->r = (strm->r << 8) | *strm->source++;
			if (++strm->rcount == 3) {
				a = b64enctable[(strm->r >> 18) & 0x3f];
				b = b64enctable[(strm->r >> 12) & 0x3f];
				c = b64enctable[(strm->r >>  6) & 0x3f];
				d = b64enctable[(strm->r >>  0) & 0x3f];
				if (strm->tend - strm->target >= 4) {
					strm->target[0] = (uint8) a;
					strm->target[1] = (uint8) b;
					strm->target[2] = (uint8) c;
					strm->target[3] = (uint8) d;
					strm->target += 4;
					break;
				}

				strm->r = (d << 24) | (c << 16) | (b << 8) | (a);
				strm->rcount = 0;
				for (; strm->tend > strm->target; strm->rcount++) {
					*strm->target++ = (uint8) strm->r;
					strm->r >>= 8;
				}
				return (strm->state = B64STRM_TGTEXHSTD);
			}
		}

		if (strm->rcount ^ 3) {
			if (final) {
				goto L2;
			}
			return B64STRM_SRCEXHSTD;
		}
	}
	else {
		if (UNLIKELY(strm->state == B64STRM_TGTEXHSTD)) {
			if (strm->rcount ^ 4) {
				while (strm->tend > strm->target) {
					*strm->target++ = (uint8) strm->r;
					strm->r >>= 8;
					if (++strm->rcount == 4) {
						strm->r = 0;
						break;
					}
				}
				if (strm->rcount ^ 4) {
					return B64STRM_TGTEXHSTD;
				}
				if (strm->final) {
					return (strm->state = 0);
				}
			}
		}
	}

L1:
	stotal = (uintxx) (strm->send - strm->source);
	ttotal = (uintxx) (strm->tend - strm->target);

	s = strm->source;
	t = strm->target;
	while (stotal >= 24 * 6 && ttotal >= 24 * 8) {
		uint32 w1;
		uint32 w2;

		for (j = 24; j; j--) {
#if defined(CTB_FASTUNALIGNED)
			w1 = ((uint32*) (s + 0))[0];
			w2 = ((uint32*) (s + 3))[0];
			w1 = CTB_SWAP32ONLE(w1);
			w2 = CTB_SWAP32ONLE(w2);
#else
			w1 = (s[0+0] << 24) | (s[0+1] << 16) | (s[0+2] << 8);
			w2 = (s[3+0] << 24) | (s[3+1] << 16) | (s[3+2] << 8);
#endif

			t[0] = b64enctable[(w1 >> 26) & 0x3f];
			t[1] = b64enctable[(w1 >> 20) & 0x3f];
			t[2] = b64enctable[(w1 >> 14) & 0x3f];
			t[3] = b64enctable[(w1 >>  8) & 0x3f];
			t += 4;
			t[0] = b64enctable[(w2 >> 26) & 0x3f];
			t[1] = b64enctable[(w2 >> 20) & 0x3f];
			t[2] = b64enctable[(w2 >> 14) & 0x3f];
			t[3] = b64enctable[(w2 >>  8) & 0x3f];
			t += 4;
			s += 6;
		}
		stotal -= 24 * 6;
		ttotal -= 24 * 8;
	}

	send = strm->send;
	tend = strm->tend;
	while (send - s >= 3) {
#if defined(CTB_FASTUNALIGNED)
		w = ((uint32*) (s + 0))[0];
		w = CTB_SWAP32ONLE(w);
#else
		w = (s[0] << 24) | (s[1] << 16) | (s[2] << 8);
#endif
		s += 3;

		a = b64enctable[(w >> 26) & 0x3f];
		b = b64enctable[(w >> 20) & 0x3f];
		c = b64enctable[(w >> 14) & 0x3f];
		d = b64enctable[(w >>  8) & 0x3f];
		if (LIKELY(ttotal >= 4)) {
			t[0] = (uint8) a;
			t[1] = (uint8) b;
			t[2] = (uint8) c;
			t[3] = (uint8) d;
			t += 4;
		}
		else {
			strm->r = (d << 24) | (c << 16) | (b << 8) | (a);
			for (strm->rcount = 0; tend > t; strm->rcount++) {
				*t++ = (uint8) strm->r; strm->r >>= 8;
			}
			strm->source = s;
			strm->target = t;
			return (strm->state = B64STRM_TGTEXHSTD);
		}
		ttotal -= 4;
	}

	strm->source = s;
	strm->target = t;

	strm->rcount = (uint32) (strm->send - strm->source);
	strm->r = 0;
	switch (strm->rcount) {
		case 2: strm->r = (strm->r << 8) | *strm->source++;  /* fallthrough */
		case 1: strm->r = (strm->r << 8) | *strm->source++;
	}

L2:
	if (final == 0) {
		if (strm->rcount) {
			return (strm->state = B64STRM_SRCEXHSTD);
		}

		strm->rcount = 3;
		return (strm->state = B64STRM_SRCEXHSTD);
	}

	strm->final = 1;
	if (strm->rcount) {
		strm->r <<= 8;
		if (strm->rcount == 1) {
			strm->r <<= 8;
		}

		a = b64enctable[(strm->r >> 18) & 0x3f];
		b = b64enctable[(strm->r >> 12) & 0x3f];
		if (strm->rcount == 2) {
			c = b64enctable[(strm->r >> 6) & 0x3f];
			d = 0x3d;
		}
		else {
			c = 0x3d;
			d = 0x3d;
		}

		if (strm->tend - strm->target >= 4) {
			*strm->target++ = (uint8) a;
			*strm->target++ = (uint8) b;
			*strm->target++ = (uint8) c;
			*strm->target++ = (uint8) d;
			return (strm->state = 0);
		}

		strm->r = (d << 24) | (c << 16) | (b << 8) | (a);
		for (strm->rcount = 0; strm->tend > strm->target; strm->rcount++) {
			*strm->target++ = (uint8) strm->r;
			strm->r >>= 8;
		}
		return (strm->state = B64STRM_TGTEXHSTD);
	}

	return (strm->state = 0);
}


static const uint8 b64dectable[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x7f,
	0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0xff, 0xff, 0xff, 0x40, 0xff, 0xff,
	0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
	0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

#if CTB_IS_LITTLEENDIAN
	#define BYTEOFFSET1 010
	#define BYTEOFFSET2 020
	#define BYTEOFFSET3 030
#else
	#define BYTEOFFSET1 020
	#define BYTEOFFSET2 010
	#define BYTEOFFSET3 000
#endif

uintxx
b64strm_decode(TBase64Strm* strm, uintxx final)
{
	uint32 n;
	uint32 w;
	uintxx ttotal;
	uintxx r;
	uint8* s;
	uint8* t;
	uint8* send;
	uint8* tend;
	CTB_ASSERT(strm);

	if (UNLIKELY(strm->mode ^ B64STRM_MODEDEC)) {
		if (strm->mode == 0) {
			strm->mode = B64STRM_MODEDEC;
		}
		else {
			strm->state = B64STRM_ERROR;
			strm->mode  = B64STRM_ERROR;
			return strm->state;
		}
	}

	if (UNLIKELY(strm->state == B64STRM_SRCEXHSTD)) {
		if (strm->rcount == 4) {
			goto L1;
		}

		while (strm->send > strm->source) {
			n = b64dectable[*strm->source++];
			if (n >= 64) {
				if (n == 0xff) {
					return (strm->state = B64STRM_ERROR);
				}
				continue;
			}
			strm->r = (strm->r << 6) | (uint8) n;
			if (++strm->rcount == 4) {
				w = ctb_swap32(strm->r);
				if (strm->tend - strm->target >= 3) {
					strm->target[0] = (uint8) (w >> BYTEOFFSET1);
					strm->target[1] = (uint8) (w >> BYTEOFFSET2);
					strm->target[2] = (uint8) (w >> BYTEOFFSET3);
					strm->target += 3;
					break;
				}

				strm->r = w;
				strm->rcount = 0;
				for (; strm->tend > strm->target; strm->rcount++) {
					*strm->target++ = (uint8) (strm->r >>= 8);
				}
				return (strm->state = B64STRM_TGTEXHSTD);
			}
		}

		if (strm->rcount ^ 4) {
			if (final) {
				goto L2;
			}
			return B64STRM_SRCEXHSTD;
		}
	}
	else {
		if (UNLIKELY(strm->state == B64STRM_TGTEXHSTD)) {
			if (strm->rcount ^ 3) {
				while (strm->tend > strm->target) {
					*strm->target++ = (uint8) (strm->r >>= 8);
					if (++strm->rcount == 3) {
						break;
					}
				}
				if (strm->rcount ^ 3) {
					return B64STRM_TGTEXHSTD;
				}
			}

			if (strm->final) {
				return (strm->state = 0);
			}
		}
	}

L1:
	ttotal = (uintxx) (strm->tend - strm->target);

	s = strm->source;
	t = strm->target;
	send = strm->send;
	tend = strm->tend;
	for (r = 0; ttotal >= 4; ttotal -= 3) {
		uint32 a;
		uint32 b;
		uint32 c;
		uint32 d;
		uint32 f1;
		uint32 f2;

		if (LIKELY(send - s >= 4)) {
			a = b64dectable[s[0]];
			b = b64dectable[s[1]];
			c = b64dectable[s[2]];
			d = b64dectable[s[3]];
		}
		else {
			break;
		}

		f1 = a | b;
		f2 = c | d;
		if (UNLIKELY(f1 >= 64 || f2 >= 64)) {
			w = 0;
			for (r = 0; r < 4; r++) {
				while ((n = b64dectable[*s++]) >= 64) {
					if (n == 0xff) {
						strm->source = s;
						strm->target = t;

						strm->mode = strm->state = B64STRM_ERROR;
						return strm->state;
					}
					if (UNLIKELY(s == send)) {
						strm->source = s;
						strm->target = t;

						strm->r = w;
						if (final == 0) {
							strm->rcount = (uint32) r;
							if (r == 0)
								strm->rcount = 4;
							return (strm->state = B64STRM_SRCEXHSTD);
						}
						goto L2;
					}
				}
				w = (w << 6) | (uint8) n;
			}
			r = 0;
		}
		else {
			w = (a << 18) | (b << 12) | (c << 6) | (d);
			s += 4;
		}
#if defined(CTB_FASTUNALIGNED)
		w = CTB_SWAP32ONLE(w << 8);
		((uint32*) t)[0] = w;
#else
		t[0] = (uint8) (w >> BYTEOFFSET1);
		t[1] = (uint8) (w >> BYTEOFFSET2);
		t[2] = (uint8) (w >> BYTEOFFSET3);
#endif
		t += 3;
	}

	strm->r = 0;
	for (r = 0; send > s; ) {
		n = b64dectable[*s++];
		if (UNLIKELY(n >= 64)) {
			if (n == 0xff) {
				strm->source = s;
				strm->target = t;

				strm->mode = strm->state = B64STRM_ERROR;
				return strm->state;
			}
			continue;
		}
		strm->r = (strm->r << 6) | (uint8) n;
		if (++r == 4) {
			w = ctb_swap32(strm->r);
			if (LIKELY(tend - t >= 3)) {
				t[0] = (uint8) (w >> BYTEOFFSET1);
				t[1] = (uint8) (w >> BYTEOFFSET2);
				t[2] = (uint8) (w >> BYTEOFFSET3);
				t += 3;
			}
			else {
				strm->rcount = 0;
				for (; tend > t; strm->rcount++) {
					*t++ = (uint8) (strm->r >>= 8);
				}
				strm->source = s;
				strm->target = t;
				return (strm->state = B64STRM_TGTEXHSTD);
			}
			strm->r = 0;
			r = 0;
		}
	}

	strm->rcount = (uint32) r;
	if (r == 0)
		strm->rcount = 4;
	strm->source = s;
	strm->target = t;
	if (final == 0) {
		return (strm->state = B64STRM_SRCEXHSTD);
	}

L2:
	strm->final = 1;

	r = strm->rcount;
	if (r == 2 || r == 3) {
		strm->r <<= 6;
		if (r == 2) {
			strm->r <<= 6;
		}
		w = ctb_swap32(strm->r);

		strm->rcount = (uint32) (3 - (r - 1));
		strm->r = w;
		while (strm->tend > strm->target) {
			*strm->target++ = (uint8) (strm->r >>= 8);
			if (++strm->rcount == 3) {
				return (strm->state = 0);
			}
		}
		return (strm->state = B64STRM_TGTEXHSTD);
	}

	return (strm->state = 0);
}
