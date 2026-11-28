#ifndef OSRM_EXTRACTOR_RECORD_SPOOL_HPP
#define OSRM_EXTRACTOR_RECORD_SPOOL_HPP

#include "util/exception.hpp"
#include "util/exception_utils.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace osrm::extractor
{

template <typename Record> class RecordSpool
{
    static_assert(std::is_trivially_copyable_v<Record>);
    static constexpr std::size_t BatchSize = (16 * 1024 * 1024) / sizeof(Record);

    std::filesystem::path m_parent;
    std::filesystem::path m_directory;
    std::ofstream m_output;
    std::vector<Record> m_batch;
    std::size_t m_written = 0;

    void writeBatch()
    {
        if (!m_output.is_open())
        {
            std::random_device entropy;
            std::filesystem::path directory;
            do
            {
                directory = m_parent / ("osrm-extract-" + std::to_string(entropy()) + "-" +
                                        std::to_string(entropy()));
            } while (!std::filesystem::create_directory(directory));
            m_directory = std::move(directory);
            m_output.open(m_directory / "records", std::ios::binary);
        }
        m_output.write(reinterpret_cast<const char *>(m_batch.data()),
                       m_batch.size() * sizeof(Record));
        if (!m_output)
            throw util::exception("Failed writing temporary extraction data" + SOURCE_REF);
        m_written += m_batch.size();
        m_batch.clear();
    }

    static void readBatch(std::ifstream &input, std::span<Record> batch)
    {
        input.read(reinterpret_cast<char *>(batch.data()), batch.size_bytes());
        if (!input)
            throw util::exception("Failed reading temporary extraction data" + SOURCE_REF);
    }

    void removeFile()
    {
        if (m_directory.empty())
            return;
        if (m_output.is_open())
            m_output.close();
        std::error_code error;
        std::filesystem::remove(m_directory / "records", error);
        std::filesystem::remove(m_directory, error);
        m_directory.clear();
    }

  public:
    using value_type = Record;

    explicit RecordSpool(std::filesystem::path parent) : m_parent(std::move(parent)) {}
    ~RecordSpool() { removeFile(); }

    std::size_t size() const { return m_written + m_batch.size(); }
    bool empty() const { return size() == 0; }
    bool spilled() const { return m_written != 0; }

    void append(const Record &record)
    {
        BOOST_ASSERT(!spilled() || m_output.is_open());
        if (m_batch.size() == BatchSize)
            writeBatch();
        m_batch.push_back(record);
    }

    void finish()
    {
        if (!m_output.is_open())
            return;
        if (!m_batch.empty())
            writeBatch();
        m_output.close();
        if (!m_output)
            throw util::exception("Failed closing temporary extraction data" + SOURCE_REF);
        std::vector<Record>().swap(m_batch);
    }

    template <typename Visit> void forEachBatch(Visit visit) const
    {
        BOOST_ASSERT(!m_output.is_open());
        if (!spilled())
        {
            visit(std::span<const Record>(m_batch));
            return;
        }

        std::ifstream input(m_directory / "records", std::ios::binary);
        std::vector<Record> buffer(std::min(BatchSize, m_written));
        for (std::size_t offset = 0; offset < m_written; offset += BatchSize)
        {
            const auto batch =
                std::span<Record>(buffer).first(std::min(BatchSize, m_written - offset));
            readBatch(input, batch);
            visit(std::span<const Record>(batch));
        }
    }

    std::vector<Record> takeVector()
    {
        finish();
        if (!spilled())
            return std::exchange(m_batch, {});

        std::vector<Record> result(m_written);
        std::ifstream input(m_directory / "records", std::ios::binary);
        for (std::size_t offset = 0; offset < result.size(); offset += BatchSize)
            readBatch(input,
                      std::span<Record>(result).subspan(
                          offset, std::min(BatchSize, result.size() - offset)));
        input.close();
        clear();
        return result;
    }

    void clear()
    {
        removeFile();
        m_written = 0;
        std::vector<Record>().swap(m_batch);
    }
};

} // namespace osrm::extractor

#endif
