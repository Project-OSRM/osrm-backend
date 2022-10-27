
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestRepeatedPackedUInt64::Test msg;

    write_to_file(msg, "data-empty.pbf");

    msg.add_i(17ULL);
    write_to_file(msg, "data-one.pbf");

    msg.add_i(200UL);
    msg.add_i(0ULL);
    msg.add_i(1ULL);
    msg.add_i(std::numeric_limits<uint64_t>::max());
    write_to_file(msg, "data-many.pbf");
}

