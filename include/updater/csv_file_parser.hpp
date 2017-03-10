#ifndef OSRM_UPDATER_CSV_FILE_PARSER_HPP
#define OSRM_UPDATER_CSV_FILE_PARSER_HPP

#include "updater/source.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"

#include <tbb/parallel_for.h>
#include <tbb/spin_mutex.h>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

#include <fstream>
#include <vector>

namespace osrm
{
namespace updater
{

namespace
{
namespace qi = boost::spirit::qi;
}

// Functor to parse a list of CSV files using "key,value,comment" grammar.
// Key and Value structures must be a model of Random Access Sequence.
// Also the Value structure must have source member that will be filled
// with the corresponding file index in the CSV filenames vector.
template <typename Key, typename Value> struct CSVFilesParser
{
    using Iterator = boost::spirit::line_pos_iterator<boost::spirit::istream_iterator>;
    using KeyRule = qi::rule<Iterator, Key()>;
    using ValueRule = qi::rule<Iterator, Value()>;

    CSVFilesParser(std::size_t start_index, const KeyRule &key_rule, const ValueRule &value_rule)
        : start_index(start_index), key_rule(key_rule), value_rule(value_rule)
    {
    }

    // Operator returns a lambda function that maps input Key to boost::optional<Value>.
    auto operator()(const std::vector<std::string> &csv_filenames) const
    {
        try
        {
            tbb::spin_mutex mutex;
            std::vector<std::pair<Key, Value>> lookup;
            tbb::parallel_for(std::size_t{0},
                              csv_filenames.size(),
                              [&](const std::size_t idx) {
                                  auto local = ParseCSVFile(csv_filenames[idx], start_index + idx);

                                  { // Merge local CSV results into a flat global vector
                                      tbb::spin_mutex::scoped_lock _{mutex};
                                      lookup.insert(end(lookup),
                                                    std::make_move_iterator(begin(local)),
                                                    std::make_move_iterator(end(local)));
                                  }
                              });

            // With flattened map-ish view of all the files, make a stable sort on key and source
            // and unique them on key to keep only the value with the largest file index
            // and the largest line number in a file.
            // The operands order is swapped to make descending ordering on (key, source)
            std::stable_sort(begin(lookup), end(lookup), [](const auto &lhs, const auto &rhs) {
                return rhs.first < lhs.first ||
                       (rhs.first == lhs.first && rhs.second.source < lhs.second.source);
            });

            // Unique only on key to take the source precedence into account and remove duplicates.
            const auto it =
                std::unique(begin(lookup), end(lookup), [](const auto &lhs, const auto &rhs) {
                    return lhs.first == rhs.first;
                });
            lookup.erase(it, end(lookup));

            util::Log() << "In total loaded " << csv_filenames.size() << " file(s) with a total of "
                        << lookup.size() << " unique values";

            return LookupTable<Key, Value>{lookup};
        }
        catch (const tbb::captured_exception &e)
        {
            throw util::exception(e.what() + SOURCE_REF);
        }
    }

  private:
    // Parse a single CSV file and return result as a vector<Key, Value>
    auto ParseCSVFile(const std::string &filename, std::size_t file_id) const
    {
        std::ifstream input_stream(filename, std::ios::binary);
        input_stream.unsetf(std::ios::skipws);

        boost::spirit::istream_iterator sfirst(input_stream), slast;
        Iterator first(sfirst), last(slast);

        BOOST_ASSERT(file_id <= std::numeric_limits<std::uint8_t>::max());
        ValueRule value_source =
            value_rule[qi::_val = qi::_1, boost::phoenix::bind(&Value::source, qi::_val) = file_id];
        qi::rule<Iterator, std::pair<Key, Value>()> csv_line =
            (key_rule >> ',' >> value_source) >> -(',' >> *(qi::char_ - qi::eol));
        std::vector<std::pair<Key, Value>> result;
        const auto ok = qi::parse(first, last, -(csv_line % qi::eol) >> *qi::eol, result);

        if (!ok || first != last)
        {
            const auto message =
                boost::format("CSV file %1% malformed on line %2%") % filename % first.position();
            throw util::exception(message.str() + SOURCE_REF);
        }

        util::Log() << "Loaded " << filename << " with " << result.size() << "values";

        return std::move(result);
    }

    const std::size_t start_index;
    const KeyRule key_rule;
    const ValueRule value_rule;
};
}
}

#endif
