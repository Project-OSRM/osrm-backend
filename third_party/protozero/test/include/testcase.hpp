#ifndef TESTCASE_HPP
#define TESTCASE_HPP

#include <cassert>
#include <fstream>
#include <limits>
#include <string>

template <class T>
std::string write_to_file(const T& msg, const char* filename) {
    std::string out;

    msg.SerializeToString(&out);
    std::ofstream d{filename, std::ios_base::out|std::ios_base::binary};
    assert(d.is_open());
    d << out;

    return out;
}

#endif // TESTCASE_HPP
