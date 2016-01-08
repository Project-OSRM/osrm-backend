#ifndef OBJECT_ENCODER_HPP
#define OBJECT_ENCODER_HPP

#include <boost/assert.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{

struct ObjectEncoder
{
    using base64_t = boost::archive::iterators::base64_from_binary<
        boost::archive::iterators::transform_width<const char *, 6, 8>>;

    using binary_t = boost::archive::iterators::transform_width<
        boost::archive::iterators::binary_from_base64<std::string::const_iterator>,
        8,
        6>;

    template <class ObjectT> static void EncodeToBase64(const ObjectT &object, std::string &encoded)
    {
        const char *char_ptr_to_object = reinterpret_cast<const char *>(&object);
        std::vector<unsigned char> data(sizeof(object));
        std::copy(char_ptr_to_object, char_ptr_to_object + sizeof(ObjectT), data.begin());

        unsigned char number_of_padded_chars = 0; // is in {0,1,2};
        while (data.size() % 3 != 0)
        {
            ++number_of_padded_chars;
            data.push_back(0x00);
        }

        BOOST_ASSERT_MSG(0 == data.size() % 3, "base64 input data size is not a multiple of 3!");
        encoded.resize(sizeof(ObjectT));
        encoded.assign(base64_t(&data[0]),
                       base64_t(&data[0] + (data.size() - number_of_padded_chars)));
        std::replace(begin(encoded), end(encoded), '+', '-');
        std::replace(begin(encoded), end(encoded), '/', '_');
    }

    template <class ObjectT> static void DecodeFromBase64(const std::string &input, ObjectT &object)
    {
        try
        {
            std::string encoded(input);
            std::replace(begin(encoded), end(encoded), '-', '+');
            std::replace(begin(encoded), end(encoded), '_', '/');

            std::copy(binary_t(encoded.begin()), binary_t(encoded.begin() + encoded.length()),
                      reinterpret_cast<char *>(&object));
        }
        catch (...)
        {
        }
    }
};
}
}

#endif /* OBJECT_ENCODER_HPP */
