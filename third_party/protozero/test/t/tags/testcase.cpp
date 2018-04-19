
#include <testcase.hpp>
#include "testcase.pb.h"

int main(int c, char *argv[]) {

    {
        TestTags::Test1 msg;
        msg.set_i(333);
        write_to_file(msg, "data-tag-1.pbf");
    }

    {
        TestTags::Test200 msg;
        msg.set_i(333);
        write_to_file(msg, "data-tag-200.pbf");
    }

    {
        TestTags::Test200000 msg;
        msg.set_i(333);
        write_to_file(msg, "data-tag-200000.pbf");
    }

    {
        TestTags::TestMax msg;
        msg.set_i(333);
        write_to_file(msg, "data-tag-max.pbf");
    }

}

