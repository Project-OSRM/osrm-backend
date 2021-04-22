
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestRepeatedPackedFloat::Test msg;

    write_to_file(msg, "data-empty.pbf");

    msg.add_f(17.34f);
    write_to_file(msg, "data-one.pbf");

    msg.add_f(0.0f);
    msg.add_f(1.0f);
    msg.add_f(std::numeric_limits<float>::min());
    msg.add_f(std::numeric_limits<float>::max());
    write_to_file(msg, "data-many.pbf");
}

