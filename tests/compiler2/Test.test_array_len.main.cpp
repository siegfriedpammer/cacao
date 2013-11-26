#include <cstdio>
#include <inttypes.h>

#include "vm/global.hpp"

extern "C" int32_t test_array_len(void*);

#define SIZE 10

template <typename T>
struct array;

template <>
struct array<long> {
    typedef java_longarray_t type;
};

template<typename T>
class ArrayTest {
    typedef typename array<T>::type type;
public:
    explicit ArrayTest(std::size_t len) {
        data = new uint8_t[sizeof(java_array_t) + SIZE * sizeof(T)];
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
    ~ArrayTest() {
        delete[] data;
    }
private:
    uint8_t *data;
};

int main() {
    printf("%d\n", test_array_len(ArrayTest<long>(10)));
    printf("%d\n", test_array_len(ArrayTest<long>(1)));
    printf("%d\n", test_array_len(ArrayTest<long>(0)));
    printf("%d\n", test_array_len(ArrayTest<long>(42)));
    return 0;
}
