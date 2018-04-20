
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestMessage::Test msg;

    TestMessage::Sub* submsg = msg.mutable_submessage();
    submsg->set_s("foobar");

    write_to_file(msg, "data-message.pbf");

    TestMessage::Opt opt_msg;
    write_to_file(opt_msg, "data-opt-empty.pbf");

    opt_msg.set_s("optional");
    write_to_file(opt_msg, "data-opt-element.pbf");
}

