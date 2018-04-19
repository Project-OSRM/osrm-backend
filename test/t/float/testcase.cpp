
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestFloat::Test msg;

    msg.set_x(0.0);
    write_to_file(msg, "data-zero.pbf");

    msg.set_x(5.34);
    write_to_file(msg, "data-pos.pbf");

    msg.set_x(-1.71);
    write_to_file(msg, "data-neg.pbf");
}

