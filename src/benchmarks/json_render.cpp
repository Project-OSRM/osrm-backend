
#include "osrm/json_container.hpp"
#include "util/json_container.hpp"
#include "util/json_renderer.hpp"
#include "util/timing_util.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <sstream>
#include <stdexcept>

using namespace osrm;

namespace
{

void convert(const rapidjson::Value &value, json::Value &result)
{
    if (value.IsString())
    {
        result = json::String{value.GetString()};
    }
    else if (value.IsNumber())
    {
        result = json::Number{value.GetDouble()};
    }
    else if (value.IsObject())
    {
        json::Object object;
        for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr)
        {
            json::Value member;
            convert(itr->value, member);
            object.values.emplace(itr->name.GetString(), std::move(member));
        }
        result = std::move(object);
    }
    else if (value.IsArray())
    {
        json::Array array;
        for (auto itr = value.Begin(); itr != value.End(); ++itr)
        {
            json::Value member;
            convert(*itr, member);
            array.values.push_back(std::move(member));
        }
        result = std::move(array);
    }
    else if (value.IsBool())
    {
        if (value.GetBool())
        {
            result = json::True{};
        }
        else
        {
            result = json::False{};
        }
    }
    else if (value.IsNull())
    {
        result = json::Null{};
    }
    else
    {
        throw std::runtime_error("unknown type");
    }
}

json::Object load(const char *filename)
{
    // load file to std string
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // load rapidjson document
    rapidjson::Document document;
    document.Parse(json.c_str());
    if (document.HasParseError())
    {
        throw std::runtime_error("Failed to parse JSON");
    }

    json::Value result;
    convert(document, result);
    return std::get<json::Object>(result);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " file.json\n";
        return EXIT_FAILURE;
    }

    const auto obj = load(argv[1]);

    TIMER_START(string);
    std::string out_str;
    json::render(out_str, obj);
    TIMER_STOP(string);
    std::cout << "String: " << TIMER_MSEC(string) << "ms" << std::endl;

    TIMER_START(stringstream);
    std::stringstream ss;
    json::render(ss, obj);
    std::string out_ss_str{ss.str()};
    TIMER_STOP(stringstream);

    std::cout << "Stringstream: " << TIMER_MSEC(stringstream) << "ms" << std::endl;
    TIMER_START(vector);
    std::vector<char> out_vec;
    json::render(out_vec, obj);
    TIMER_STOP(vector);
    std::cout << "Vector: " << TIMER_MSEC(vector) << "ms" << std::endl;

    if (std::string{out_vec.begin(), out_vec.end()} != out_str || out_str != out_ss_str)
    {
        throw std::logic_error("Vector/stringstream/string results are not equal");
    }
    return EXIT_SUCCESS;
}
