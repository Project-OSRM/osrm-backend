
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestComplex::Test msg;

    msg.set_f(12345678);
    TestComplex::Sub* submsg = msg.mutable_submessage();
    submsg->set_s("foobar");

    write_to_file(msg, "data-minimal.pbf");

    msg.add_u(22);
    msg.add_u(44);
    msg.set_i(-9876543);

    write_to_file(msg, "data-some.pbf");

    msg.set_s("optionalstring");
    msg.add_u(44);
    msg.add_u(66);
    msg.add_u(66);
    msg.add_d(-17);
    msg.add_d(22);
    msg.set_j(555555555);

    write_to_file(msg, "data-all.pbf");
}

