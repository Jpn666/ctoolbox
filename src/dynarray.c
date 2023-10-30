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

#include "../dynarray.h"
#include "../cmemory.h"


CTB_INLINE void*
_reserve(struct TDynArray* p, uintxx amount)
{
	if (p->allctr) {
		return p->allctr->reserve(p->allctr->user, amount);
	}
	return CTB_RESERVE(amount);
}

CTB_INLINE void*
_realloc(struct TDynArray* p, void* memory, uintxx amount)
{
	if (p->allctr) {
		return p->allctr->realloc(p->allctr->user, memory, amount);
	}
	return CTB_REALLOC(memory, amount);
}

CTB_INLINE void
_release(struct TDynArray* p, void* memory)
{
	if (p->allctr) {
		p->allctr->release(p->allctr->user, memory);
		return;
	}
	CTB_RELEASE(memory);
}


TDynArray*
dynarray_create(uintxx size, uintxx datasize, TAllocator* allctr)
{
	struct TDynArray* array;

	if (datasize == 0) {
		return NULL;
	}

	if (allctr) {
		array = allctr->reserve(allctr->user, sizeof(struct TDynArray));
	}
	else {
		array = CTB_RESERVE(sizeof(struct TDynArray));
	}
	if (array == NULL) {
		return NULL;
	}

	array->allctr = allctr;
	if (DYNARRAY_MINSIZE > size) {
		size = DYNARRAY_MINSIZE;
	}
	array->capacity = size;
	array->datasize = datasize;
	array->used = 0;

	array->buffer = _reserve(array, size * datasize);
	if (array->buffer == NULL) {
		_release(array, array);
		return NULL;
	}

	return array;
}

void
dynarray_destroy(TDynArray* array)
{
	if (array == NULL) {
		return;
	}
	_release(array, array->buffer);
	_release(array, array);
}


#define AS_POINTER(A) ((void*) DYNARRAY_AT(array, (A)))

eintxx
dynarray_insert(TDynArray* array, void* element, uintxx index)
{
	uintxx i;
	CTB_ASSERT(array);

	if (index < array->used) {
		if (array->used >= array->capacity) {
			if (dynarray_reserve(array, array->used + 1)) {
				return CTB_EOOM;
			}
		}
		array->used++;
		for (i = array->used; index < i; i--) {
			ctb_memcpy(AS_POINTER(i), AS_POINTER(i - 1), array->datasize);
		}
	}
	else {
		if (index >= array->capacity) {
			if (dynarray_reserve(array, index + 1)) {
				return CTB_EOOM;
			}
		}
		for (i = array->used; i < index; i++) {
			ctb_memset(AS_POINTER(i), 0, array->datasize);
		}
		array->used = index + 1;
	}
	ctb_memcpy(AS_POINTER(index), element, array->datasize);

	return CTB_OK;
}

eintxx
dynarray_remove(TDynArray* array, uintxx index)
{
	uintxx i;
	CTB_ASSERT(array && dynarray_checkrange(array, index));

	for (i = index + 1; i < array->used; i++) {
		ctb_memcpy(AS_POINTER(i - 1), AS_POINTER(i), array->datasize);
	}
	array->used--;

	return CTB_OK;
}

eintxx
dynarray_clear(TDynArray* array, TFreeFn freefn)
{
	CTB_ASSERT(array);

	if (freefn) {
		while (array->used) {
			array->used--;
			freefn(AS_POINTER(array->used));
		}
	}
	else {
		array->used = 0;
	}

	if (dynarray_shrink(array)) {
		return CTB_EOOM;
	}
	return CTB_OK;
}

#undef AS_POINTER


static eintxx
dynarray_resize(TDynArray* array, uintxx size)
{
	uint8* buffer;
	uintxx v;

	v = size;
	v--;
	v |= v >> 0x01;
	v |= v >> 0x02;
	v |= v >> 0x04;
	v |= v >> 0x08;
	v |= v >> 0x10;
#if defined(CTB_ENV64)
	v |= v >> 0x20;
#endif
	v++;

	if (v > array->capacity) {
		if (array->capacity > v - (array->capacity >> 1))
			v = v << 1;

		buffer = _realloc(array, array->buffer, v * array->datasize);
		if (buffer == NULL) {
			return CTB_EOOM;
		}
		array->capacity = v;
		array->buffer   = buffer;
	}

	return CTB_OK;
}

eintxx
dynarray_reserve(TDynArray* array, uintxx size)
{
	CTB_ASSERT(array);

	if (size >= array->capacity) {
		if (dynarray_resize(array, size)) {
			return CTB_EOOM;
		}
	}
	return CTB_OK;
}

eintxx
dynarray_shrink(TDynArray* array)
{
	uint8* buffer;
	uintxx v;
	CTB_ASSERT(array);

	v = array->used;
	if (v < DYNARRAY_MINSIZE)
		v = DYNARRAY_MINSIZE;

	if (array->capacity > v) {
		buffer = _realloc(array, array->buffer, v * array->datasize);
		if (buffer == NULL) {
			return CTB_EOOM;
		}

		array->capacity = v;
		array->buffer   = buffer;
	}
	return CTB_OK;
}
