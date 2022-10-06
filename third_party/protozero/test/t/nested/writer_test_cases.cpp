
#include <buffer.hpp>

#include "t/nested/nested_testcase.pb.h"

TEMPLATE_TEST_CASE("write nested message fields and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    SECTION("string") {
        std::string buffer_subsub;
        protozero::pbf_writer pbf_subsub{buffer_subsub};
        pbf_subsub.add_string(1, "foobar");
        pbf_subsub.add_int32(2, 99);

        std::string buffer_sub;
        protozero::pbf_writer pbf_sub{buffer_sub};
        pbf_sub.add_string(1, buffer_subsub);
        pbf_sub.add_int32(2, 88);

        pw.add_message(1, buffer_sub);
    }

    SECTION("with subwriter") {
        typename TestType::writer_type pbf_sub{pw, 1};
        {
            typename TestType::writer_type pbf_subsub(pbf_sub, 1);
            pbf_subsub.add_string(1, "foobar");
            pbf_subsub.add_int32(2, 99);
        }
        pbf_sub.add_int32(2, 88);
    }

    pw.add_int32(2, 77);

    TestNested::Test msg;
    msg.ParseFromArray(buffer.data(), buffer.size());

    REQUIRE(msg.i() == 77);
    REQUIRE(msg.sub().i() == 88);
    REQUIRE(msg.sub().subsub().i() == 99);
    REQUIRE(msg.sub().subsub().s() == "foobar");

}

