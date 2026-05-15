#pragma once

#include <boost/program_options.hpp>
#include <filesystem>
#include <string>
#include <vector>

// Custom validate() for std::filesystem::path so that
// boost::program_options handles paths with spaces correctly.
// Without this, the default stream extraction operator>> truncates
// at whitespace.
namespace boost::program_options
{
inline void validate(boost::any &v,
                     const std::vector<std::string> &values,
                     std::filesystem::path * /*target_type*/,
                     int /*unused*/)
{
    validators::check_first_occurrence(v);
    const std::string &s = validators::get_single_string(values);
    v = boost::any(std::filesystem::path(s));
}
} // namespace boost::program_options
