// based on
// https://svn.apache.org/repos/asf/mesos/tags/release-0.9.0-incubating-RC0/src/common/json.hpp

#ifndef JSON_RENDERER_HPP
#define JSON_RENDERER_HPP

#include "util/cast.hpp"
#include "util/ieee754.hpp"
#include "util/string_util.hpp"

#include "osrm/json_container.hpp"

#include <iterator>
#include <ostream>
#include <string>
#include <vector>

namespace osrm
{
namespace util
{
namespace json
{

namespace
{
constexpr int MAX_FLOAT_STRING_LENGTH = 256;
}

template <typename Out> struct Renderer
{
    explicit Renderer(Out &_out) : out(_out) {}

    void operator()(const String &string)
    {
        write('"');
        auto size = SizeOfEscapedJSONString(string.value);
        if (size == string.value.size()) {
            // we don't need to escape anything
            write(string.value);
        } else {
            std::string escaped;
            escaped.reserve(size);
            EscapeJSONString(string.value, escaped);

            write(escaped);
        }
        write('"');
    }

    void operator()(const Number &number)
    {
        char buffer[MAX_FLOAT_STRING_LENGTH] = {'\0'};
        ieee754::dtoa_milo(number.value, buffer);

        // Trucate to 10 decimal places
        int pos = 0;
        int decimalpos = 0;
        while (decimalpos == 0 && pos < MAX_FLOAT_STRING_LENGTH && buffer[pos] != 0)
        {
            if (buffer[pos] == '.')
            {
                decimalpos = pos;
                break;
            }
            ++pos;
        }
        while (pos < MAX_FLOAT_STRING_LENGTH && buffer[pos] != 0)
        {
            if (pos - decimalpos == 10)
            {
                buffer[pos] = '\0';
                break;
            }
            ++pos;
        }
        write(buffer);
    }

    void operator()(const Object &object)
    {
        write('{');
        for (auto it = object.values.begin(), end = object.values.end(); it != end;)
        {
            write('\"');
            write(it->first);
            write("\"");
            mapbox::util::apply_visitor(Renderer(out), it->second);
            if (++it != end)
            {
                write(',');
            }
        }
        write('}');
    }

    void operator()(const Array &array)
    {
        write('[');
        for (auto it = array.values.cbegin(), end = array.values.cend(); it != end;)
        {
            mapbox::util::apply_visitor(Renderer(out), *it);
            if (++it != end)
            {
                write(',');
            }
        }
        write(']');
    }

    void operator()(const True &) { write("true"); }

    void operator()(const False &) { write("false"); }

    void operator()(const Null &) { write("null"); }

  private:
    void write(const std::string &str);
    void write(const char *str);
    void write(char ch);

  private:
    Out &out;
};

template <> void Renderer<std::vector<char>>::write(const std::string &str)
{
    out.insert(out.end(), str.begin(), str.end());
}

template <> void Renderer<std::vector<char>>::write(const char *str)
{
    out.insert(out.end(), str, str + strlen(str));
}
template <> void Renderer<std::vector<char>>::write(char ch) { out.push_back(ch); }


template <> void Renderer<std::ostream>::write(const std::string &str) { out << str; }

template <> void Renderer<std::ostream>::write(const char *str) { out << str; }

template <> void Renderer<std::ostream>::write(char ch) { out << ch; }


template <> void Renderer<std::string>::write(const std::string &str) { out += str; }

template <> void Renderer<std::string>::write(const char *str) { out += str; }

template <> void Renderer<std::string>::write(char ch) { out += ch; }


inline void render(std::ostream &out, const Object &object)
{
    Value value = object;
    mapbox::util::apply_visitor(Renderer(out), value);
}

inline void render(std::string &out, const Object &object)
{
    Value value = object;
    mapbox::util::apply_visitor(Renderer(out), value);
}

inline void render(std::vector<char> &out, const Object &object)
{
    Value value = object;
    mapbox::util::apply_visitor(Renderer(out), value);
}

} // namespace json
} // namespace util
} // namespace osrm

#endif // JSON_RENDERER_HPP
