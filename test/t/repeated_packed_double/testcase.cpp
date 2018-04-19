
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestRepeatedPackedDouble::Test msg;

    write_to_file(msg, "data-empty.pbf");

    msg.add_d(17.34);
    write_to_file(msg, "data-one.pbf");

    msg.add_d(0.0);
    msg.add_d(1.0);
    msg.add_d(std::numeric_limits<double>::min());
    msg.add_d(std::numeric_limits<double>::max());
    write_to_file(msg, "data-many.pbf");
}

