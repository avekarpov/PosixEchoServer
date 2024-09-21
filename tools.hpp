#pragma once

#include <errno.h>
#include <string.h>

#include <stdexcept>
#include <functional>

#define CHECK_THROW_POSIX(__VALUE__) \
do { \
    if ((__VALUE__) == -1) { \
        throw std::runtime_error { strerror(errno) }; \
    } \
} \
while (false)

size_t readFull(const int fd, char* buffer, const size_t count_to_read);

size_t writeFull(const int fd, const char* data, const size_t count_to_write);

void toupper(char* buffer, const size_t size);

class Defer {
public:
    template <class F>
    Defer(F function)
        : _function { std::move(function) }
    {}
    ~Defer() {
        _function();
    }
private:
    std::function<void()> _function;
};

#define COMBINE_HELPER(__X__, __Y__) __X__##__Y__
#define COMBINE(__X__, __Y__) COMBINE_HELPER(__X__, __Y__)

#define ADD_LINE(__X__) COMBINE(__X__, __LINE__)

#define DEFER Defer ADD_LINE(deref) = [&]() // { function body; }
