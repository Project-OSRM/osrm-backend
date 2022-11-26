#ifndef OSRM_UPDATER_PARQUET_FILE_PARSER_HPP
#define OSRM_UPDATER_PARQUET_FILE_PARSER_HPP
#include "file_parser.hpp"
#include <arrow/io/file.h>
#include <optional>
#include <parquet/arrow/reader.h>
#include <parquet/exception.h>
#include <parquet/stream_reader.h>

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

template <typename Key, typename Value> struct ParquetFilesParser : public FilesParser<Key, Value>
{
  private:
    // Parse a single Parquet file and return result as a vector<Key, Value>
    std::vector<std::pair<Key, Value>> ParseFile(const std::string &filename,
                                                 std::size_t file_id) const final
    {
        try
        {
            std::shared_ptr<arrow::io::ReadableFile> infile;

            PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(filename));

            parquet::StreamReader os{parquet::ParquetFileReader::Open(infile)};

            std::vector<std::pair<Key, Value>> result;
            while (!os.eof())
            {
                result.emplace_back(ReadKeyValue(os, file_id));
            }
            return result;
        }
        catch (const std::exception &e)
        {
            throw util::exception(e.what() + SOURCE_REF);
        }
    }

    std::pair<Key, Value> ReadKeyValue(parquet::StreamReader &os, std::size_t file_id) const
    {
        Key key;
        Value value;
        Read(os, key);
        Read(os, value);
        value.source = file_id;
        os >> parquet::EndRow;
        return {key, value};
    }

    void Read(parquet::StreamReader &os, Turn &turn) const
    {
        os >> turn.from >> turn.via >> turn.to;
    }

    void Read(parquet::StreamReader &os, PenaltySource &penalty_source) const
    {
        os >> penalty_source.duration >> penalty_source.weight;
    }

    void Read(parquet::StreamReader &os, Segment &segment) const
    {
        os >> segment.from >> segment.to;
    }

    void Read(parquet::StreamReader &os, SpeedSource &speed_source) const
    {
        std::optional<double> rate;
        os >> speed_source.speed >> rate;
        // TODO: boost::optional
        if (rate)
        {
            speed_source.rate = *rate;
        }
    }
};
} // namespace updater
} // namespace osrm

#endif
