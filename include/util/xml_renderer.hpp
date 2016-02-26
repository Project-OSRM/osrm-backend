#ifndef XML_RENDERER_HPP
#define XML_RENDERER_HPP

#include "util/cast.hpp"

#include "osrm/json_container.hpp"

namespace osrm
{
namespace util
{
namespace json
{

struct XMLToArrayRenderer : mapbox::util::static_visitor<>
{
    explicit XMLToArrayRenderer(std::vector<char> &_out) : out(_out) {}

    void operator()(const Buffer &buffer) const
    {
        out.push_back('\"');
        out.insert(out.end(), buffer.value.begin(), buffer.value.end());
        out.push_back('\"');
    }

    void operator()(const String &string) const
    {
        out.push_back('\"');
        out.insert(out.end(), string.value.begin(), string.value.end());
        out.push_back('\"');
    }

    void operator()(const Number &number) const
    {
        const std::string number_string = cast::to_string_with_precision(number.value);
        out.insert(out.end(), number_string.begin(), number_string.end());
    }

    void operator()(const Object &object) const
    {
        for (auto &&each : object.values)
        {
            if (each.first.at(0) != '_')
            {
                out.push_back('<');
                out.insert(out.end(), each.first.begin(), each.first.end());
            }
            else
            {
                out.push_back(' ');
                out.insert(out.end(), ++(each).first.begin(), each.first.end());
                out.push_back('=');
            }
            mapbox::util::apply_visitor(XMLToArrayRenderer(out), each.second);
            if (each.first.at(0) != '_')
            {
                out.push_back('/');
                out.push_back('>');
            }
        }
    }

    void operator()(const Array &array) const
    {
        for (auto &&each : array.values)
        {
            mapbox::util::apply_visitor(XMLToArrayRenderer(out), each);
        }
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

template <class JSONObject> inline void xml_render(std::vector<char> &out, const JSONObject &object)
{
    Value value = object;
    mapbox::util::apply_visitor(XMLToArrayRenderer(out), value);
}

template <class JSONObject> inline void gpx_render(std::vector<char> &out, const JSONObject &object)
{
    // add header

    const std::string header{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?><gpx creator=\"OSRM Routing Engine\""
        " version=\"1.1\" xmlns=\"http://www.topografix.com/GPX/1/1\" xmlns:xsi=\"http:"
        "//www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topogr"
        "afix.com/GPX/1/1 gpx.xsd\"><metadata><copyright author=\"Project OSRM\"><lice"
        "nse>Data (c) OpenStreetMap contributors (ODbL)</license></copyright></metadat"
        "a><rte>"};
    out.insert(out.end(), header.begin(), header.end());

    xml_render(out, object);

    const std::string footer{"</rte></gpx>"};
    out.insert(out.end(), footer.begin(), footer.end());
}

} // namespace json
} // namespace util
} // namespace osrm

#endif // XML_RENDERER_HPP
