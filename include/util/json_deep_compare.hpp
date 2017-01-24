#ifndef UTIL_JSON_DEEP_COMPARE_HPP
#define UTIL_JSON_DEEP_COMPARE_HPP

#include "util/integer_range.hpp"
#include "util/json_container.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <functional>
#include <set>

namespace osrm
{
namespace util
{
namespace json
{

struct Comparator
{
    Comparator(std::string &reason_, const std::string &lhs_path_, const std::string &rhs_path_)
        : reason(reason_), lhs_path(lhs_path_), rhs_path(rhs_path_)
    {
    }

    bool operator()(const String &lhs, const String &rhs) const
    {
        bool is_same = lhs.value == rhs.value;
        if (!is_same)
        {
            reason = lhs_path + " (= \"" + lhs.value + "\") != " + rhs_path + " (= \"" + rhs.value +
                     "\")";
        }
        return is_same;
    }

    bool operator()(const Number &lhs, const Number &rhs) const
    {
        bool is_same = lhs.value == rhs.value;
        if (!is_same)
        {
            reason = lhs_path + " (= " + std::to_string(lhs.value) + ") != " + rhs_path + " (= " +
                     std::to_string(rhs.value) + ")";
        }
        return is_same;
    }

    bool operator()(const Object &lhs, const Object &rhs) const
    {
        std::set<std::string> lhs_keys;
        for (const auto &key_value : lhs.values)
        {
            lhs_keys.insert(key_value.first);
        }

        std::set<std::string> rhs_keys;
        for (const auto &key_value : rhs.values)
        {
            rhs_keys.insert(key_value.first);
        }

        for (const auto &key : lhs_keys)
        {
            if (rhs_keys.find(key) == rhs_keys.end())
            {
                reason = rhs_path + " doesn't have key \"" + key + "\"";
                return false;
            }
        }

        for (const auto &key : rhs_keys)
        {
            if (lhs_keys.find(key) == lhs_keys.end())
            {
                reason = lhs_path + " doesn't have key \"" + key + "\"";
                return false;
            }
        }

        for (const auto &key : lhs_keys)
        {
            BOOST_ASSERT(rhs.values.find(key) != rhs.values.end());
            BOOST_ASSERT(lhs.values.find(key) != lhs.values.end());

            const auto &rhs_child = rhs.values.find(key)->second;
            const auto &lhs_child = lhs.values.find(key)->second;
            auto is_same = mapbox::util::apply_visitor(
                Comparator(reason, lhs_path + "." + key, rhs_path + "." + key),
                lhs_child,
                rhs_child);
            if (!is_same)
            {
                return false;
            }
        }
        return true;
    }

    bool operator()(const Array &lhs, const Array &rhs) const
    {
        if (lhs.values.size() != rhs.values.size())
        {
            reason = lhs_path + ".length " + std::to_string(lhs.values.size()) + " != " + rhs_path +
                     ".length " + std::to_string(rhs.values.size());
            return false;
        }

        for (auto i = 0UL; i < lhs.values.size(); ++i)
        {
            auto is_same =
                mapbox::util::apply_visitor(Comparator(reason,
                                                       lhs_path + "[" + std::to_string(i) + "]",
                                                       rhs_path + "[" + std::to_string(i) + "]"),
                                            lhs.values[i],
                                            rhs.values[i]);
            if (!is_same)
            {
                return false;
            }
        }

        return true;
    }

    bool operator()(const True &, const True &) const { return true; }
    bool operator()(const False &, const False &) const { return true; }
    bool operator()(const Null &, const Null &) const { return true; }

    bool operator()(const False &, const True &) const
    {
        reason = lhs_path + " is false but " + rhs_path + " is true";
        return false;
    }
    bool operator()(const True &, const False &) const
    {
        reason = lhs_path + " is true but " + rhs_path + " is false";
        return false;
    }

    template <typename T1,
              typename T2,
              typename = typename std::enable_if<!std::is_same<T1, T2>::value>::type>
    bool operator()(const T1 &, const T2 &)
    {
        reason = lhs_path + " and " + rhs_path + " have different types";
        return false;
    }

  private:
    std::string &reason;
    const std::string &lhs_path;
    const std::string &rhs_path;
};

inline bool compare(const Value &reference, const Value &result, std::string &reason)
{
    return mapbox::util::apply_visitor(
        Comparator(reason, "reference", "result"), reference, result);
}
}
}
}

#endif
