#include <cstdio>
#include <inttypes.h>

#include "vm/global.hpp"


template <typename T>
struct array;

template <>
struct array<long> {
    typedef java_longarray_t type;
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
    operator void*(){
        return data;
    }
    ~MockArray() {
        delete[] data;
    }
private:
    uint8_t *data;
};

