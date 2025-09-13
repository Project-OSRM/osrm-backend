
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestRepeatedPackedFixed32::Test msg;

    write_to_file(msg, "data-empty.pbf");

    msg.add_i(17UL);
    write_to_file(msg, "data-one.pbf");

    msg.add_i(200UL);
    msg.add_i(0UL);
    msg.add_i(1UL);
    msg.add_i(std::numeric_limits<uint32_t>::max());
    write_to_file(msg, "data-many.pbf");
}

