// based on
// https://svn.apache.org/repos/asf/mesos/tags/release-0.9.0-incubating-RC0/src/common/json.hpp

#ifndef JSON_RENDERER_HPP
#define JSON_RENDERER_HPP

#include "util/string_util.hpp"

#include "osrm/json_container.hpp"

#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <ostream>
#include <string>
#include <vector>

#include "util/format.hpp"

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
        // Handle NaN and Infinity gracefully - output null for invalid numbers.
        // This produces valid JSON instead of "nan" or "inf".
        if (!std::isfinite(number.value))
        {
            write<>("null");
            return;
        }

        // Check if value is a non-negative whole number that can be exactly
        // represented in a double. Doubles have 53 mantissa bits, so integers
        // up to 2^53 can be exactly represented. This handles large OSM IDs
        // (up to ~10 billion currently) without scientific notation.
        constexpr auto max_exact_int =
            static_cast<double>(1ULL << std::numeric_limits<double>::digits);
        if (number.value >= 0.0 && number.value <= max_exact_int &&
            std::trunc(number.value) == number.value)
        {
            auto int_value = static_cast<std::uint64_t>(number.value);
            std::string formatted = compat::format("{}", int_value);
            write(formatted.data(), formatted.size());
        }
        else
        {
            std::string formatted = compat::format("{:.10g}", number.value);
            write(formatted.data(), formatted.size());
        }
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
    void write(std::string_view str);
    void write(const char *str, size_t size);
    void write(char ch);

    template <size_t StrLength> void write(const char (&str)[StrLength])
    {
        write(str, StrLength - 1);
    }

  private:
    Out &out;
};

template <> inline void Renderer<std::vector<char>>::write(std::string_view str)
{
    out.insert(out.end(), str.begin(), str.end());
}

template <> inline void Renderer<std::vector<char>>::write(const char *str, size_t size)
{
    out.insert(out.end(), str, str + size);
}

template <> inline void Renderer<std::vector<char>>::write(char ch) { out.push_back(ch); }

template <> inline void Renderer<std::ostream>::write(std::string_view str) { out << str; }

template <> inline void Renderer<std::ostream>::write(const char *str, size_t size)
{
    out.write(str, size);
}

template <> inline void Renderer<std::ostream>::write(char ch) { out << ch; }

template <> inline void Renderer<std::string>::write(std::string_view str) { out += str; }

template <> inline void Renderer<std::string>::write(const char *str, size_t size)
{
    out.append(str, size);
}

template <> inline void Renderer<std::string>::write(char ch) { out += ch; }

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
