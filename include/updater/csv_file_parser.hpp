#ifndef OSRM_UPDATER_CSV_FILE_PARSER_HPP
#define OSRM_UPDATER_CSV_FILE_PARSER_HPP

#include "updater/source.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"

#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>
#include <tbb/spin_mutex.h>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <vector>

namespace osrm
{
namespace updater
{

// Functor to parse a list of CSV files using "key,value,comment" grammar.
// Key and Value structures must be a model of Random Access Sequence.
// Also the Value structure must have source member that will be filled
// with the corresponding file index in the CSV filenames vector.
template <typename Key, typename Value> struct CSVFilesParser
{
    using Iterator = boost::iostreams::mapped_file_source::iterator;
    using KeyRule = boost::spirit::qi::rule<Iterator, Key()>;
    using ValueRule = boost::spirit::qi::rule<Iterator, Value()>;

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
            tbb::parallel_sort(begin(lookup), end(lookup), [](const auto &lhs, const auto &rhs) {
                return std::tie(rhs.first, rhs.second.source) <
                       std::tie(lhs.first, lhs.second.source);
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
        namespace qi = boost::spirit::qi;

        std::vector<std::pair<Key, Value>> result;
        try
        {
            if (boost::filesystem::file_size(filename) == 0)
                return result;

            boost::iostreams::mapped_file_source mmap(filename);
            auto first = mmap.begin(), last = mmap.end();

            BOOST_ASSERT(file_id <= std::numeric_limits<std::uint8_t>::max());
            ValueRule value_source =
                value_rule[qi::_val = qi::_1, bind(&Value::source, qi::_val) = file_id];
            qi::rule<Iterator, std::pair<Key, Value>()> csv_line =
                (key_rule >> ',' >> value_source) >> -(',' >> *(qi::char_ - qi::eol));
            const auto ok = qi::parse(first, last, -(csv_line % qi::eol) >> *qi::eol, result);

            if (!ok || first != last)
            {
                auto begin_of_line = first - 1;
                while (begin_of_line >= mmap.begin() && *begin_of_line != '\n')
                    --begin_of_line;
                auto line_number = std::count(mmap.begin(), first, '\n') + 1;
                const auto message = boost::format("CSV file %1% malformed on line %2%:\n %3%\n") %
                                     filename % std::to_string(line_number) %
                                     std::string(begin_of_line + 1, std::find(first, last, '\n'));
                throw util::exception(message.str() + SOURCE_REF);
            }

            util::Log() << "Loaded " << filename << " with " << result.size() << "values";

            return std::move(result);
        }
        catch (const boost::exception &e)
        {
            const auto message = boost::format("exception in loading %1%:\n %2%") % filename %
                                 boost::diagnostic_information(e);
            throw util::exception(message.str() + SOURCE_REF);
        }
    }

    const std::size_t start_index;
    const KeyRule key_rule;
    const ValueRule value_rule;
};
}
}

#endif
