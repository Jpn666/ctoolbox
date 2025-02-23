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

#include <ctoolbox/ecnfg.h>
#include <ctoolbox/ctype.h>


#if !defined(EOF)
	#define EOF (-1)
#endif


CTB_INLINE void*
request_(struct TECnfg* cfg, uintxx size)
{
	struct TAllocator* a;

	a = cfg->allctr;
	return a->request(size, a->user);
}

CTB_INLINE void
dispose_(struct TECnfg* cfg, void* memory, uintxx size)
{
	struct TAllocator* a;

	a = cfg->allctr;
	a->dispose(memory, size, a->user);
}


TECnfg*
ecnfg_create(TAllocator* allctr)
{
	struct TECnfg* cfg;

	if (allctr == NULL) {
		allctr = (void*) ctb_defaultallocator(NULL);
	}

	cfg = allctr->request(sizeof(struct TECnfg), allctr->user);
	if (cfg == NULL) {
		return NULL;
	}
	cfg->allctr = allctr;

	ecnfg_reset(cfg);
	cfg->bufferend = cfg->buffermem + sizeof(cfg->buffermem);
	return cfg;
}

void
ecnfg_reset(TECnfg* cfg)
{
	CTB_ASSERT(cfg);

	cfg->state = 0;
	cfg->rvcnt = 0;
	cfg->sncnt = 0;
	cfg->event = 0;

	cfg->inputfn  = NULL;
	cfg->payload  = NULL;
	cfg->input    = cfg->inputmem;
	cfg->inputend = cfg->inputmem;

	/* lexer stuff */
	cfg->lastchr = 0x20;  /* space */
	cfg->line  = 0;
	cfg->token = 0;
	cfg->error = 0;

	cfg->buffer    = cfg->buffermem;
	cfg->bufferbgn = cfg->buffermem;
}

void
ecnfg_destroy(TECnfg* cfg)
{
	if (cfg == NULL) {
		return;
	}
	dispose_(cfg, cfg, sizeof(struct TECnfg));
}


#define SETERROR(E) (cfg->error = (E))

CTB_INLINE uintxx
ecnfg_fetchchr(struct TECnfg* cfg)
{
	intxx r;

	if (cfg->input < cfg->inputend) {
		return *cfg->input++;
	}

	if (cfg->inputfn == NULL) {
		SETERROR(ECNFG_EINPUT);
		return (uintxx) EOF;
	}
	r = cfg->inputfn(cfg->inputmem, sizeof(cfg->inputmem), cfg->payload);
	if (r) {
		if (0 > r || r > (intxx) sizeof(cfg->inputmem)) {
			SETERROR(ECNFG_EINPUT);
			return (uintxx) EOF;
		}
		cfg->input    = cfg->inputmem;
		cfg->inputend = cfg->inputmem + r;

		return *cfg->input++;
	}
	return (uintxx) EOF;
}

static void
ecnfg_appendchr(struct TECnfg* cfg, uintxx c)
{
	const uint8 hmask[] = {
		0x00,
		0x00,
		0xc0,
		0xe0,
		0xf0
	};
	uint8* buffer;
	uintxx j;

	if (cfg->error) {
		return;
	}

	/* ascii character */
	if (c < 0x00080UL) {
		if (cfg->bufferend > cfg->buffer) {
			*cfg->buffer++ = (uint8) c;
			return;
		}
		j = 1;
	}
	else {
		/* unicode */
		if (c < 0x00800UL) j = 2; else
		if (c < 0x10000UL) j = 3;
		else
			j = 4;
	}

	if (cfg->buffer + j >= cfg->bufferend) {
		SETERROR(ECNFG_EBUFFERLIMIT);
		return;
	}

	buffer = (cfg->buffer = cfg->buffer + j);
	switch (j) {
		case 4: *--buffer = ((c | 0x80) & 0xbf); c >>= 6;  /* fallthrough */
		case 3: *--buffer = ((c | 0x80) & 0xbf); c >>= 6;  /* fallthrough */
		case 2: *--buffer = ((c | 0x80) & 0xbf); c >>= 6;  /* fallthrough */
		case 1:
			*--buffer = (uint8) ((c | hmask[j]));
			break;
	}
}


#define ISSEQUENCE(N) (((N) & 0xc0) == 0x80)

#define INVALIDCHR 0UL

CTB_INLINE uintxx
ecnfg_fetchunicode(struct TECnfg* cfg)
{
	uintxx c;

	c = ecnfg_fetchchr(cfg);
	if (c < 0x80 || c == (uintxx) EOF) {
		return c;
	}
	else {
		uintxx c1;
		uintxx c2;
		uintxx c3;

		if ((c & 0xe0) == 0xc0) {
			c1 = ecnfg_fetchchr(cfg);

			if (!ISSEQUENCE(c1)) {
				return INVALIDCHR;
			}
			c = c << 6; c += c1;
			c = c - 0x00003080UL;
			if (c < 0x00000080) {
				return INVALIDCHR;
			}
			return c;
		}
		if ((c & 0xf0) == 0xe0) {
			c1 = ecnfg_fetchchr(cfg);
			c2 = ecnfg_fetchchr(cfg);

			if (!ISSEQUENCE(c1) || !ISSEQUENCE(c2)) {
				return INVALIDCHR;
			}
			c = c << 6; c += c1;
			c = c << 6; c += c2;
			c = c - 0x000E2080UL;
			if (c < 0x00000800 || (c >= 0xd800 && c <= 0xdfff)) {
				return INVALIDCHR;
			}
			return c;
		}
		if ((c & 0xf8) == 0xf0) {
			c1 = ecnfg_fetchchr(cfg);
			c2 = ecnfg_fetchchr(cfg);
			c3 = ecnfg_fetchchr(cfg);

			if (!ISSEQUENCE(c1) || !ISSEQUENCE(c2) || !ISSEQUENCE(c3)) {
				return INVALIDCHR;
			}
			c = c << 6; c += c1;
			c = c << 6; c += c2;
			c = c << 6; c += c3;
			c = c - 0x03C82080UL;
			if (c < 0x00010000 || c > 0x10ffffUL) {
				return INVALIDCHR;
			}
			return c;
		}
	}
	return INVALIDCHR;
}

#undef ISSEQUENCE


CTB_INLINE uintxx
ecnfg_readhex(struct TECnfg* cfg)
{
	uintxx c;
	uintxx b;
	uintxx n;
	uintxx i;
	uintxx overflow;

	n = overflow = i = 0;
	for (c = cfg->lastchr; ;c = ecnfg_fetchchr(cfg)) {
		if (ctb_isdigit(c)) {
			b = c - 0x30;
		}
		else {
			b = c | 0x20;
			if (b >= 0x61 && b <= 0x66) {
				b = b - 0x61 + 10;
			}
			else {
				break;
			}
		}

		if (overflow || n > 0xfffffffUL) {
			overflow = 1;
			continue;
		}
		n = (n << 4) + b;
		i++;
	}

	if (i == 0 || overflow) {
		return UINT_MAX;
	}
	cfg->lastchr = c;
	return n;
}


/* tokens */
enum {
	ECNFG_TKNINVALID = 0,
	ECNFG_TKNSTR     = ECNFG_TYPESTR,  /* 1 */
	ECNFG_TKNINT     = ECNFG_TYPEINT,  /* 2 */
	ECNFG_TKNFLT     = ECNFG_TYPEFLT,  /* 3 */

	ECNFG_TKNIDENT   = 4,
	ECNFG_TKNCOLON   = 5,
	ECNFG_TKNLBRACE  = 6,
	ECNFG_TKNRBRACE  = 7,
	ECNFG_TKNEOF     = 8
};

static uintxx
ecnfg_parsestring(struct TECnfg* cfg)
{
	uintxx j;
	uintxx c;

	j = 0;
	while ((c = ecnfg_fetchunicode(cfg)) != 0x22) {
		if (c == 0x0a || c == (uintxx) EOF) {
			break;
		}

		/* unicode characters are only allowed in strings */
		if (c == INVALIDCHR || (ctb_isascii(c) && ctb_iscntrl(c))) {
			SETERROR(ECNFG_EINVALIDCHR);
			return ECNFG_TKNINVALID;
		}

		if (j == 0x00) {
			if (c == 0x24) {
				j = 1;
				continue;
			}
			ecnfg_appendchr(cfg, c);
			continue;
		}
		if (c != 0x28) {
			ecnfg_appendchr(cfg, 0x24);
			continue;
		}

		/* escape sequence */
		cfg->lastchr = ecnfg_fetchchr(cfg);
		for (;;) {
			while (cfg->lastchr == 0x20)
				cfg->lastchr = ecnfg_fetchchr(cfg);

			if (cfg->lastchr == 0x29)
				break;

			c = ecnfg_readhex(cfg);
			if (c == UINT_MAX) {
				SETERROR(ECNFG_EINVALIDESCAPESQ);
				return ECNFG_TKNINVALID;
			}

			if (c == 0 || c > 0x10ffffUL || (c >= 0xd800 && c <= 0xdfff)) {
				SETERROR(ECNFG_EINVALIDCHR);
				return ECNFG_TKNINVALID;
			}
			ecnfg_appendchr(cfg, c);
		}
		j = 0;
	}

	if (cfg->error) {
		return ECNFG_TKNINVALID;
	}
	ecnfg_appendchr(cfg, 0);

	if (c != 0x22) {
		SETERROR(ECNFG_EUNTERMINATEDSTR);
		return ECNFG_TKNINVALID;
	}
	return ECNFG_TKNSTR;
}


static uintxx
ecnfg_guesttype(struct TECnfg* cfg)
{
	uintxx exponent;
	uintxx isint;
	uintxx ishex;
	uintxx expsign;
	uintxx e, n, c;
	uintxx lchr;

	c = 0;
	n = 0;
	for (lchr = cfg->lastchr; lchr; lchr = ecnfg_fetchchr(cfg)) {
		if (lchr != 0x2d && lchr != 0x2b)
			break;
		c = lchr;
	}
	if (c)
		ecnfg_appendchr(cfg, c);

	exponent = 0;
	expsign  = 0;

	ishex = 0;
	isint = 1;
	if (lchr == 0x30) {
		ecnfg_appendchr(cfg, lchr);
		lchr = ecnfg_fetchchr(cfg);
		n++;

		if ((lchr | 0x20) == 0x78) {
			ecnfg_appendchr(cfg, lchr);
			lchr = ecnfg_fetchchr(cfg);

			ishex = 1;
			n = 0;
		}
	}

	for (e = 0; lchr; lchr = ecnfg_fetchchr(cfg)) {
		if (ctb_isdigit(lchr)) {
			if (exponent) {
				e++;
			}
			else {
				n++;
			}
		}
		else {
			if (ishex) {
				if (((c = lchr | 0x20) >= 0x61) && (c <= 0x66)) {
					n++;
					ecnfg_appendchr(cfg, lchr);
					continue;
				}
				break;
			}
			if (exponent) {
				if (e == 0 && expsign == 0) {
					if (lchr == 0x2d || lchr == 0x2b) {
						expsign++;
						ecnfg_appendchr(cfg, lchr);
						continue;
					}
				}
				break;
			}
			if (lchr == 0x2e) {
				if (isint == 0) {
					break;
				}
				isint = 0;
			}
			else {
				if ((lchr | 0x20) == 0x65) {
					exponent = 1;
				}
				else {
					break;
				}
			}
		}
		ecnfg_appendchr(cfg, lchr);
	}

	if (n) {
		if (isint) {
			if (exponent == 0 || e == 0) {
				ecnfg_appendchr(cfg, 0);
				cfg->lastchr = lchr;
				return ECNFG_TKNINT;
			}
		}
		if (exponent && e == 0) {
			return ECNFG_TKNINVALID;
		}
		ecnfg_appendchr(cfg, 0);
		cfg->lastchr = lchr;
		return ECNFG_TKNFLT;
	}
	return ECNFG_TKNINVALID;
}

static uintxx
ecnfg_parsenumber(struct TECnfg* cfg)
{
	uintxx ntype;
	uintxx lchr;

	ntype = ecnfg_guesttype(cfg);
	if (cfg->error) {
		return ECNFG_TKNINVALID;
	}

	if (ntype == ECNFG_TKNFLT || ntype == ECNFG_TKNINT) {
		lchr = cfg->lastchr;
		if (lchr == 0x7b ||
		    lchr == 0x7d ||
		    lchr == 0x2d ||
		    lchr == 0x2b || lchr == (uintxx) EOF || ctb_isspace(lchr)) {
			return ntype;
		}
	}

	SETERROR(ECNFG_EINVALIDTKN);
	return ECNFG_TKNINVALID;
}

static uintxx
ecnfg_nexttoken(struct TECnfg* cfg)
{
	uintxx r;
	uintxx lchr;

	cfg->buffer    = cfg->bufferbgn;
	cfg->buffer[0] = 0x00;

	lchr = cfg->lastchr;

L_LOOP:
	while (ctb_isspace(lchr)) {
		if (lchr == 0x0a) {
			lchr = ecnfg_fetchchr(cfg);
			cfg->line++;
			continue;
		}
		lchr = ecnfg_fetchchr(cfg);
	}

	switch (lchr) {
		case 0x7b:
			cfg->lastchr = ecnfg_fetchchr(cfg);
			return ECNFG_TKNLBRACE;

		case 0x7d:
			cfg->lastchr = ecnfg_fetchchr(cfg);
			return ECNFG_TKNRBRACE;

		case 0x3a:
			cfg->lastchr = ecnfg_fetchchr(cfg);
			return ECNFG_TKNCOLON;

		case 0x23:
			while ((lchr = ecnfg_fetchchr(cfg)) != 0x0a) {
				if (lchr == 0) {
					if (cfg->error == 0) {
						SETERROR(ECNFG_EINVALIDCHR);
					}
					return ECNFG_TKNINVALID;
				}
				if (lchr == (uintxx) EOF) {
					if (cfg->error) {
						return ECNFG_TKNINVALID;
					}
					return ECNFG_TKNEOF;
				}
			}
			lchr = ecnfg_fetchchr(cfg);
			cfg->line++;
			goto L_LOOP;

		case EOF:
			if (cfg->error) {
				return ECNFG_TKNINVALID;
			}
			return ECNFG_TKNEOF;
	}

	if (ctb_iscntrl(lchr) || ctb_isascii(lchr) == 0) {
		if (cfg->error == 0) {
			SETERROR(ECNFG_EINVALIDCHR);
		}
		return ECNFG_TKNINVALID;
	}

	if (ctb_isalpha(lchr)) {
		for (;;) {
			ecnfg_appendchr(cfg, lchr);

			lchr = ecnfg_fetchchr(cfg);
			if (lchr == 0x2d ||
				lchr == 0x2b || ctb_isalnum(lchr)) {
				continue;
			}
			break;
		}

		if (cfg->error) {
			return ECNFG_TKNINVALID;
		}
		ecnfg_appendchr(cfg, 0);

		cfg->lastchr = lchr;
		return ECNFG_TKNIDENT;
	}

	if (lchr == 0x2e ||
		lchr == 0x2d ||
		lchr == 0x2b || ctb_isdigit(lchr)) {
		cfg->lastchr = lchr;
		return ecnfg_parsenumber(cfg);
	}

	if (lchr == 0x22) {
		r = ecnfg_parsestring(cfg);
		cfg->lastchr = ecnfg_fetchchr(cfg);
		return r;
	}

	while (ctb_isspace(lchr) == 0) {
		lchr = ecnfg_fetchchr(cfg);
	}
	SETERROR(ECNFG_EINVALIDTKN);
	return ECNFG_TKNINVALID;
}


#define NEXTTOKEN(C) ((C)->token = ecnfg_nexttoken(C))


static uintxx
ecnfg_readnextstr(struct TECnfg* cfg)
{
	uintxx r;
	uintxx lchr;

	if (cfg->buffer - cfg->bufferbgn) {
		cfg->buffer--;
	}

	lchr = cfg->lastchr;

L_LOOP:
	while (ctb_isspace(lchr)) {
		if (lchr == 0x0a) {
			lchr = ecnfg_fetchchr(cfg);
			cfg->line++;
			continue;
		}
		lchr = ecnfg_fetchchr(cfg);
	}

	switch (lchr) {
		case 0x23:
			do {
				lchr = ecnfg_fetchchr(cfg);
			} while (lchr != 0x0a);

			lchr = ecnfg_fetchchr(cfg);
			cfg->line++;
			goto L_LOOP;

		case EOF:
			cfg->lastchr = lchr;
			return 0;
	}

	if (ctb_isalpha(lchr)) {
		cfg->lastchr = lchr;
		return 0;
	}
	if (lchr == 0x2e ||
	    lchr == 0x2d ||
	    lchr == 0x2b || ctb_isdigit(lchr)) {
		return 0;
	}

	if (lchr == 0x22) {
		r = ecnfg_parsestring(cfg);
		if (r == ECNFG_TKNINVALID) {
			cfg->token = ECNFG_TKNINVALID;
			return 0;
		}
		cfg->lastchr = ecnfg_fetchchr(cfg);
		return 1;
	}

	cfg->lastchr = lchr;
	return 0;
}


#define ISRVALUE(TKN) (((TKN) >= ECNFG_TKNSTR) && ((TKN) <= ECNFG_TKNFLT))

eECNFGType
ecnfg_nextrvaltype(TECnfg* cfg)
{
	uintxx rtype;
	CTB_ASSERT(cfg);

	rtype = ECNFG_TYPENONE;
	if (cfg->state != ECNFG_PARSING) {
		return rtype;
	}

	switch (cfg->event) {
		case ECNFG_EVNTDIRECTIVE:
			if (cfg->rvcnt) {
				NEXTTOKEN(cfg);
			}

			if (ISRVALUE(cfg->token))
				rtype = cfg->token;
			cfg->rvcnt = 1;
			break;

		case ECNFG_EVNTENTRY:
			if (cfg->rvcnt) {
				break;
			}

			rtype = cfg->token;
			cfg->rvcnt = 1;
			break;
	}
	return rtype;
}


#define DISPATCH(EVNT) return (cfg->event = (EVNT))

CTB_INLINE void
restorebuffer(struct TECnfg* cfg)
{
	if (cfg->event == ECNFG_EVNTDIRECTIVE) {
		if (cfg->token == ECNFG_TKNIDENT) {
			cfg->buffer = cfg->buffermem;
			while (cfg->bufferbgn[0]) {
				*cfg->buffer++ = *cfg->bufferbgn++;
			}
			*cfg->buffer++ = 0x00;
		}
		cfg->bufferbgn = cfg->buffermem;

		while (ISRVALUE(cfg->token)) {
			NEXTTOKEN(cfg);
		}
		return;
	}
	cfg->bufferbgn = cfg->buffermem;
	NEXTTOKEN(cfg);
}

eECNFGEvent
ecnfg_nextevnttype(TECnfg* cfg)
{
	uintxx colon;
	CTB_ASSERT(cfg);

	if (cfg->state != ECNFG_PARSING) {
		if (cfg->state == ECNFG_READY) {
			cfg->state = ECNFG_PARSING;
		}
		else {
			DISPATCH(ECNFG_EVNTNONE);
		}
	}
	restorebuffer(cfg);

	colon = 0;
	cfg->rvcnt = 0;

	switch (cfg->token) {
		case ECNFG_TKNIDENT:
			cfg->bufferbgn = cfg->buffer;

			NEXTTOKEN(cfg);
			goto L_HASIDENT;

		case ECNFG_TKNRBRACE:
			cfg->sncnt--;
			if (cfg->sncnt < 0) {
				SETERROR(ECNFG_EUNPAIREDSECTION);
				goto L_ERROR;
			}
			DISPATCH(ECNFG_EVNTSECTIONEND);

		case ECNFG_TKNLBRACE:
		case ECNFG_TKNSTR:
		case ECNFG_TKNINT:
		case ECNFG_TKNFLT:
		case ECNFG_TKNCOLON:
			SETERROR(ECNFG_EUNSPECTEDTKN);

		case ECNFG_TKNINVALID:
			goto L_ERROR;

		case ECNFG_TKNEOF:
			if (cfg->sncnt) {
				SETERROR(ECNFG_EUNPAIREDSECTION);
				goto L_ERROR;
			}
			cfg->state = ECNFG_DONE;
			DISPATCH(ECNFG_EVNTNONE);
	}

L_HASIDENT:
	switch (cfg->token) {
		case ECNFG_TKNCOLON:
			colon = 1;

			NEXTTOKEN(cfg);
			goto L_HASIDENT;

		case ECNFG_TKNLBRACE:
			cfg->sncnt++;
			if (cfg->sncnt == INTXX_MAX) {
				SETERROR(ECNFG_ELIMIT);
				goto L_ERROR;
			}
			DISPATCH(ECNFG_EVNTSECTIONBGN);

		case ECNFG_TKNSTR:
			if (colon) {
				while (ecnfg_readnextstr(cfg))
					;
				if (cfg->error) {
					goto L_ERROR;
				}
				DISPATCH(ECNFG_EVNTENTRY);
			}

		/* fallthrough */
		case ECNFG_TKNINT:
		case ECNFG_TKNFLT:
			if (colon) {
				DISPATCH(ECNFG_EVNTENTRY);
			}

		/* fallthrough */
		case ECNFG_TKNRBRACE:
		case ECNFG_TKNEOF:
			if (colon) {
				SETERROR(ECNFG_EUNSPECTEDTKN);
				goto L_ERROR;
			}
			DISPATCH(ECNFG_EVNTDIRECTIVE);

		case ECNFG_TKNIDENT:
			if (colon) {
				SETERROR(ECNFG_EUNSPECTEDTKN);
				goto L_ERROR;
			}

			/* we have 2 identifiers in the buffer now */
			DISPATCH(ECNFG_EVNTDIRECTIVE);

		case ECNFG_TKNINVALID:
			goto L_ERROR;
	}

L_ERROR:
	cfg->state = ECNFG_ABORTED;
	DISPATCH(ECNFG_EVNTNONE);
}

#undef ISRVALUE
#undef DISPATCH
#undef SETERROR


const uint8*
ecnfg_getlval(TECnfg* cfg)
{
	CTB_ASSERT(cfg);

	if (cfg->state != ECNFG_PARSING) {
		return NULL;
	}
	switch (cfg->event) {
		case ECNFG_EVNTDIRECTIVE:
		case ECNFG_EVNTSECTIONBGN:
		case ECNFG_EVNTENTRY:
			return (const uint8*) cfg->buffermem;
	}
	return NULL;
}

const uint8*
ecnfg_getrval(TECnfg* cfg)
{
	CTB_ASSERT(cfg);

	if (cfg->state != ECNFG_PARSING) {
		return NULL;
	}
	switch (cfg->event) {
		case ECNFG_EVNTDIRECTIVE:
		case ECNFG_EVNTENTRY:
			return (const uint8*) cfg->bufferbgn;
	}
	return NULL;
}


#undef ECNFG_INPUTBFFRSZ
