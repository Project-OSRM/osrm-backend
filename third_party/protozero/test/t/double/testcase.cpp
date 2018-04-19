
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestDouble::Test msg;

    msg.set_x(0.0);
    write_to_file(msg, "data-zero.pbf");

    msg.set_x(4.893);
    write_to_file(msg, "data-pos.pbf");

    msg.set_x(-9232.33);
    write_to_file(msg, "data-neg.pbf");
}

