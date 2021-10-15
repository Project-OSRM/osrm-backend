
#include <cstdlib>
#include <string>

#ifndef _WIN32
# include <unistd.h>
# include <fcntl.h>

// This function counts the number of open file descriptors. It is used in
// some tests to make sure that we are not leaking file descriptors.
inline int count_fds() noexcept {
    int count = 0;
    for (int fd = 0; fd < 100; ++fd) {
        if (fcntl(fd, F_GETFD) == 0) {
            ++count;
        }
    }
    return count;
}

#else
// Dummy for Windows which doesn't have fcntl
inline int count_fds() noexcept {
    return 0;
}
#endif


inline std::string with_data_dir(const char* filename) {
    const char* data_dir = getenv("OSMIUM_TEST_DATA_DIR");

    std::string result;
    if (data_dir) {
        result = data_dir;
        result += '/';
    }

    result += filename;

    return result;
}

