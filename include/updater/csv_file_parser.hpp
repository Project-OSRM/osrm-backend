#ifndef OSRM_UPDATER_CSV_FILE_PARSER_HPP
#define OSRM_UPDATER_CSV_FILE_PARSER_HPP
#include "file_parser.hpp"
#include <parquet/arrow/reader.h>
#include <parquet/stream_reader.h>
#include <arrow/io/file.h>

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

#include <exception>
#include <stdexcept>
#include <vector>

namespace osrm
{
namespace updater
{

// Functor to parse a list of CSV files using "key,value,comment" grammar.
// Key and Value structures must be a model of Random Access Sequence.
// Also the Value structure must have source member that will be filled
// with the corresponding file index in the CSV filenames vector.
template <typename Key, typename Value> struct CSVFilesParser : public FilesParser<Key, Value>
{
    using Iterator = boost::iostreams::mapped_file_source::iterator;
    using KeyRule = boost::spirit::qi::rule<Iterator, Key()>;
    using ValueRule = boost::spirit::qi::rule<Iterator, Value()>;

    CSVFilesParser(const KeyRule &key_rule, const ValueRule &value_rule)
        : key_rule(key_rule), value_rule(value_rule)
    {
    }

  private:
    // Parse a single CSV file and return result as a vector<Key, Value>
    std::vector<std::pair<Key, Value>> ParseFile(const std::string &filename, std::size_t file_id) const final
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

            return result;
        }
        catch (const boost::exception &e)
        {
            const auto message = boost::format("exception in loading %1%:\n %2%") % filename %
                                 boost::diagnostic_information(e);
            throw util::exception(message.str() + SOURCE_REF);
        }
    }
  
    const KeyRule key_rule;
    const ValueRule value_rule;
};
} // namespace updater
} // namespace osrm

#endif
