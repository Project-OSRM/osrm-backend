
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestRepeated::Test msg;

    write_to_file(msg, "data-empty.pbf");

    msg.add_i(0);
    write_to_file(msg, "data-one.pbf");

    msg.add_i(1);
    msg.add_i(-1);
    msg.add_i(std::numeric_limits<int32_t>::max());
    msg.add_i(std::numeric_limits<int32_t>::min());
    write_to_file(msg, "data-many.pbf");
}

