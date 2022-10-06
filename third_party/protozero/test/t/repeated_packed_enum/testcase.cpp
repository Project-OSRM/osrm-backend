
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {
    TestRepeatedPackedEnum::Test msg;

    write_to_file(msg, "data-empty.pbf");

    msg.add_color(TestRepeatedPackedEnum::BLACK);
    write_to_file(msg, "data-one.pbf");

    msg.add_color(TestRepeatedPackedEnum::BLUE);
    msg.add_color(TestRepeatedPackedEnum::GREEN);
    write_to_file(msg, "data-many.pbf");
}

