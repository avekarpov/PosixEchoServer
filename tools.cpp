#include "tools.hpp"

#include <unistd.h>
#include <ctype.h>

size_t readFull(const int fd, char* buffer, const size_t count_to_read) {
    size_t read_count = 0;
    while (read_count < count_to_read) {
        const auto chunk_size = read(fd, buffer + read_count, count_to_read - read_count);
        CHECK_THROW_POSIX(chunk_size);

        if (0 == chunk_size) {
            break;
        }

        read_count += chunk_size;
    }

    return read_count;
}

size_t writeFull(const int fd, const char* data, const size_t count_to_write) {
    size_t written_count = 0;
    while (written_count < count_to_write) {
        const auto chunk_written_count = ::write(fd, &data[written_count], count_to_write - written_count);
        CHECK_THROW_POSIX(chunk_written_count);

        if (0 == chunk_written_count) {
            break;
        }

        written_count += chunk_written_count;
    }

    return written_count;
}

void toupper(char* buffer, const size_t size) {
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = toupper(buffer[i]);
    }
}
