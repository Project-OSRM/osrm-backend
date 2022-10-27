
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestFixed64::Test msg;

    msg.set_i(0);
    write_to_file(msg, "data-zero.pbf");

    msg.set_i(1);
    write_to_file(msg, "data-pos.pbf");

    msg.set_i(200);
    write_to_file(msg, "data-pos200.pbf");

    msg.set_i(std::numeric_limits<uint64_t>::max());
    write_to_file(msg, "data-max.pbf");
}

