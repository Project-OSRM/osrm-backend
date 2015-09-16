#ifndef PROTOZERO_PBF_MESSAGE_HPP
#define PROTOZERO_PBF_MESSAGE_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

#include <type_traits>

#include <protozero/pbf_reader.hpp>
#include <protozero/pbf_types.hpp>

namespace protozero {

template <typename T>
class pbf_message : public pbf_reader {

    static_assert(std::is_same<pbf_tag_type, typename std::underlying_type<T>::type>::value, "T must be enum with underlying type protozero::pbf_tag_type");

public:

    using enum_type = T;

    template <typename... Args>
    pbf_message(Args&&... args) noexcept :
        pbf_reader(std::forward<Args>(args)...) {
    }

    inline bool next() {
        return pbf_reader::next();
    }

    inline bool next(T tag) {
        return pbf_reader::next(pbf_tag_type(tag));
    }

    inline T tag() const noexcept {
        return T(pbf_reader::tag());
    }

};

} // end namespace protozero

#endif // PROTOZERO_PBF_MESSAGE_HPP
