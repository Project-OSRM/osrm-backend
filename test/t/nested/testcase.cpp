
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestNested::Test msg;
    msg.set_i(77);

    TestNested::Sub* sub = msg.mutable_sub();

    write_to_file(msg, "data-no-message.pbf");

    sub->set_i(88);

    TestNested::SubSub* subsub = sub->mutable_subsub();
    subsub->set_s("foobar");
    subsub->set_i(99);

    write_to_file(msg, "data-message.pbf");
}

