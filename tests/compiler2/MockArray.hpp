#include <cstdio>
#include <cassert>
#include <inttypes.h>
#include <vector>
#include <algorithm>
#include <iostream>

#include "vm/global.hpp"


template <typename T>
struct array;

template <>
struct array<int64_t> {
	typedef java_longarray_t type;
};
template <>
struct array<int32_t> {
	typedef java_intarray_t type;
};


template<typename T>
class MockArray {
	typedef typename array<T>::type type;
public:
	explicit MockArray(std::size_t len) {
		std::size_t size = sizeof(java_array_t) + len * sizeof(T);
		data = new uint8_t[size];
		type *a = (type*)data;
		a->header.size = len;
	}
	std::size_t size() {
		type *a = (type*)data;
		return a->header.size;
	}
	void* raw() {
		return data;
	}
	T& operator[](std::size_t index) {
		assert(index < size());
		T *a = (T*)(data+sizeof(java_array_t));
		return a[index];
	}
	operator void*(){
		return data;
	}
	~MockArray() {
		delete[] data;
	}
private:
	// prevent copying
	MockArray(const MockArray&);
	MockArray& operator=(const MockArray&);
	uint8_t *data;
};

template<typename T>
void deleter(T t) {
	delete t;
}

template<typename T>
class MockArray2dim {
	typedef typename array<T>::type type;
public:
	explicit MockArray2dim(std::size_t rows, std::size_t columns, T* init = NULL) : inner(rows) {
		std::size_t size  = sizeof(java_array_t) + rows * sizeof(java_objectarray_t*);
		data = new uint8_t[size];
		java_objectarray_t *a = (java_objectarray_t*)data;
		a->header.size = rows;
		for (std::size_t i = 0; i < rows; ++i) {
			MockArray<T> *row = new MockArray<T>(columns);
			inner[i] = row;
			a->data[i] = (java_object_t*)row->raw();
		}
		if (init)
			for (std::size_t i = 0; i < rows; ++i)
				for (std::size_t j = 0; j < columns; ++j)
					(*this)[i][j] = *(init + i * columns +j);
	}
	std::size_t size() {
		type *a = (type*)data;
		return a->header.size;
	}
	void* raw() {
		return data;
	}
	MockArray<T>& operator[](std::size_t index) {
		assert(index < size());
		return *inner[index];
	}
	operator void*(){
		return data;
	}
	~MockArray2dim() {
		std::for_each(inner.begin(),inner.end(),deleter<MockArray<T>*>);
		delete[] data;
	}
private:
	// prevent copying
	MockArray2dim(const MockArray2dim&);
	MockArray2dim& operator=(const MockArray2dim&);
	uint8_t *data;
	std::vector<MockArray<T>*> inner;
};

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
