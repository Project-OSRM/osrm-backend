#include "extractor/edge_based_node_segment.hpp"
#include "extractor/files.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "storage/compact_dataset.hpp"
#include "storage/serialization.hpp"
#include "storage/storage.hpp"
#include "storage/storage_config.hpp"
#include "storage/tar.hpp"
#include "util/block_delta_encoding.hpp"
#include "util/hilbert_value.hpp"
#include "util/mmap_file.hpp"
#include "util/serialization.hpp"
#include "util/spatial_delta_view.hpp"

#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
using osrm::storage::Storage;
using osrm::storage::StorageConfig;
using osrm::storage::tar::FileReader;
using osrm::storage::tar::FileWriter;
using osrm::util::Coordinate;
namespace fs = std::filesystem;
namespace serial = osrm::storage::serialization;

constexpr auto node_ids_name = "/common/nbn_data/osm_node_ids";
constexpr auto coordinate_name = "/common/nbn_data/coordinates_spatial";
constexpr auto geometry_name = "/common/segment_data/nodes";
constexpr auto endpoints_name = "/common/rtree/spatial_endpoints";
constexpr auto maneuver_name = "/common/maneuver_overrides/overrides";

class Archive
{
  public:
    explicit Archive(const fs::path &path)
        : reader(path, FileReader::VerifyFingerprint),
          bytes(osrm::util::mmapFile<std::uint8_t>(path, map))
    { reader.List(std::back_inserter(entries)); }

    bool contains(const std::string &name) const { return reader.HasEntry(name); }

    template <typename T> std::span<const T> get(const std::string &name)
    {
        const auto entry = std::ranges::find(entries, name, &FileReader::FileEntry::name);
        if (entry == entries.end() || entry->size % sizeof(T) != 0 ||
            entry->size / sizeof(T) != reader.ReadElementCount64(name) ||
            entry->offset % alignof(T) != 0)
            throw osrm::util::exception("Invalid archive member " + name);
        return {reinterpret_cast<const T *>(bytes.data() + entry->offset), entry->size / sizeof(T)};
    }

    void copyExcept(FileWriter &writer, std::string_view replaced = {}) const
    {
        const auto metadata = std::string(replaced) + ".meta";
        for (const auto &entry : entries)
        {
            if (entry.name != "osrm_fingerprint.meta" && entry.name != replaced &&
                entry.name != metadata)
                writer.WriteFrom(entry.name, bytes.data() + entry.offset, entry.size);
        }
    }

  private:
    FileReader reader;
    boost::iostreams::mapped_file_source map;
    osrm::util::vector_view<const std::uint8_t> bytes;
    std::vector<FileReader::FileEntry> entries;
};

void writeDeltaVector(FileWriter &writer,
                      const std::string &name,
                      const std::vector<unsigned> &values)
{
    const auto encoded = osrm::util::encodeBlockDelta<unsigned>(values);
    if (!encoded)
        throw osrm::util::exception("Too many packed words for " + name);
    serial::write(writer, name + "/headers", encoded->Headers);
    serial::write(writer, name + "/words", encoded->Words);
}

struct SpatialOrder
{
    std::vector<NodeID> old_to_new;
    std::vector<NodeID> new_to_old;
};

SpatialOrder packCoordinates(const fs::path &input, const fs::path &output)
{
    std::vector<Coordinate> coordinates;
    osrm::extractor::PackedOSMIDs ids;
    osrm::extractor::files::readNodes(input, coordinates, ids);
    if (coordinates.empty() || coordinates.size() != ids.size() ||
        coordinates.size() >= SPECIAL_NODEID)
        throw osrm::util::exception("Invalid node coordinate or OSM node ID count");

    std::vector<NodeID> new_to_old(coordinates.size());
    std::iota(new_to_old.begin(), new_to_old.end(), NodeID{0});
    {
        std::vector<std::uint64_t> keys(coordinates.size());
        tbb::parallel_for(std::size_t{0},
                          coordinates.size(),
                          [&](std::size_t i)
                          { keys[i] = osrm::util::GetHilbertCode(coordinates[i]); });
        tbb::parallel_sort(new_to_old.begin(),
                           new_to_old.end(),
                           [&](NodeID a, NodeID b)
                           { return std::tie(keys[a], a) < std::tie(keys[b], b); });
    }

    std::vector<NodeID> old_to_new(coordinates.size());
    std::vector<unsigned> lon(coordinates.size()), lat(coordinates.size());
    tbb::parallel_for(
        std::size_t{0},
        coordinates.size(),
        [&](std::size_t i)
        {
            const auto old_id = new_to_old[i];
            old_to_new[old_id] = static_cast<NodeID>(i);
            lon[i] = osrm::util::spatialUnsigned(static_cast<int>(coordinates[old_id].lon));
            lat[i] = osrm::util::spatialUnsigned(static_cast<int>(coordinates[old_id].lat));
        });

    osrm::extractor::PackedOSMIDs reordered_ids;
    reordered_ids.reserve(ids.size());
    for (const auto id : new_to_old)
        reordered_ids.push_back(std::as_const(ids)[id]);

    FileWriter writer(output, FileWriter::GenerateFingerprint);
    osrm::util::serialization::write(writer, node_ids_name, reordered_ids);
    const auto count_name = std::string(coordinate_name) + "/count";
    writer.WriteElementCount64(count_name, 1);
    writer.WriteFrom(count_name, static_cast<std::uint64_t>(coordinates.size()));
    writeDeltaVector(writer, std::string(coordinate_name) + "/lon", lon);
    writeDeltaVector(writer, std::string(coordinate_name) + "/lat", lat);
    writer.Close();
    return {std::move(old_to_new), std::move(new_to_old)};
}

void packGeometry(const fs::path &input, const fs::path &output, const SpatialOrder &order)
{
    Archive archive(input);
    const auto nodes = archive.get<NodeID>(geometry_name);
    std::vector<NodeID> remapped(nodes.size());
    tbb::parallel_for(std::size_t{0},
                      nodes.size(),
                      [&](std::size_t i) { remapped[i] = order.old_to_new.at(nodes[i]); });

    FileWriter writer(output, FileWriter::GenerateFingerprint);
    archive.copyExcept(writer, geometry_name);
    writeDeltaVector(writer, std::string(geometry_name) + "_spatial", remapped);
    writer.Close();
}

void packSearchTree(const StorageConfig &input,
                    const StorageConfig &output,
                    const SpatialOrder &order)
{
    using Leaf = osrm::extractor::EdgeBasedNodeSegment;
    using Record = std::array<unsigned, 3>;
    static_assert(sizeof(Record) == 12);
    static_assert(sizeof(Leaf) == 20);

    const auto leaves_path = input.GetPath(".osrm.fileIndex");
    if (fs::file_size(leaves_path) % sizeof(Leaf) != 0)
        throw osrm::util::exception("Invalid road R-tree leaf file size");
    boost::iostreams::mapped_file_source map;
    const auto leaves = osrm::util::mmapFile<Leaf>(leaves_path, map);
    std::vector<unsigned> endpoints(leaves.size() * 2);
    std::vector<Record> records(leaves.size());
    tbb::parallel_for(std::size_t{0},
                      leaves.size(),
                      [&](std::size_t i)
                      {
                          const auto &leaf = leaves[i];
                          endpoints[2 * i] = order.old_to_new.at(leaf.u);
                          endpoints[2 * i + 1] = order.old_to_new.at(leaf.v);
                          records[i] = {std::bit_cast<unsigned>(leaf.forward_segment_id),
                                        std::bit_cast<unsigned>(leaf.reverse_segment_id),
                                        static_cast<unsigned>(leaf.fwd_segment_position) |
                                            (leaf.is_startpoint ? 32768u : 0u)};
                      });

    std::ofstream leaf_output(output.GetPath(".osrm.fileIndex"), std::ios::binary);
    leaf_output.exceptions(std::ios::failbit | std::ios::badbit);
    leaf_output.write(reinterpret_cast<const char *>(records.data()),
                      static_cast<std::streamsize>(records.size() * sizeof(Record)));
    leaf_output.close();

    FileWriter writer(output.GetPath(".osrm.ramIndex"), FileWriter::GenerateFingerprint);
    Archive(input.GetPath(".osrm.ramIndex")).copyExcept(writer);
    writeDeltaVector(writer, endpoints_name, endpoints);
    writer.Close();
}

void remapBearings(const fs::path &input, const fs::path &output, const SpatialOrder &order)
{
    Archive archive(input);
    const std::string classes_name = "/common/intersection_bearings/node_to_class_id";
    const auto narrow_classes_name = classes_name + "16";
    const bool narrow = archive.contains(narrow_classes_name);
    const auto &name = narrow ? narrow_classes_name : classes_name;

    FileWriter writer(output, FileWriter::GenerateFingerprint);
    archive.copyExcept(writer, name);
    const auto remap = [&]<typename T>()
    {
        const auto classes = archive.get<T>(name);
        if (classes.size() != order.new_to_old.size())
            throw osrm::util::exception("Invalid intersection bearing class count");
        std::vector<T> reordered(classes.size());
        tbb::parallel_for(std::size_t{0},
                          reordered.size(),
                          [&](std::size_t i) { reordered[i] = classes[order.new_to_old[i]]; });
        serial::write(writer, name, reordered);
    };

    if (narrow)
        remap.operator()<std::uint16_t>();
    else
        remap.operator()<BearingClassID>();
    writer.Close();
}

void remapManeuvers(const fs::path &input, const fs::path &output, const SpatialOrder &order)
{
    Archive archive(input);
    const auto original = archive.get<osrm::extractor::StorageManeuverOverride>(maneuver_name);
    std::vector<osrm::extractor::StorageManeuverOverride> remapped(original.begin(),
                                                                   original.end());
    for (auto &entry : remapped)
        entry.instruction_node = order.old_to_new.at(entry.instruction_node);

    FileWriter writer(output, FileWriter::GenerateFingerprint);
    archive.copyExcept(writer, maneuver_name);
    serial::write(writer, maneuver_name, remapped);
    writer.Close();
}
} // namespace

int main(int argc, char *argv[])
try
{
    if (argc == 2 && std::string_view(argv[1]) == "--help")
    {
        std::cout << "Usage: osrm-spatial-pack DATASET.osrm NEW_OUTPUT_DIRECTORY\n";
        return EXIT_SUCCESS;
    }
    if (argc != 3)
        throw osrm::util::exception("Usage: osrm-spatial-pack DATASET.osrm NEW_OUTPUT_DIRECTORY");

    const StorageConfig input(argv[1]);
    osrm::storage::requireMutableDataset(input.base_path);
    if (!input.IsValid() ||
        (!fs::exists(input.GetPath(".osrm.hsgr")) && !fs::exists(input.GetPath(".osrm.mldgr"))))
        throw osrm::util::exception("Expected a prepared CH or MLD routing dataset");
    if (fs::exists(input.GetPath(".osrm.mldgr")) &&
        (!fs::exists(input.GetPath(".osrm.cells")) ||
         !fs::exists(input.GetPath(".osrm.cell_metrics")) ||
         !fs::exists(input.GetPath(".osrm.partition"))))
        throw osrm::util::exception("Expected a complete MLD routing dataset");

    Storage storage(input);
    const auto static_files = storage.GetStaticFiles();
    const auto metric_files = storage.GetUpdatableFiles();
    const std::set<fs::path> replaced{input.GetPath(".osrm.nbg_nodes"),
                                      input.GetPath(".osrm.geometry"),
                                      input.GetPath(".osrm.ramIndex"),
                                      input.GetPath(".osrm.icd"),
                                      input.GetPath(".osrm.maneuver_overrides"),
                                      input.GetPath(".osrm.properties")};

    const fs::path directory(argv[2]);
    if (!fs::create_directory(directory))
        throw osrm::util::exception("Output directory already exists: " + directory.string());

    const auto marker = directory / osrm::storage::COMPACT_INCOMPLETE_FILE;
    {
        std::ofstream file(marker);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.close();
    }

    StorageConfig output;
    output.base_path = directory / input.base_path.filename();
    std::set<fs::path> files{input.GetPath(".osrm.turn_weight_penalties"),
                             input.GetPath(".osrm.turn_duration_penalties"),
                             input.GetPath(".osrm.openareas.fileIndex")};
    for (const auto &list : {static_files, metric_files})
        for (const auto &entry : list)
            files.insert(entry.second);

    for (const auto &file : files)
        if (!replaced.contains(file) && fs::exists(file))
            fs::copy_file(file, directory / file.filename());

    const auto order =
        packCoordinates(input.GetPath(".osrm.nbg_nodes"), output.GetPath(".osrm.nbg_nodes"));
    packGeometry(input.GetPath(".osrm.geometry"), output.GetPath(".osrm.geometry"), order);
    packSearchTree(input, output, order);
    remapBearings(input.GetPath(".osrm.icd"), output.GetPath(".osrm.icd"), order);
    remapManeuvers(input.GetPath(".osrm.maneuver_overrides"),
                   output.GetPath(".osrm.maneuver_overrides"),
                   order);

    {
        FileWriter properties(output.GetPath(".osrm.properties"), FileWriter::GenerateFingerprint);
        Archive(input.GetPath(".osrm.properties")).copyExcept(properties);
        properties.WriteElementCount64(osrm::storage::COMPACT_VERSION_ENTRY, 1);
        properties.WriteFrom(osrm::storage::COMPACT_VERSION_ENTRY,
                             osrm::storage::SPATIAL_PACKED_DATASET_VERSION);
        properties.Close();
    }
    fs::remove(marker);
    std::cout << output.base_path.string() << ".osrm\n";
    return EXIT_SUCCESS;
}
catch (const std::exception &error)
{
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
}
