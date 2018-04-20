
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestSInt32::Test msg;

    msg.set_i(0L);
    write_to_file(msg, "data-zero.pbf");

    msg.set_i(1L);
    write_to_file(msg, "data-pos.pbf");

    msg.set_i(200);
    write_to_file(msg, "data-pos200.pbf");

    msg.set_i(-1);
    write_to_file(msg, "data-neg.pbf");

    msg.set_i(-200);
    write_to_file(msg, "data-neg200.pbf");

    msg.set_i(std::numeric_limits<int32_t>::max());
    write_to_file(msg, "data-max.pbf");

    msg.set_i(std::numeric_limits<int32_t>::min());
    write_to_file(msg, "data-min.pbf");
}

