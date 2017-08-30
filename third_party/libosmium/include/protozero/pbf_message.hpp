#ifndef PROTOZERO_PBF_MESSAGE_HPP
#define PROTOZERO_PBF_MESSAGE_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file pbf_message.hpp
 *
 * @brief Contains the pbf_message template class.
 */

#include <type_traits>

#include <protozero/pbf_reader.hpp>
#include <protozero/types.hpp>

namespace protozero {

/**
 * This class represents a protobuf message. Either a top-level message or
 * a nested sub-message. Top-level messages can be created from any buffer
 * with a pointer and length:
 *
 * @code
 *    enum class Message : protozero::pbf_tag_type {
 *       ...
 *    };
 *
 *    std::string buffer;
 *    // fill buffer...
 *    pbf_message<Message> message(buffer.data(), buffer.size());
 * @endcode
 *
 * Sub-messages are created using get_message():
 *
 * @code
 *    enum class SubMessage : protozero::pbf_tag_type {
 *       ...
 *    };
 *
 *    pbf_message<Message> message(...);
 *    message.next();
 *    pbf_message<SubMessage> submessage = message.get_message();
 * @endcode
 *
 * All methods of the pbf_message class except get_bytes() and get_string()
 * provide the strong exception guarantee, ie they either succeed or do not
 * change the pbf_message object they are called on. Use the get_data() method
 * instead of get_bytes() or get_string(), if you need this guarantee.
 *
 * This template class is based on the pbf_reader class and has all the same
 * methods. The difference is that whereever the pbf_reader class takes an
 * integer tag, this template class takes a tag of the template type T.
 *
 * Read the tutorial to understand how this class is used.
 */
template <typename T>
class pbf_message : public pbf_reader {

    static_assert(std::is_same<pbf_tag_type, typename std::underlying_type<T>::type>::value, "T must be enum with underlying type protozero::pbf_tag_type");

public:

    using enum_type = T;

    template <typename... Args>
    pbf_message(Args&&... args) noexcept :
        pbf_reader(std::forward<Args>(args)...) {
    }

    bool next() {
        return pbf_reader::next();
    }

    bool next(T next_tag) {
        return pbf_reader::next(pbf_tag_type(next_tag));
    }

    bool next(T next_tag, pbf_wire_type type) {
        return pbf_reader::next(pbf_tag_type(next_tag), type);
    }

    T tag() const noexcept {
        return T(pbf_reader::tag());
    }

}; // class pbf_message

} // end namespace protozero

#endif // PROTOZERO_PBF_MESSAGE_HPP
