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

#ifndef abf33644_6d5e_4a96_a46d_a70d8c9c58e1
#define abf33644_6d5e_4a96_a46d_a70d8c9c58e1

/*
 * base64.h
 * Stream oriented base64 (rfc4648) encoder and decoder.
 */

#include "../ctoolbox.h"


/* Return codes for encode or decode call */
typedef enum {
	B64STRM_OK        = 0,
	B64STRM_SRCEXHSTD = 1,
	B64STRM_TGTEXHSTD = 2,
	B64STRM_ERROR     = 3
} eB64StrmResult;


/* */
struct TBase64Strm {
	uint32 state;
	uint32 mode;
	uint32 final;

	/* stream buffer */
	uint8* source;
	uint8* target;
	uint8* sbgn;
	uint8* send;
	uint8* tbgn;
	uint8* tend;

	/* remaining input (or output) bytes */
	uint32 rcount;
	uint32 r;
};

typedef struct TBase64Strm TBase64Strm;


/*
 * */
CTB_INLINE void b64strm_init(TBase64Strm*);

/*
 * */
CTB_INLINE void b64strm_setsrc(TBase64Strm*, uint8* source, uintxx size);

/*
 * */
CTB_INLINE void b64strm_settgt(TBase64Strm*, uint8* target, uintxx size);

/*
 * */
CTB_INLINE uintxx b64strm_srcend(TBase64Strm*);

/*
 * */
CTB_INLINE uintxx b64strm_tgtend(TBase64Strm*);

/*
 * */
uintxx b64strm_encode(TBase64Strm*, uintxx final);

/*
 * */
uintxx b64strm_decode(TBase64Strm*, uintxx final);


/*
 * Inlines */

CTB_INLINE void
b64strm_init(TBase64Strm* strm)
{
	CTB_ASSERT(strm);
	strm->state  = 0;
	strm->mode   = 0;
	strm->rcount = 0;
	strm->final  = 0;
	strm->r = 0;
	strm->source = strm->sbgn = strm->send = NULL;
	strm->target = strm->tbgn = strm->tend = NULL;
}

CTB_INLINE void
b64strm_setsrc(TBase64Strm* strm, uint8* source, uintxx size)
{
	CTB_ASSERT(strm);
	strm->sbgn = strm->source = source;
	strm->send = source + size;
}

CTB_INLINE void
b64strm_settgt(TBase64Strm* strm, uint8* target, uintxx size)
{
	CTB_ASSERT(strm);
	strm->tbgn = strm->target = target;
	strm->tend = target + size;
}

CTB_INLINE uintxx
b64strm_srcend(TBase64Strm* strm)
{
	CTB_ASSERT(strm);
	return (uintxx) (strm->source - strm->sbgn);
}

CTB_INLINE uintxx
b64strm_tgtend(TBase64Strm* strm)
{
	CTB_ASSERT(strm);
	return (uintxx) (strm->target - strm->tbgn);
}

#endif
