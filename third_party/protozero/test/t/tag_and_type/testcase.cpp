
#include <testcase.hpp>

#include "testcase.pb.h"

int main() {

    std::string out;

    {
        TestTagAndType::TestNotPacked msg;
        for (uint32_t x = 10; x < 13; ++x) {
            msg.add_x(x);
        }

        out.append(write_to_file(msg, "data-not-packed.pbf"));
    }

    {
        TestTagAndType::TestPacked msg;
        for (uint32_t x = 20; x < 23; ++x) {
            msg.add_x(x);
        }

        out.append(write_to_file(msg, "data-packed.pbf"));
    }

    {
        TestTagAndType::TestNotPacked msg;
        for (uint32_t x = 30; x < 33; ++x) {
            msg.add_x(x);
        }

        std::string mout;
        msg.SerializeToString(&mout);
        out.append(mout);
    }

    std::ofstream d("data-combined.pbf", std::ios_base::out|std::ios_base::binary);
    assert(d.is_open());
    d << out;
}

