#include <ruby.h>

void* malloc_or_die(size_t sz) {
    return malloc(sz);
}

void* realloc_or_die(void* p, size_t sz) {
    return realloc(p, sz);
}

FILE* fopen_or_die(const char* file, const char* mode) {
    // to do raise error
    return fopen(file, mode);
}
