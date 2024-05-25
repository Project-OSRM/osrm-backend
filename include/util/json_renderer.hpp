// based on
// https://svn.apache.org/repos/asf/mesos/tags/release-0.9.0-incubating-RC0/src/common/json.hpp

#ifndef JSON_RENDERER_HPP
#define JSON_RENDERER_HPP

#include "util/string_util.hpp"

#include "osrm/json_container.hpp"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

#include <boost/assert.hpp>

#include <fmt/compile.h>

namespace osrm::util::json
{

template <typename Out> struct Renderer
{
    explicit Renderer(Out &_out) : out(_out) {}

    void operator()(const String &string)
    {
        write('"');
        // here we assume that vast majority of strings don't need to be escaped,
        // so we check it first and escape only if needed
        if (RequiresJSONStringEscaping(string.value))
        {
            std::string escaped;
            // just a guess that 16 bytes for escaped characters will be enough to avoid
            // reallocations
            escaped.reserve(string.value.size() + 16);
            EscapeJSONString(string.value, escaped);

            write(escaped);
        }
        else
        {
            write(string.value);
        }
        write('"');
    }

    void operator()(const Number &number)
    {
        // we don't want to print NaN or Infinity
        BOOST_ASSERT(std::isfinite(number.value));
        // `fmt::memory_buffer` stores first 500 bytes in the object itself(i.e. on stack in this
        // case) and then grows using heap if needed
        fmt::memory_buffer buffer;
        fmt::format_to(std::back_inserter(buffer), FMT_COMPILE("{:.10g}"), number.value);

        write(buffer.data(), buffer.size());
    }

    void operator()(const Object &object)
    {
        write('{');
        for (auto it = object.values.begin(), end = object.values.end(); it != end;)
        {
            write('\"');
            write(it->first);
            write<>("\":");
            std::visit(Renderer(out), it->second);
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
            std::visit(Renderer(out), *it);
            if (++it != end)
            {
                write(',');
            }
        }
        write(']');
    }

    void operator()(const True &) { write<>("true"); }

    void operator()(const False &) { write<>("false"); }

    void operator()(const Null &) { write<>("null"); }

  private:
    void write(const std::string &str);
    void write(const char *str, size_t size);
    void write(char ch);

    template <size_t StrLength> void write(const char (&str)[StrLength])
    {
        write(str, StrLength - 1);
    }

  private:
    Out &out;
};

template <> void Renderer<std::vector<char>>::write(const std::string &str)
{
    out.insert(out.end(), str.begin(), str.end());
}

template <> void Renderer<std::vector<char>>::write(const char *str, size_t size)
{
    out.insert(out.end(), str, str + size);
}

template <> void Renderer<std::vector<char>>::write(char ch) { out.push_back(ch); }

template <> void Renderer<std::ostream>::write(const std::string &str) { out << str; }

template <> void Renderer<std::ostream>::write(const char *str, size_t size)
{
    out.write(str, size);
}

template <> void Renderer<std::ostream>::write(char ch) { out << ch; }

template <> void Renderer<std::string>::write(const std::string &str) { out += str; }

template <> void Renderer<std::string>::write(const char *str, size_t size)
{
    out.append(str, size);
}

template <> void Renderer<std::string>::write(char ch) { out += ch; }

inline void render(std::ostream &out, const Object &object)
{
    Renderer renderer(out);
    renderer(object);
}

inline void render(std::string &out, const Object &object)
{
    Renderer renderer(out);
    renderer(object);
}

inline void render(std::vector<char> &out, const Object &object)
{
    Renderer renderer(out);
    renderer(object);
}

} // namespace osrm::util::json

#endif // JSON_RENDERER_HPP
