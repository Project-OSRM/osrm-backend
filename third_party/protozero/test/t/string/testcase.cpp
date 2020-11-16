
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestString::Test msg;

    msg.set_s("");
    write_to_file(msg, "data-empty.pbf");

    msg.set_s("x");
    write_to_file(msg, "data-one.pbf");

    msg.set_s("foobar");
    write_to_file(msg, "data-string.pbf");
}

