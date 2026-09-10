#include "engine/datafacade/mmap_memory_allocator.hpp"
#include "engine/datafacade/process_memory_allocator.hpp"
#include "engine/isochrone/duration_graph.hpp"

#include "contractor/query_graph.hpp"

#include "storage/serialization.hpp"
#include "storage/storage_config.hpp"
#include "storage/tar.hpp"

#include "util/exception.hpp"
#include "util/serialization.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
constexpr const char *METRIC_PREFIX = "/ch/metrics/routability";

std::filesystem::path makeCHDatasetCopy()
{
    const auto source_base = std::filesystem::path{OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    const auto directory =
        std::filesystem::temp_directory_path() / ("osrm-mmap-isochrone-" + random_string(8));
    std::filesystem::create_directory(directory);
    const auto target_base = directory / source_base.filename();

    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(source_base.parent_path()))
        {
            if (!entry.is_regular_file() || !entry.path().filename().string().starts_with(
                                                source_base.filename().string() + "."))
            {
                continue;
            }

            std::filesystem::copy_file(entry.path(),
                                       directory / entry.path().filename(),
                                       std::filesystem::copy_options::overwrite_existing);
        }
    }
    catch (...)
    {
        std::filesystem::remove_all(directory);
        throw;
    }

    return target_base;
}

void writeIsochroneGraph(const std::filesystem::path &base, const bool corrupt)
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    using QueryGraph = osrm::contractor::QueryGraph;
    using QueryEdge = osrm::contractor::QueryEdge;

    const std::vector<QueryGraph::InputEdge> input_edges = {{0, 1, QueryEdge::EdgeData{}},
                                                            {1, 2, QueryEdge::EdgeData{}}};
    const QueryGraph graph{3, input_edges};
    const std::vector<::EdgeID> forward_offsets = {0, 1, 2, 2};
    const std::vector<Arc> forward_arcs = {{1, ::EdgeWeight{10}, ::EdgeDuration{10}},
                                           {2, ::EdgeWeight{20}, ::EdgeDuration{20}}};
    const std::vector<::EdgeID> reverse_offsets = {0, 0, 1, 2};
    const std::vector<Arc> reverse_arcs = {{0, ::EdgeWeight{10}, ::EdgeDuration{corrupt ? 11 : 10}},
                                           {1, ::EdgeWeight{20}, ::EdgeDuration{20}}};

    // The process-memory loader also validates this value against the copied
    // edge-based graph.  Preserve it when replacing the CH archive below so
    // this fixture isolates validation of the isochrone sidecar.
    std::uint32_t connectivity_checksum;
    {
        osrm::storage::tar::FileReader reader(base.string() + ".hsgr",
                                              osrm::storage::tar::FileReader::VerifyFingerprint);
        reader.ReadInto("/ch/connectivity_checksum", connectivity_checksum);
    }

    osrm::storage::tar::FileWriter writer(base.string() + ".hsgr",
                                          osrm::storage::tar::FileWriter::GenerateFingerprint);
    writer.WriteElementCount64("/ch/connectivity_checksum", 1);
    writer.WriteFrom("/ch/connectivity_checksum", connectivity_checksum);
    osrm::util::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/contracted_graph", graph);
    writer.WriteElementCount64(std::string(METRIC_PREFIX) + "/exclude", 0);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/forward_offsets", forward_offsets);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/forward_arcs", forward_arcs);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/reverse_offsets", reverse_offsets);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs", reverse_arcs);
}

void writeOneBlockIsochroneGraph(const std::filesystem::path &base)
{
    osrm::storage::tar::FileWriter writer(base.string() + ".hsgr",
                                          osrm::storage::tar::FileWriter::GenerateFingerprint);
    const std::vector<::EdgeID> offsets = {0};
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/forward_offsets", offsets);
}

void writeEmptyIsochroneGraph(const std::filesystem::path &base)
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    using QueryGraph = osrm::contractor::QueryGraph;
    using QueryEdge = osrm::contractor::QueryEdge;

    const std::vector<QueryGraph::InputEdge> input_edges = {{0, 1, QueryEdge::EdgeData{}},
                                                            {1, 2, QueryEdge::EdgeData{}}};
    const QueryGraph graph{3, input_edges};
    const std::vector<::EdgeID> offsets;
    const std::vector<Arc> arcs;

    osrm::storage::tar::FileWriter writer(base.string() + ".hsgr",
                                          osrm::storage::tar::FileWriter::GenerateFingerprint);
    osrm::util::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/contracted_graph", graph);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/forward_offsets", offsets);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/forward_arcs", arcs);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/reverse_offsets", offsets);
    osrm::storage::serialization::write(
        writer, std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs", arcs);
}

void writeAndTruncateAValidTrailingIsochroneGraph(const std::filesystem::path &base)
{
    using Arc = osrm::engine::isochrone::DurationGraphArc;
    const auto hsgr = std::filesystem::path(base.string() + ".hsgr");
    writeIsochroneGraph(base, false);

    osrm::storage::tar::FileReader reader(hsgr, osrm::storage::tar::FileReader::VerifyFingerprint);
    std::vector<osrm::storage::tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));
    const auto arc_entry = std::find_if(
        entries.begin(),
        entries.end(),
        [](const auto &entry)
        { return entry.name == std::string(METRIC_PREFIX) + "/isochrone/reverse_arcs"; });
    BOOST_REQUIRE(arc_entry != entries.end());
    BOOST_REQUIRE_EQUAL(arc_entry->size, 2 * sizeof(Arc));
    std::filesystem::resize_file(hsgr, arc_entry->offset + 1);
}

void removeIsochroneGraph(const std::filesystem::path &base)
{
    const auto hsgr = std::filesystem::path(base.string() + ".hsgr");
    const auto replacement = std::filesystem::path(base.string() + ".legacy.hsgr");
    osrm::storage::tar::FileReader reader(hsgr, osrm::storage::tar::FileReader::VerifyFingerprint);
    std::vector<osrm::storage::tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));

    {
        osrm::storage::tar::FileWriter writer(replacement,
                                              osrm::storage::tar::FileWriter::HasNoFingerprint);
        for (const auto &entry : entries)
        {
            if (entry.name.find("/isochrone/") != std::string::npos)
                continue;

            std::vector<char> bytes(entry.size);
            reader.ReadInto(entry.name, bytes.data(), bytes.size());
            writer.WriteFrom(entry.name, bytes.data(), bytes.size());
        }
    }

    std::filesystem::rename(replacement, hsgr);
}

template <typename PrepareDataset> void checkMMapRejects(const PrepareDataset &prepare_dataset)
{
    const auto base = makeCHDatasetCopy();
    try
    {
        prepare_dataset(base);
        BOOST_CHECK_THROW(
            osrm::engine::datafacade::MMapMemoryAllocator{osrm::storage::StorageConfig{base}},
            osrm::util::exception);
    }
    catch (...)
    {
        std::filesystem::remove_all(base.parent_path());
        throw;
    }
    std::filesystem::remove_all(base.parent_path());
}

template <typename PrepareDataset> void checkMMapAccepts(const PrepareDataset &prepare_dataset)
{
    const auto base = makeCHDatasetCopy();
    try
    {
        prepare_dataset(base);
        BOOST_CHECK_NO_THROW(
            osrm::engine::datafacade::MMapMemoryAllocator{osrm::storage::StorageConfig{base}});
    }
    catch (...)
    {
        std::filesystem::remove_all(base.parent_path());
        throw;
    }
    std::filesystem::remove_all(base.parent_path());
}

template <typename PrepareDataset>
void checkDirectMemoryRejects(const PrepareDataset &prepare_dataset)
{
    const auto base = makeCHDatasetCopy();
    try
    {
        prepare_dataset(base);
        BOOST_CHECK_THROW(
            osrm::engine::datafacade::ProcessMemoryAllocator{osrm::storage::StorageConfig{base}},
            osrm::util::exception);
    }
    catch (...)
    {
        std::filesystem::remove_all(base.parent_path());
        throw;
    }
    std::filesystem::remove_all(base.parent_path());
}

template <typename PrepareDataset>
void checkDirectMemoryAccepts(const PrepareDataset &prepare_dataset)
{
    const auto base = makeCHDatasetCopy();
    try
    {
        prepare_dataset(base);
        BOOST_CHECK_NO_THROW(
            osrm::engine::datafacade::ProcessMemoryAllocator{osrm::storage::StorageConfig{base}});
    }
    catch (...)
    {
        std::filesystem::remove_all(base.parent_path());
        throw;
    }
    std::filesystem::remove_all(base.parent_path());
}
} // namespace

BOOST_AUTO_TEST_SUITE(mmap_isochrone_index)

BOOST_AUTO_TEST_CASE(accepts_a_complete_isochrone_graph)
{
    checkMMapAccepts([](const auto &base) { writeIsochroneGraph(base, false); });
    checkDirectMemoryAccepts([](const auto &base) { writeIsochroneGraph(base, false); });
}

BOOST_AUTO_TEST_CASE(accepts_a_legacy_hsgr_without_an_isochrone_graph)
{
    checkMMapAccepts(removeIsochroneGraph);
    checkDirectMemoryAccepts(removeIsochroneGraph);
}

BOOST_AUTO_TEST_CASE(rejects_a_partial_isochrone_graph)
{ checkMMapRejects(writeOneBlockIsochroneGraph); }

BOOST_AUTO_TEST_CASE(rejects_a_present_but_empty_isochrone_graph)
{
    checkMMapRejects(writeEmptyIsochroneGraph);
    checkDirectMemoryRejects(writeEmptyIsochroneGraph);
}

BOOST_AUTO_TEST_CASE(rejects_a_semantically_corrupt_isochrone_graph)
{
    checkMMapRejects([](const auto &base) { writeIsochroneGraph(base, true); });
    checkDirectMemoryRejects([](const auto &base) { writeIsochroneGraph(base, true); });
}

BOOST_AUTO_TEST_CASE(rejects_a_truncated_trailing_isochrone_graph)
{ checkMMapRejects(writeAndTruncateAValidTrailingIsochroneGraph); }

BOOST_AUTO_TEST_SUITE_END()
