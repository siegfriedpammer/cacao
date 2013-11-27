#include <cstdio>
#include <cassert>
#include <inttypes.h>
#include <vector>
#include <algorithm>

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
        data = new uint8_t[sizeof(java_array_t) + len * sizeof(T)];
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
    explicit MockArray2dim(std::size_t rows, std::size_t columns) : inner(columns) {
        data = new uint8_t[sizeof(java_array_t) + rows * sizeof(java_objectarray_t*)];
        java_objectarray_t *a = (java_objectarray_t*)data;
        a->header.size = rows;
		for (std::size_t i = 0; i < rows; ++i) {
			MockArray<T> *row = new MockArray<T>(columns);
			inner[i] = row;
			a->data[i] = (java_object_t*)row->raw();
		}
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
        delete[] data;
		std::for_each(inner.begin(),inner.end(),deleter<MockArray<T>*>);
    }
private:
	// prevent copying
	MockArray2dim(const MockArray2dim&);
	MockArray2dim& operator=(const MockArray2dim&);
    uint8_t *data;
	std::vector<MockArray<T>*> inner;
};

