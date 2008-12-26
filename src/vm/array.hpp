/* src/vm/array.hpp - Java array functions

   Copyright (C) 2007
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2008 Theobroma Systems Ltd.

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#ifndef _VM_ARRAY_HPP
#define _VM_ARRAY_HPP

#include "config.h"

#include <stdint.h>

#include "mm/gc.hpp" // XXX Remove me!

#include "native/llni.h" // XXX Remove me!

#include "vm/class.hpp"
#include "vm/exceptions.hpp"
#include "vm/global.h"
#include "vm/primitive.hpp"


/* array types ****************************************************************/

/* CAUTION: Don't change the numerical values! These constants (with
   the exception of ARRAYTYPE_OBJECT) are used as indices in the
   primitive type table. */

#define ARRAYTYPE_INT         PRIMITIVETYPE_INT
#define ARRAYTYPE_LONG        PRIMITIVETYPE_LONG
#define ARRAYTYPE_FLOAT       PRIMITIVETYPE_FLOAT
#define ARRAYTYPE_DOUBLE      PRIMITIVETYPE_DOUBLE
#define ARRAYTYPE_BYTE        PRIMITIVETYPE_BYTE
#define ARRAYTYPE_CHAR        PRIMITIVETYPE_CHAR
#define ARRAYTYPE_SHORT       PRIMITIVETYPE_SHORT
#define ARRAYTYPE_BOOLEAN     PRIMITIVETYPE_BOOLEAN
#define ARRAYTYPE_OBJECT      PRIMITIVETYPE_VOID     /* don't use as index! */


#ifdef __cplusplus

/**
 * This is a generic accessor class for Java arrays (of unspecified type),
 * which can be used to safely operate on Java arrays in native code.
 */
class Array {
protected:
	// Handle of Java array.
	java_handle_array_t* _handle;

private:
	// We don't want a Java arrays to be copied.
	Array(Array* a) {}
	Array(Array& a) {}

public:
	inline Array(java_handle_t* h);
	inline Array(int32_t length, classinfo* arrayclass);
	virtual ~Array() {}

	// Getters.
	virtual inline java_handle_array_t* get_handle() const { return _handle; }
	inline int32_t                      get_length() const;

	inline bool is_null    () const;
	inline bool is_non_null() const;

	// Safe element modification functions for primitive values
	imm_union get_primitive_element(int32_t index);
	void      set_primitive_element(int32_t index, imm_union value);

	// Safe element modification functions for boxed values
	java_handle_t* get_boxed_element(int32_t index);
	void           set_boxed_element(int32_t index, java_handle_t *o);

	// XXX REMOVE ME!
	inline void* get_raw_data_ptr() { return ((java_objectarray_t*) _handle)->data; }
};


/**
 * Constructor checks if passed handle really is a Java array.
 */
inline Array::Array(java_handle_t* h)
{
	if (h == NULL) {
		_handle = NULL;
		return;
	}

#if 0
	classinfo* c;
	LLNI_class_get(h, c);
	if (!class_is_array(c)) {
		printf("Array::Array(): WARNING, passed handle is not an array\n");
 		//exceptions_throw_illegalargumentexception("Argument is not an array");
		exceptions_throw_illegalargumentexception();
		_handle = NULL;
		return;
	}
#endif

	_handle = h;
}

/**
 * Creates an array of the given array type on the heap.
 * The handle pointer to the array can be NULL in case of an exception.
 */
inline Array::Array(int32_t size, classinfo* arrayclass)
{
	// Sanity check.
	assert(class_is_array(arrayclass));

	if (size < 0) {
		exceptions_throw_negativearraysizeexception();
		_handle = NULL;
		return;
	}

	arraydescriptor* desc          = arrayclass->vftbl->arraydesc;
	int32_t          dataoffset    = desc->dataoffset;
	int32_t          componentsize = desc->componentsize;
	int32_t          actualsize    = dataoffset + size * componentsize;

	// Check for overflow.

	if (((u4) actualsize) < ((u4) size)) {
		exceptions_throw_outofmemoryerror();
		_handle = NULL;
		return;
	}

	java_array_t* a = (java_array_t*) heap_alloc(actualsize, (desc->arraytype == ARRAYTYPE_OBJECT), NULL, true);

	if (a == NULL) {
		_handle = NULL;
		return;
	}

	LLNI_vftbl_direct(a) = arrayclass->vftbl;

#if defined(ENABLE_THREADS)
	a->objheader.lockword.init();
#endif

	a->size = size;

	_handle = (java_handle_array_t*) a;
}

inline int32_t Array::get_length() const
{
	if (is_null()) {
		printf("Array::get_length(): WARNING, got null-pointer\n");
		exceptions_throw_nullpointerexception();
		return -1;
	}

	// XXX Fix me!
	int32_t length = ((java_array_t*) _handle)->size;

	return length;
}

inline bool Array::is_null() const
{
	return (_handle == NULL);
}

inline bool Array::is_non_null() const
{
	return (_handle != NULL);
}


/**
 * This is a template of an accessor class for Java arrays
 * of a specific type.
 */
template<class T> class ArrayTemplate : public Array {
protected:
	ArrayTemplate(int32_t length, classinfo* arrayclass) : Array(length, arrayclass) {}

public:
	inline ArrayTemplate(java_handle_array_t* h) : Array(h) {}

	// Safe element modification functions
	inline T    get_element(int32_t index);
	inline void set_element(int32_t index, T value);

	// Region copy functions
	inline void get_region(int32_t offset, int32_t count, T* buffer);
	inline void set_region(int32_t offset, int32_t count, const T* buffer);
};


template<class T> inline T ArrayTemplate<T>::get_element(int32_t index)
{
	if (is_null()) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	if ((index < 0) || (index >= get_length())) {
		exceptions_throw_arrayindexoutofboundsexception();
		return 0;
	}

	// XXX Fix me!
	T* ptr = (T*) get_raw_data_ptr();

	return ptr[index];
}

template<class T> inline void ArrayTemplate<T>::set_element(int32_t index, T value)
{
	if (is_null()) {
		exceptions_throw_nullpointerexception();
		return;
	}

	if ((index < 0) || (index >= get_length())) {
		exceptions_throw_arrayindexoutofboundsexception();
		return;
	}

	// XXX Fix me!
	T* ptr = (T*) get_raw_data_ptr();

	ptr[index] = value;
}

template<> inline void ArrayTemplate<java_handle_t*>::set_element(int32_t index, java_handle_t* value)
{
	if (is_null()) {
		exceptions_throw_nullpointerexception();
		return;
	}

	// Sanity check.
	assert(((java_array_t*) _handle)->objheader.vftbl->arraydesc->arraytype == ARRAYTYPE_OBJECT);

	// Check if value can be stored
	if (value != NULL) {
		if (builtin_canstore(get_handle(), value) == false) {
			exceptions_throw_illegalargumentexception();
			return;
		}
	}

	if ((index < 0) || (index >= get_length())) {
		exceptions_throw_arrayindexoutofboundsexception();
		return;
	}

	// XXX Fix me!
	java_handle_t** ptr = (java_handle_t**) get_raw_data_ptr();

	ptr[index] = value;
}

template<class T> inline void ArrayTemplate<T>::get_region(int32_t offset, int32_t count, T* buffer)
{
	// Copy the array region inside a GC critical section.
	GCCriticalSection cs;

	// XXX Fix me!
	const T* ptr = (T*) get_raw_data_ptr();

	os::memcpy(buffer, ptr + offset, sizeof(T) * count);
}

template<class T> inline void ArrayTemplate<T>::set_region(int32_t offset, int32_t count, const T* buffer)
{
	// Copy the array region inside a GC critical section.
	GCCriticalSection cs;

	// XXX Fix me!
	T* ptr = (T*) get_raw_data_ptr();

	os::memcpy(ptr + offset, buffer, sizeof(T) * count);
}


/**
 * Actual implementations of common Java array access classes.
 */
class BooleanArray : public ArrayTemplate<uint8_t> {
public:
	inline BooleanArray(java_handle_booleanarray_t* h) : ArrayTemplate<uint8_t>(h) {}
	inline BooleanArray(int32_t length) : ArrayTemplate<uint8_t>(length, primitivetype_table[ARRAYTYPE_BOOLEAN].arrayclass) {}
};

class ByteArray : public ArrayTemplate<int8_t> {
public:
	inline ByteArray(java_handle_bytearray_t* h) : ArrayTemplate<int8_t>(h) {}
	inline ByteArray(int32_t length) : ArrayTemplate<int8_t>(length, primitivetype_table[ARRAYTYPE_BYTE].arrayclass) {}
};

class CharArray : public ArrayTemplate<uint16_t> {
public:
	inline CharArray(java_handle_chararray_t* h) : ArrayTemplate<uint16_t>(h) {}
	inline CharArray(int32_t length) : ArrayTemplate<uint16_t>(length, primitivetype_table[ARRAYTYPE_CHAR].arrayclass) {}
};

class ShortArray : public ArrayTemplate<int16_t> {
public:
	inline ShortArray(java_handle_shortarray_t* h) : ArrayTemplate<int16_t>(h) {}
	inline ShortArray(int32_t length) : ArrayTemplate<int16_t>(length, primitivetype_table[ARRAYTYPE_SHORT].arrayclass) {}
};

class IntArray : public ArrayTemplate<int32_t> {
public:
	inline IntArray(java_handle_intarray_t* h) : ArrayTemplate<int32_t>(h) {}
	inline IntArray(int32_t length) : ArrayTemplate<int32_t>(length, primitivetype_table[ARRAYTYPE_INT].arrayclass) {}
};

class LongArray : public ArrayTemplate<int64_t> {
public:
	inline LongArray(java_handle_longarray_t* h) : ArrayTemplate<int64_t>(h) {}
	inline LongArray(int32_t length) : ArrayTemplate<int64_t>(length, primitivetype_table[ARRAYTYPE_LONG].arrayclass) {}
};

class FloatArray : public ArrayTemplate<float> {
public:
	inline FloatArray(java_handle_floatarray_t* h) : ArrayTemplate<float>(h) {}
	inline FloatArray(int32_t length) : ArrayTemplate<float>(length, primitivetype_table[ARRAYTYPE_FLOAT].arrayclass) {}
};

class DoubleArray : public ArrayTemplate<double> {
public:
	inline DoubleArray(java_handle_doublearray_t* h) : ArrayTemplate<double>(h) {}
	inline DoubleArray(int32_t length) : ArrayTemplate<double>(length, primitivetype_table[ARRAYTYPE_DOUBLE].arrayclass) {}
};

/**
 * Actual implementation of access class for Java Object arrays.
 */
class ObjectArray : public ArrayTemplate<java_handle_t*> {
public:
	inline ObjectArray(java_handle_objectarray_t* h) : ArrayTemplate<java_handle_t*>(h) {}
	ObjectArray(int32_t length, classinfo* componentclass);
};

/**
 * Actual implementation of access class for java.lang.Class arrays.
 */
class ClassArray : public ArrayTemplate<classinfo*> {
public:
	ClassArray(int32_t length);
};


#else
# warning No legacy C functions for array access classes.
#endif

#endif // _VM_ARRAY_HPP


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
