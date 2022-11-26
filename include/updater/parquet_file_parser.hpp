#ifndef OSRM_UPDATER_PARQUET_FILE_PARSER_HPP
#define OSRM_UPDATER_PARQUET_FILE_PARSER_HPP
#include "file_parser.hpp"
#include <optional>
#include <parquet/arrow/reader.h>
#include <parquet/stream_reader.h>
#include <arrow/io/file.h>

#include "updater/source.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"

#include <parquet/stream_writer.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>
#include <tbb/spin_mutex.h>

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
template <typename Key, typename Value> struct ParquetFilesParser : public FilesParser<Key, Value>
{
  private:
    // Parse a single CSV file and return result as a vector<Key, Value>
    std::vector<std::pair<Key, Value>> ParseFile(const std::string &filename, std::size_t file_id) const final
    {
        // TODO: error handling
        std::shared_ptr<arrow::io::ReadableFile> infile;

        PARQUET_ASSIGN_OR_THROW(
            infile,
            arrow::io::ReadableFile::Open(filename));

        parquet::StreamReader os{parquet::ParquetFileReader::Open(infile)};

        std::vector<std::pair<Key, Value>> result;
        while ( !os.eof() )
        {
            result.emplace_back(ReadKey(os), ReadValue(os, file_id));
      // ...
        }
        return result;
//     (void)os;

    }

    Key ReadKey(parquet::StreamReader &os) const
    {
        Key key;
        ReadKey(os, key);
        return key;
    }

    Value ReadValue(parquet::StreamReader &os, std::size_t file_id) const
    {
        Value value;
        ReadValue(os, value);
        value.source = file_id;
        return value;
    }

    void ReadKey(parquet::StreamReader &os, Turn& turn) const {
        os >> turn.from >> turn.via >> turn.to >> parquet::EndRow;
    }

    void ReadValue(parquet::StreamReader &os, PenaltySource& penalty_source) const {
        os >> penalty_source.duration >> penalty_source.weight >> parquet::EndRow;
    }

    void ReadKey(parquet::StreamReader &os, Segment& segment) const {
        os >> segment.from >> segment.to >> parquet::EndRow;
    }

    void ReadValue(parquet::StreamReader &os, SpeedSource& speed_source) const {
        std::optional<double> rate;
        os >> speed_source.speed >> rate >> parquet::EndRow;
        // TODO: boost::optional
        if (rate) {
            speed_source.rate = *rate;
        }
       // os >> turn.weight >> turn.duration >> turn.pre_turn_bearing >> turn.post_turn_bearing >> turn.source >> parquet::EndRow;
    }
  
};
} // namespace updater
} // namespace osrm

#endif
