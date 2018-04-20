
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestEnum::Test msg;

    msg.set_color(TestEnum::BLACK);
    write_to_file(msg, "data-black.pbf");

    msg.set_color(TestEnum::BLUE);
    write_to_file(msg, "data-blue.pbf");

    msg.set_color(TestEnum::NEG);
    write_to_file(msg, "data-neg.pbf");

    msg.set_color(TestEnum::MAX);
    write_to_file(msg, "data-max.pbf");

    msg.set_color(TestEnum::MIN);
    write_to_file(msg, "data-min.pbf");
}

