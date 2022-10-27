
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestBoolean::Test msg;

    msg.set_b(0);
    write_to_file(msg, "data-false.pbf");

    msg.set_b(1);
    write_to_file(msg, "data-true.pbf");

    msg.set_b(2);
    write_to_file(msg, "data-also-true.pbf");

    msg.set_b(2000);
    write_to_file(msg, "data-still-true.pbf");
}

