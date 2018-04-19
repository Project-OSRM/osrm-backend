
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {
    TestBytes::Test msg;

    msg.set_s("");
    write_to_file(msg, "data-empty.pbf");

    msg.set_s("x");
    write_to_file(msg, "data-one.pbf");

    msg.set_s("foobar");
    write_to_file(msg, "data-string.pbf");

    std::string data;
    data.append(1, char(1));
    data.append(1, char(2));
    data.append(1, char(3));
    msg.set_s(data);
    write_to_file(msg, "data-binary.pbf");
}

