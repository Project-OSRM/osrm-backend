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

struct Renderer
{
    explicit Renderer(std::ostream &_out) : out(_out) {}

    void operator()(const String &string) const
    {
        out << "\"";
        out << escape_JSON(string.value);
        out << "\"";
    }

    void operator()(const Number &number) const
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
        out << buffer;
    }

    void operator()(const Object &object) const
    {
        out << "{";
        for (auto it = object.values.begin(), end = object.values.end(); it != end;)
        {
            out << "\"" << it->first << "\":";
            mapbox::util::apply_visitor(Renderer(out), it->second);
            if (++it != end)
            {
                out << ",";
            }
        }
        out << "}";
    }

    void operator()(const Array &array) const
    {
        out << "[";
        for (auto it = array.values.cbegin(), end = array.values.cend(); it != end;)
        {
            mapbox::util::apply_visitor(Renderer(out), *it);
            if (++it != end)
            {
                out << ",";
            }
        }
        out << "]";
    }

    void operator()(const True &) const { out << "true"; }

    void operator()(const False &) const { out << "false"; }

    void operator()(const Null &) const { out << "null"; }

  private:
    std::ostream &out;
};

struct ArrayRenderer
{
    explicit ArrayRenderer(std::vector<char> &_out) : out(_out) {}

    void operator()(const String &string) const
    {
        out.push_back('\"');
        const auto string_to_insert = escape_JSON(string.value);
        out.insert(std::end(out), std::begin(string_to_insert), std::end(string_to_insert));
        out.push_back('\"');
    }

    void operator()(const Number &number) const
    {
        const std::string number_string = cast::to_string_with_precision(number.value);
        out.insert(out.end(), number_string.begin(), number_string.end());
    }

    void operator()(const Object &object) const
    {
        out.push_back('{');
        for (auto it = object.values.begin(), end = object.values.end(); it != end;)
        {
            out.push_back('\"');
            out.insert(out.end(), it->first.begin(), it->first.end());
            out.push_back('\"');
            out.push_back(':');

            mapbox::util::apply_visitor(ArrayRenderer(out), it->second);
            if (++it != end)
            {
                out.push_back(',');
            }
        }
        out.push_back('}');
    }

    void operator()(const Array &array) const
    {
        out.push_back('[');
        for (auto it = array.values.cbegin(), end = array.values.cend(); it != end;)
        {
            mapbox::util::apply_visitor(ArrayRenderer(out), *it);
            if (++it != end)
            {
                out.push_back(',');
            }
        }
        out.push_back(']');
    }

    void operator()(const True &) const
    {
        const std::string temp("true");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const False &) const
    {
        const std::string temp("false");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const Null &) const
    {
        const std::string temp("null");
        out.insert(out.end(), temp.begin(), temp.end());
    }

  private:
    std::vector<char> &out;
};

inline void render(std::ostream &out, const Object &object)
{
    Value value = object;
    mapbox::util::apply_visitor(Renderer(out), value);
}

inline void render(std::vector<char> &out, const Object &object)
{
    Value value = object;
    mapbox::util::apply_visitor(ArrayRenderer(out), value);
}

} // namespace json
} // namespace util
} // namespace osrm

#endif // JSON_RENDERER_HPP
