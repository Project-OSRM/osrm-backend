#ifndef OSRM_UPDATER_FILE_PARSER_HPP
#define OSRM_UPDATER_FILE_PARSER_HPP
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
template <typename Key, typename Value> struct FilesParser
{

    virtual ~FilesParser() = default;

    // Operator returns a lambda function that maps input Key to boost::optional<Value>.
    auto operator()(const std::vector<std::string> &csv_filenames) const
    {
        try
        {
            tbb::spin_mutex mutex;
            std::vector<std::pair<Key, Value>> lookup;
            tbb::parallel_for(std::size_t{0}, csv_filenames.size(), [&](const std::size_t idx) {
                // TODO: do we need this `1 + ` here?
                auto local = ParseFile(csv_filenames[idx], 1 + idx);

                { // Merge local results into a flat global vector
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
        catch (const std::exception &e)
        // TBB should capture to std::exception_ptr and automatically rethrow in this thread.
        // https://software.intel.com/en-us/node/506317
        {
            throw util::exception(e.what() + SOURCE_REF);
        }
    }

  protected:
    // Parse a single CSV file and return result as a vector<Key, Value>
    virtual std::vector<std::pair<Key, Value>> ParseFile(const std::string &filename, std::size_t file_id) const;
};
} // namespace updater
} // namespace osrm

#endif
