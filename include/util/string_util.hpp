#ifndef STRING_UTIL_HPP
#define STRING_UTIL_HPP

#include <array>
#include <cctype>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace osrm::util
{

// implements Lemire's table-based escape needs check
// cf. https://lemire.me/blog/2024/05/31/quickly-checking-whether-a-string-needs-escaping/
inline static constexpr std::array<uint8_t, 256> json_quotable_character = []() constexpr
{
    std::array<uint8_t, 256> result{};
    for (auto i = 0; i < 32; i++)
    {
        result[i] = 1;
    }
    for (auto i : {'"', '\\'})
    {
        result[i] = 1;
    }
    return result;
}();

inline bool RequiresJSONStringEscaping(const std::string &string)
{
    uint8_t needs = 0;
    for (uint8_t c : string)
    {
        needs |= json_quotable_character[c];
    }
    return needs;
}

inline void EscapeJSONString(const std::string &input, std::string &output)
{
    for (const char letter : input)
    {
        switch (letter)
        {
        case '\\':
            output += "\\\\";
            break;
        case '"':
            output += "\\\"";
            break;
        case '/':
            output += "\\/";
            break;
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            output.append(1, letter);
            break;
        }
    }
}

inline std::size_t URIDecode(const std::string &input, std::string &output)
{
    auto src_iter = std::begin(input);
    const auto src_end = std::end(input);
    output.resize(input.size() + 1);
    std::size_t decoded_length = 0;
    for (decoded_length = 0; src_iter != src_end; ++decoded_length)
    {
        if (src_iter[0] == '%' && src_iter[1] && src_iter[2] && isxdigit(src_iter[1]) &&
            isxdigit(src_iter[2]))
        {
            std::string::value_type a = src_iter[1];
            std::string::value_type b = src_iter[2];
            a -= src_iter[1] < 58 ? 48 : src_iter[1] < 71 ? 55 : 87;
            b -= src_iter[2] < 58 ? 48 : src_iter[2] < 71 ? 55 : 87;
            output[decoded_length] = 16 * a + b;
            src_iter += 3;
            continue;
        }
        output[decoded_length] = *src_iter++;
    }
    output.resize(decoded_length);
    return decoded_length;
}

inline std::size_t URIDecodeInPlace(std::string &URI) { return URIDecode(URI, URI); }
} // namespace osrm::util

#endif // STRING_UTIL_HPP
