#include "storage/compact.hpp"

#include "contractor/query_graph.hpp"
#include "customizer/edge_based_graph.hpp"
#include "customizer/shared_cell_metrics.hpp"
#include "extractor/edge_based_node.hpp"
#include "extractor/files.hpp"
#include "partitioner/files.hpp"
#include "storage/compact_dataset.hpp"
#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/storage.hpp"
#include "storage/view_factory.hpp"
#include "util/block_delta_encoding.hpp"
#include "util/log.hpp"
#include "util/mmap_file.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <memory>
#include <random>
#include <set>
#include <span>
#include <unordered_map>

namespace osrm::storage
{
namespace
{
struct CompactPaths
{
    StorageConfig Source;
    StorageConfig Metrics;
    StorageConfig Target;

    std::filesystem::path getInputPath(const std::string &suffix) const
    {
        return (suffix == ".osrm.mldgr" || suffix == ".osrm.cell_metrics") ? Metrics.GetPath(suffix)
                                                                           : Source.GetPath(suffix);
    }
};

CompactPaths getCompactPaths(const std::filesystem::path &input,
                             const std::filesystem::path &output,
                             const std::filesystem::path &mld_input)
{
    if (std::endian::native != std::endian::little || sizeof(void *) != 8)
        throw util::exception("Compact datasets currently require a 64-bit little-endian platform");
    const auto absolute_input = std::filesystem::absolute(input).lexically_normal();
    const auto absolute_mld =
        std::filesystem::absolute(mld_input.empty() ? input : mld_input).lexically_normal();
    const auto absolute_output = std::filesystem::absolute(output).lexically_normal();
    if (absolute_output.extension() != ".osrm")
        throw util::exception("Output must end in .osrm and name a new parent directory");
    CompactPaths paths{{absolute_input}, {absolute_mld}, {absolute_output}};
    const auto expected_properties = [](auto path)
    {
        if (path.extension() != ".osrm")
            path += ".osrm";
        return path.string() + ".properties";
    };
    if (paths.Source.GetPath(".osrm.properties") != expected_properties(absolute_input) ||
        paths.Metrics.GetPath(".osrm.properties") != expected_properties(absolute_mld) ||
        paths.Target.GetPath(".osrm.properties") != absolute_output.string() + ".properties")
        throw util::exception("Ambiguous dataset path: remove format extensions such as .osrm "
                              "from parent directory names.");
    return paths;
}

class LoadedCompactInput
{
  public:
    LoadedCompactInput(const CompactPaths &paths, std::initializer_list<const char *> suffixes)
    {
        std::vector<SharedDataIndex::AllocatedRegion> regions;
        for (const auto &suffix : suffixes)
        {
            const auto path = paths.getInputPath(suffix);
            auto layout = std::make_unique<TarDataLayout>();
            populateLayoutFromFile(path, *layout);
            io::FileReader reader(path, io::FileReader::HasNoFingerprint);
            const auto size = reader.GetSize();
            if (size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
                throw util::exception("Compaction archive exceeds the supported read size");
            auto data = std::make_unique<char[]>(size);
            reader.ReadInto(data.get(), size);
            regions.push_back({data.get(), std::move(layout)});
            m_buffers.push_back(std::move(data));
        }
        m_index = SharedDataIndex{std::move(regions)};
    }

    const SharedDataIndex &getIndex() const { return m_index; }

  private:
    std::vector<std::unique_ptr<char[]>> m_buffers;
    SharedDataIndex m_index;
};

class OutputDirectory
{
  public:
    explicit OutputDirectory(const std::filesystem::path &path) : m_path(path)
    {
        if (!std::filesystem::create_directory(m_path))
            throw util::exception("Output directory must be new: " + m_path.string());
        try
        {
            std::ofstream marker(m_path / COMPACT_INCOMPLETE_FILE);
            marker << "Compaction in progress\n";
            marker.close();
            if (!marker)
                throw util::exception("Cannot create compaction progress marker");
        }
        catch (...)
        {
            std::error_code error;
            std::filesystem::remove_all(m_path, error);
            throw;
        }
    }

    ~OutputDirectory()
    {
        if (!m_complete)
        {
            std::error_code error;
            std::filesystem::remove_all(m_path, error);
        }
    }

    void complete()
    {
        std::filesystem::remove(m_path / COMPACT_INCOMPLETE_FILE);
        m_complete = true;
    }

  private:
    std::filesystem::path m_path;
    bool m_complete = false;
};

template <typename Append>
void rewriteArchive(const std::filesystem::path &source,
                    const std::filesystem::path &target,
                    const std::vector<std::string> &drop,
                    Append append)
{
    tar::FileReader reader(source, tar::FileReader::VerifyFingerprint);
    boost::iostreams::mapped_file_source mapped;
    const auto bytes = util::mmapFile<char>(source, mapped);
    tar::FileWriter writer(target, tar::FileWriter::HasNoFingerprint);
    std::vector<tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));
    for (const auto &entry : entries)
    {
        const auto skip = std::any_of(drop.begin(),
                                      drop.end(),
                                      [&](const auto &name)
                                      {
                                          return entry.name == name ||
                                                 entry.name == name + ".meta" ||
                                                 entry.name.starts_with(name + "/");
                                      });
        if (skip)
            continue;
        if (entry.offset > bytes.size() || entry.size > bytes.size() - entry.offset)
            throw util::exception("Archive entry exceeds file bounds: " + entry.name);
        writer.WriteFrom(entry.name, bytes.data() + entry.offset, entry.size);
    }
    append(writer);
    writer.Close();
}

struct TurnKey
{
    std::uint64_t Guidance;
    std::uint32_t Penalties;
    bool operator==(const TurnKey &) const = default;
};

struct TurnHash
{
    std::size_t operator()(const TurnKey &key) const
    {
        auto hash = key.Guidance ^ (std::uint64_t(key.Penalties) * 0x9e3779b97f4a7c15ULL);
        hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
        hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
        return hash ^ (hash >> 31);
    }
};

struct Turns
{
    static constexpr std::array<const char *, 7> Names = {"/common/turn_data/turn_instructions",
                                                          "/common/turn_data/lane_data_ids",
                                                          "/common/turn_data/entry_class_ids",
                                                          "/common/turn_data/pre_turn_bearings",
                                                          "/common/turn_data/post_turn_bearings",
                                                          "/common/turn_penalty/weight",
                                                          "/common/turn_penalty/duration"};
    static constexpr std::array<unsigned, 7> Widths = {1, 2, 2, 1, 1, 2, 2};
    std::vector<std::uint32_t> Mapping;
    std::array<std::vector<unsigned char>, 7> Fields;
    std::uint64_t Unique = 0;
};

Turns canonicalizeTurns(const SharedDataIndex &index)
{
    Turns result;
    const auto count = index.GetBlockEntries(Turns::Names[0]);
    if (count > std::numeric_limits<std::uint32_t>::max())
        throw util::exception("Turn count exceeds the supported graph ID range");
    std::array<const unsigned char *, 7> input;
    for (unsigned field = 0; field < input.size(); ++field)
    {
        if (index.GetBlockEntries(Turns::Names[field]) != count ||
            index.GetBlockSize(Turns::Names[field]) != count * Turns::Widths[field])
            throw util::exception("Inconsistent turn record dimensions");
        input[field] = index.GetBlockPtr<unsigned char>(Turns::Names[field]);
    }
    std::unordered_map<TurnKey, std::uint32_t, TurnHash> dictionary;
    dictionary.reserve(count / 16);
    result.Mapping.resize(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        TurnKey key{};
        unsigned offset = 0;
        for (unsigned field = 0; field < 5; ++field)
            for (unsigned byte = 0; byte < Turns::Widths[field]; ++byte)
                key.Guidance |= std::uint64_t(input[field][i * Turns::Widths[field] + byte])
                                << (offset++ * 8);
        offset = 0;
        for (unsigned field = 5; field < 7; ++field)
            for (unsigned byte = 0; byte < 2; ++byte)
                key.Penalties |= std::uint32_t(input[field][i * 2 + byte]) << (offset++ * 8);
        const auto [entry, inserted] =
            dictionary.emplace(key, static_cast<std::uint32_t>(dictionary.size()));
        result.Mapping[i] = entry->second;
        for (unsigned field = 0; field < input.size(); ++field)
        {
            const auto width = Turns::Widths[field];
            if (inserted)
                result.Fields[field].insert(result.Fields[field].end(),
                                            input[field] + i * width,
                                            input[field] + (i + 1) * width);
            if (!std::equal(input[field] + i * width,
                            input[field] + (i + 1) * width,
                            result.Fields[field].data() + result.Mapping[i] * width))
                throw util::exception("Turn record reconstruction failed");
        }
    }
    result.Unique = dictionary.size();
    util::Log() << "Turns: " << count << " -> " << result.Unique << " exact records";
    return result;
}

void writeTurns(const StorageConfig &source, const StorageConfig &target, const Turns &turns)
{
    for (const auto &suffix :
         {".osrm.edges", ".osrm.turn_weight_penalties", ".osrm.turn_duration_penalties"})
    {
        std::vector<std::string> drop;
        const unsigned begin = std::string(suffix) == ".osrm.edges"                   ? 0
                               : std::string(suffix) == ".osrm.turn_weight_penalties" ? 5
                                                                                      : 6;
        const auto end = begin == 0 ? 5 : begin + 1;
        for (auto field = begin; field < end; ++field)
            drop.emplace_back(Turns::Names[field]);
        rewriteArchive(source.GetPath(suffix),
                       target.GetPath(suffix),
                       drop,
                       [&](auto &writer)
                       {
                           for (auto field = begin; field < end; ++field)
                           {
                               writer.WriteElementCount64(Turns::Names[field], turns.Unique);
                               writer.WriteFrom(Turns::Names[field],
                                                turns.Fields[field].data(),
                                                turns.Fields[field].size());
                           }
                       });
    }
}

void writeGraph(const std::filesystem::path &source_graph,
                const StorageConfig &target,
                const SharedDataIndex &index,
                const Turns &turns)
{
    using Graph = customizer::MultiLevelEdgeBasedGraphView;
    const std::string prefix = "/mld/multilevelgraph";
    const auto edge_name = prefix + "/edge_array";
    const auto count = index.GetBlockEntries(edge_name);
    const auto original = index.GetBlockPtr<Graph::EdgeArrayEntry>(edge_name);
    std::vector<Graph::EdgeArrayEntry> edges(original, original + count);
    bool narrow = true;
    for (auto &edge : edges)
    {
        if (edge.data.turn_id >= turns.Mapping.size())
            throw util::exception("Graph turn reference is out of bounds");
        edge.data.turn_id = turns.Mapping[edge.data.turn_id];
        narrow &= edge.target < (1u << 25) && edge.data.turn_id < (1u << 22);
    }
    std::vector<std::uint8_t> packed;
    if (narrow)
    {
        packed.resize(count * 6 + 8, 0);
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto bits =
                std::uint64_t(edges[i].target) | (std::uint64_t(edges[i].data.turn_id) << 25);
            const auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(bits)>>(bits);
            std::copy_n(bytes.begin(), 6, packed.data() + i * 6);
            std::array<std::uint8_t, sizeof(std::uint64_t)> restored_bytes;
            std::copy_n(packed.data() + i * 6, restored_bytes.size(), restored_bytes.begin());
            const auto restored = std::bit_cast<std::uint64_t>(restored_bytes);
            if ((restored & ((1ULL << 25) - 1)) != edges[i].target ||
                ((restored >> 25) & ((1ULL << 22) - 1)) != edges[i].data.turn_id)
                throw util::exception("Graph edge reconstruction failed");
        }
    }
    const auto node_name = prefix + "/node_array";
    const auto nodes = index.GetBlockPtr<Graph::NodeArrayEntry>(node_name);
    const std::uint64_t node_count = index.GetBlockEntries(node_name);
    std::vector<std::uint32_t> offsets(node_count);
    for (std::size_t i = 0; i < node_count; ++i)
        offsets[i] = nodes[i].first_edge;
    auto blocks = util::encodeBlockDelta<std::uint32_t>(offsets);
    if (blocks && blocks->Headers.size() * sizeof(util::BlockDeltaHeader) +
                          blocks->Words.size() * sizeof(std::uint64_t) >=
                      offsets.size() * sizeof(offsets[0]))
        blocks.reset();
    std::vector<std::string> drop{edge_name};
    if (blocks)
        drop.push_back(node_name);
    rewriteArchive(source_graph,
                   target.GetPath(".osrm.mldgr"),
                   drop,
                   [&](auto &writer)
                   {
                       if (narrow)
                           serialization::write(writer, edge_name + "6", packed);
                       else
                           serialization::write(writer, edge_name, edges);
                       if (blocks)
                       {
                           const auto name = prefix + "/node_blocks";
                           serialization::write(writer, name + "/headers", blocks->Headers);
                           serialization::write(writer, name + "/words", blocks->Words);
                           writer.WriteElementCount64(name + "/count", 1);
                           writer.WriteFrom(name + "/count", node_count);
                       }
                   });
    util::Log() << "Graph: " << (narrow ? "6-byte edges" : "full-width edges") << ", "
                << (blocks ? "compressed offsets" : "full-width offsets");
}

void writeContractedGraph(const CompactPaths &paths, const Turns &turns)
{
    const auto source = paths.Source.GetPath(".osrm.hsgr");
    if (!std::filesystem::exists(source))
        return;
    LoadedCompactInput input(paths, {".osrm.hsgr"});
    const auto &index = input.getIndex();
    extractor::ProfileProperties properties;
    extractor::files::readProfileProperties(paths.Source.GetPath(".osrm.properties"), properties);
    const auto name =
        "/ch/metrics/" + std::string(properties.GetWeightName()) + "/contracted_graph/edge_array";
    using Edge = contractor::QueryGraphView::EdgeArrayEntry;
    const auto original = index.GetBlockPtr<Edge>(name);
    std::vector<Edge> edges(original, original + index.GetBlockEntries(name));
    for (auto &edge : edges)
        if (!edge.data.shortcut)
        {
            if (edge.data.turn_id >= turns.Mapping.size())
                throw util::exception("CH graph turn reference is out of bounds");
            edge.data.turn_id = turns.Mapping[edge.data.turn_id];
        }
    rewriteArchive(source,
                   paths.Target.GetPath(".osrm.hsgr"),
                   {name},
                   [&](auto &writer) { serialization::write(writer, name, edges); });
}

void writeCellMetrics(const CompactPaths &paths)
{
    LoadedCompactInput input(paths, {".osrm.cell_metrics"});
    const auto &index = input.getIndex();
    extractor::ProfileProperties properties;
    extractor::files::readProfileProperties(paths.Source.GetPath(".osrm.properties"), properties);
    const auto prefix = "/mld/metrics/" + std::string(properties.GetWeightName());
    partitioner::CellStorage cells;
    partitioner::files::readCells(paths.Source.GetPath(".osrm.cells"), cells);
    const auto metrics = make_cell_metric_view(index, prefix);
    const auto shared = customizer::shareCellMetrics(cells.GetCellData(), metrics);
    tar::FileWriter writer(paths.Target.GetPath(".osrm.cell_metrics"),
                           tar::FileWriter::GenerateFingerprint);
    serialization::write(writer, prefix + "/shared/weights", shared.values.weights);
    serialization::write(writer, prefix + "/shared/durations", shared.values.durations);
    serialization::write(writer, prefix + "/shared/distances", shared.values.distances);
    writer.WriteElementCount64(prefix + "/exclude", shared.offsets.size());
    for (std::size_t i = 0; i < shared.offsets.size(); ++i)
        serialization::write(writer,
                             prefix + "/exclude/" + std::to_string(i) + "/cell_value_offsets",
                             shared.offsets[i]);
    writer.Close();
}

void writeNodes(const StorageConfig &source,
                const StorageConfig &target,
                const SharedDataIndex &index)
{
    const std::string name = "/common/ebg_node_data/nodes";
    const auto nodes = index.GetBlockPtr<extractor::EdgeBasedNode>(name);
    const auto count = index.GetBlockEntries(name);
    bool narrow = true;
    for (std::size_t i = 0; i < count; ++i)
        narrow &= nodes[i].geometry_id.id <= 0xffffff && nodes[i].component_id.id <= 0x1ffff &&
                  nodes[i].annotation_id <= 0xfffff;
    if (!narrow)
    {
        std::filesystem::copy_file(source.GetPath(".osrm.ebg_nodes"),
                                   target.GetPath(".osrm.ebg_nodes"));
        util::Log() << "Node metadata: retaining full-width records";
        return;
    }
    std::vector<std::uint64_t> packed(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto &node = nodes[i];
        packed[i] =
            std::uint64_t(node.geometry_id.id) | (std::uint64_t(node.geometry_id.forward) << 24) |
            (std::uint64_t(node.component_id.id) << 25) |
            (std::uint64_t(node.component_id.is_tiny) << 42) |
            (std::uint64_t(node.annotation_id) << 43) | (std::uint64_t(node.segregated) << 63);
    }
    rewriteArchive(source.GetPath(".osrm.ebg_nodes"),
                   target.GetPath(".osrm.ebg_nodes"),
                   {name},
                   [&](auto &writer) { serialization::write(writer, name + "8", packed); });
}

void writeBearings(const StorageConfig &source,
                   const StorageConfig &target,
                   const SharedDataIndex &index)
{
    const std::string name = "/common/intersection_bearings/node_to_class_id";
    const auto values = index.GetBlockPtr<std::uint32_t>(name);
    const auto count = index.GetBlockEntries(name);
    for (std::size_t i = 0; i < count; ++i)
        if (values[i] != 0xffffffff && values[i] >= 65535)
        {
            std::filesystem::copy_file(source.GetPath(".osrm.icd"), target.GetPath(".osrm.icd"));
            util::Log() << "Bearing references: retaining full-width IDs";
            return;
        }
    std::vector<std::uint16_t> packed(count);
    for (std::size_t i = 0; i < count; ++i)
        packed[i] = values[i] == 0xffffffff ? 65535 : values[i];
    rewriteArchive(source.GetPath(".osrm.icd"),
                   target.GetPath(".osrm.icd"),
                   {name},
                   [&](auto &writer) { serialization::write(writer, name + "16", packed); });
}

void writePartition(const StorageConfig &source,
                    const StorageConfig &target,
                    const SharedDataIndex &index)
{
    const std::string name = "/mld/multilevelpartition/partition";
    const auto values = index.GetBlockPtr<PartitionID>(name);
    const std::uint64_t count = index.GetBlockEntries(name);
    const auto blocks = util::encodeBlockDelta<PartitionID>({values, count});
    if (!blocks || blocks->Headers.size() * sizeof(util::BlockDeltaHeader) +
                           blocks->Words.size() * sizeof(std::uint64_t) >=
                       count * sizeof(PartitionID))
    {
        std::filesystem::copy_file(source.GetPath(".osrm.partition"),
                                   target.GetPath(".osrm.partition"));
        util::Log() << "Partition: retaining full-width IDs";
        return;
    }
    rewriteArchive(source.GetPath(".osrm.partition"),
                   target.GetPath(".osrm.partition"),
                   {name},
                   [&](auto &writer)
                   {
                       const auto prefix = name + "_blocks";
                       serialization::write(writer, prefix + "/headers", blocks->Headers);
                       serialization::write(writer, prefix + "/words", blocks->Words);
                       writer.WriteElementCount64(prefix + "/count", 1);
                       writer.WriteFrom(prefix + "/count", count);
                   });
}

struct GeometryCosts
{
    std::vector<std::uint64_t> Descriptors;
    std::vector<std::uint64_t> Words{0};
    std::uint64_t Entries = 0;
};

std::optional<GeometryCosts> encodeGeometryCosts(const extractor::SegmentDataView &segment)
{
    GeometryCosts result;
    const auto count = segment.GetNumberOfGeometries();
    result.Descriptors.reserve(count + 1);
    std::vector<std::array<std::uint32_t, 4>> values;
    std::uint64_t bit = 0;
    for (std::size_t geometry = 0; geometry < count; ++geometry)
    {
        const auto fw = segment.GetForwardWeights(geometry);
        const auto rw = segment.GetReverseWeights(geometry);
        const auto fd = segment.GetForwardDurations(geometry);
        const auto rd = segment.GetReverseDurations(geometry);
        const auto length = fw.size();
        if (length > 65535 || bit > 0xffffffffULL)
            return std::nullopt;
        if (fd.size() != length || rw.size() != length || rd.size() != length)
            throw util::exception("Geometry cost dimensions do not agree");
        values.resize(length);
        std::array<std::array<bool, 4>, 4> equal;
        for (auto &row : equal)
            row.fill(true);
        std::uint32_t max_finite = 0;
        bool sentinel = false;
        for (std::size_t k = 0; k < length; ++k)
        {
            values[k] = {static_cast<std::uint32_t>(fw[k]),
                         static_cast<std::uint32_t>(fd[k]),
                         static_cast<std::uint32_t>(rw[length - 1 - k]),
                         static_cast<std::uint32_t>(rd[length - 1 - k])};
            for (auto value : values[k])
                if (value == 4194303)
                    sentinel = true;
                else
                    max_finite = std::max(max_finite, value);
            for (unsigned a = 0; a < 4; ++a)
                for (unsigned b = 0; b < a; ++b)
                    equal[a][b] &= values[k][a] == values[k][b];
        }
        std::array<unsigned, 4> representatives{}, ordinals{};
        unsigned unique = 0;
        for (unsigned a = 0; a < 4; ++a)
        {
            bool found = false;
            for (unsigned b = 0; b < unique; ++b)
                if (equal[a][representatives[b]])
                {
                    ordinals[a] = b;
                    found = true;
                    break;
                }
            if (!found)
            {
                ordinals[a] = unique;
                representatives[unique++] = a;
            }
        }
        const auto width = std::bit_width(std::uint64_t(max_finite) + unsigned(sentinel));
        if (width > 22)
            return std::nullopt;
        const auto final_bit = bit + std::uint64_t(unique) * length * width;
        if (final_bit > 0xffffffffULL)
            return std::nullopt;
        const auto base = bit;
        const std::uint64_t mask = (1ULL << width) - 1;
        result.Descriptors.push_back(
            base | (std::uint64_t(length) << 32) | (std::uint64_t(ordinals[1]) << 48) |
            (std::uint64_t(ordinals[2]) << 50) | (std::uint64_t(ordinals[3]) << 52) |
            (std::uint64_t(width) << 54) | (std::uint64_t(sentinel) << 59) | (1ULL << 63));
        result.Words.resize((final_bit + 63) / 64 + 1, 0);
        for (unsigned a = 0; a < unique; ++a)
            for (std::size_t k = 0; k < length; ++k)
            {
                const std::uint64_t value =
                    values[k][representatives[a]] == 4194303 ? mask : values[k][representatives[a]];
                result.Words[bit / 64] |= value << (bit % 64);
                if (bit % 64 + width > 64)
                    result.Words[bit / 64 + 1] |= value >> (64 - bit % 64);
                bit += width;
            }
        for (unsigned a = 0; a < 4; ++a)
            for (std::size_t k = 0; k < length; ++k)
            {
                const auto position = base + (ordinals[a] * length + k) * width;
                auto value = result.Words[position / 64] >> (position % 64);
                if (position % 64 + width > 64)
                    value |= result.Words[position / 64 + 1] << (64 - position % 64);
                value &= mask;
                if (sentinel && value == mask)
                    value = 4194303;
                if (value != values[k][a])
                    throw util::exception("Geometry cost reconstruction failed");
            }
        result.Entries += std::uint64_t(unique) * length;
    }
    result.Descriptors.push_back(result.Entries);
    return result;
}

void writeGeometry(const StorageConfig &source,
                   const StorageConfig &target,
                   const SharedDataIndex &index)
{
    const std::string prefix = "/common/segment_data";
    const auto segment = make_segment_data_view(index, prefix);
    auto costs = encodeGeometryCosts(segment);
    std::size_t original_cost_bytes = 0;
    std::vector<std::string> cost_names;
    for (const auto &role :
         {"forward_weights", "reverse_weights", "forward_durations", "reverse_durations"})
    {
        cost_names.push_back(prefix + "/" + role);
        original_cost_bytes += index.GetBlockSize(cost_names.back() + "/packed");
    }
    if (costs && (costs->Descriptors.size() + costs->Words.size()) * sizeof(std::uint64_t) >=
                     original_cost_bytes)
        costs.reset();
    std::vector<std::string> drop = costs ? cost_names : std::vector<std::string>{};
    unsigned max_length = 0;
    const auto offsets = index.GetBlockPtr<std::uint32_t>(prefix + "/index");
    for (std::size_t geometry = 0; geometry < segment.GetNumberOfGeometries(); ++geometry)
    {
        if (offsets[geometry + 1] <= offsets[geometry])
            throw util::exception("Invalid geometry length");
        max_length = std::max(max_length, offsets[geometry + 1] - offsets[geometry] - 1);
    }
    std::array<bool, 2> constant;
    for (unsigned direction = 0; direction < 2; ++direction)
    {
        const auto name =
            prefix + (direction == 0 ? "/forward_data_sources" : "/reverse_data_sources");
        const auto values = index.GetBlockPtr<DatasourceID>(name);
        const auto count = index.GetBlockEntries(name);
        constant[direction] =
            max_length < count &&
            std::all_of(values, values + count, [](auto value) { return value == 0; });
        if (constant[direction])
            drop.push_back(name);
    }
    const std::vector<DatasourceID> zero(max_length, 0);
    rewriteArchive(source.GetPath(".osrm.geometry"),
                   target.GetPath(".osrm.geometry"),
                   drop,
                   [&](auto &writer)
                   {
                       if (costs)
                       {
                           const auto name = prefix + "/adaptive_costs";
                           serialization::write(writer, name + "/descriptors", costs->Descriptors);
                           serialization::write(writer, name + "/packed", costs->Words);
                           writer.WriteFrom(name + "/number_of_elements.meta", costs->Entries);
                       }
                       for (unsigned direction = 0; direction < 2; ++direction)
                           if (constant[direction])
                               serialization::write(writer,
                                                    prefix + (direction == 0
                                                                  ? "/constant_forward_sources"
                                                                  : "/constant_reverse_sources"),
                                                    zero);
                   });
    util::Log() << "Geometry costs: " << (costs ? "shared adaptive widths" : "original widths")
                << "; zero datasource spans: " << (constant[0] + constant[1]);
}
} // namespace

void validateCompactDatasetPaths(const std::filesystem::path &input,
                                 const std::filesystem::path &output,
                                 const std::filesystem::path &mld_input)
{
    const auto paths = getCompactPaths(input, output, mld_input);
    requireMutableDataset(paths.Source.base_path);
    requireMutableDataset(paths.Metrics.base_path);
    const auto parent = std::filesystem::absolute(output).lexically_normal().parent_path();
    if (std::filesystem::exists(parent) || std::filesystem::is_symlink(parent))
        throw util::exception("Output directory must be new: " + parent.string());
    if (!std::filesystem::is_directory(parent.parent_path()))
        throw util::exception("Output directory's parent must exist: " +
                              parent.parent_path().string());
}

namespace
{
void writeCompactDataset(const std::filesystem::path &input,
                         const std::filesystem::path &output,
                         const std::filesystem::path &mld_input,
                         bool copy_unchanged)
{
    validateCompactDatasetPaths(input, output, mld_input);
    const auto paths = getCompactPaths(input, output, mld_input);
    const auto &source = paths.Source;
    const auto &target = paths.Target;
    const auto absolute_output = std::filesystem::absolute(output).lexically_normal();
    if (!source.IsValid())
        throw util::exception("Input is missing required query files");
    for (const auto &suffix :
         {".osrm.partition", ".osrm.cells", ".osrm.cell_metrics", ".osrm.mldgr"})
        if (!std::filesystem::is_regular_file(paths.getInputPath(suffix)))
            throw util::exception(std::string("Missing MLD input: ") + suffix);

    OutputDirectory output_directory(absolute_output.parent_path());
    {
        const auto turns = [&]
        {
            LoadedCompactInput input(
                paths,
                {".osrm.edges", ".osrm.turn_weight_penalties", ".osrm.turn_duration_penalties"});
            const auto &index = input.getIndex();
            tar::FileReader graph_reader(paths.getInputPath(".osrm.mldgr"),
                                         tar::FileReader::VerifyFingerprint);
            std::uint32_t graph_checksum;
            graph_reader.ReadInto("/mld/connectivity_checksum", graph_checksum);
            if (*index.GetBlockPtr<std::uint32_t>("/common/connectivity_checksum") !=
                graph_checksum)
                throw util::exception("Graph and guidance connectivity checksums do not match");
            auto result = canonicalizeTurns(index);
            writeTurns(source, target, result);
            return result;
        }();
        writeContractedGraph(paths, turns);
        LoadedCompactInput graph_input(paths, {".osrm.mldgr"});
        writeGraph(paths.getInputPath(".osrm.mldgr"), target, graph_input.getIndex(), turns);
    }
    const auto run_stage = [&](const char *suffix, auto writer)
    {
        LoadedCompactInput input(paths, {suffix});
        writer(source, target, input.getIndex());
    };
    run_stage(".osrm.ebg_nodes", writeNodes);
    run_stage(".osrm.icd", writeBearings);
    run_stage(".osrm.partition", writePartition);
    run_stage(".osrm.geometry", writeGeometry);
    writeCellMetrics(paths);
    rewriteArchive(source.GetPath(".osrm.properties"),
                   target.GetPath(".osrm.properties"),
                   {},
                   [](auto &writer)
                   {
                       writer.WriteElementCount64(COMPACT_VERSION_ENTRY, 1);
                       writer.WriteFrom(COMPACT_VERSION_ENTRY, COMPACT_DATASET_VERSION);
                   });

    const std::set<std::string> rewritten = {".osrm.edges",
                                             ".osrm.turn_weight_penalties",
                                             ".osrm.turn_duration_penalties",
                                             ".osrm.mldgr",
                                             ".osrm.ebg_nodes",
                                             ".osrm.icd",
                                             ".osrm.partition",
                                             ".osrm.geometry",
                                             ".osrm.cell_metrics",
                                             ".osrm.hsgr",
                                             ".osrm.properties"};
    std::vector<std::pair<bool, std::filesystem::path>> files;
    if (copy_unchanged)
    {
        for (const auto &suffix : GetRequiredFiles({}))
            files.emplace_back(true, source.GetPath(suffix.string()));
        for (const auto &suffix : {".osrm.cells",
                                   ".osrm.openareas",
                                   ".osrm.openareas.ramIndex",
                                   ".osrm.openareas.fileIndex"})
            files.emplace_back(false, source.GetPath(suffix));
    }
    for (const auto &[required, common_path] : files)
    {
        const auto suffix = common_path.string().substr(source.base_path.string().size());
        const auto path = paths.getInputPath(suffix);
        if (rewritten.contains(suffix) || suffix == ".osrm.query")
            continue;
        if (required || std::filesystem::exists(path))
        {
            if (suffix != ".osrm.fileIndex" && suffix != ".osrm.openareas.fileIndex")
            {
                tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);
            }
            std::filesystem::copy_file(path, target.GetPath(suffix));
        }
    }
    output_directory.complete();
    util::Log() << "Prepared compact query dataset: " << absolute_output.string();
}
} // namespace

void compactDataset(const std::filesystem::path &input,
                    const std::filesystem::path &output,
                    const std::filesystem::path &mld_input)
{ writeCompactDataset(input, output, mld_input, true); }

void writeQueryData(const std::filesystem::path &input)
{
    if (std::endian::native != std::endian::little || sizeof(void *) != 8)
        return;
    StorageConfig source(input);
    if (!std::filesystem::exists(source.GetPath(".osrm.mldgr")) ||
        !std::filesystem::exists(source.GetPath(".osrm.cell_metrics")))
        return;
    invalidateQueryData(source.base_path);
    std::uint32_t guidance_checksum;
    {
        tar::FileReader reader(source.GetPath(".osrm.edges"), tar::FileReader::VerifyFingerprint);
        reader.ReadInto("/common/connectivity_checksum", guidance_checksum);
    }
    for (const auto &[suffix, entry] : {std::pair{".osrm.mldgr", "/mld/connectivity_checksum"},
                                        std::pair{".osrm.hsgr", "/ch/connectivity_checksum"}})
    {
        const auto path = source.GetPath(suffix);
        if (!std::filesystem::exists(path))
            continue;
        tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);
        std::uint32_t graph_checksum;
        reader.ReadInto(entry, graph_checksum);
        if (graph_checksum != guidance_checksum)
        {
            util::Log() << "Deferring compact query data until both prepared graphs match the "
                           "guidance data.";
            return;
        }
    }
    std::random_device random;
    const auto nonce = (std::uint64_t(random()) << 32) | std::uint64_t(random());
    const auto stage =
        source.base_path.parent_path() / ("osrm-query-stage-" + std::to_string(nonce));
    const auto output = stage / "data.osrm";
    bool stage_created = false;
    try
    {
        const auto ch_path = source.GetPath(".osrm.hsgr");
        if (std::filesystem::exists(ch_path) &&
            readQueryGeneration(ch_path, CH_GENERATION_ENTRY) == 0)
        {
            const auto generation = nextQueryGeneration(ch_path, CH_GENERATION_ENTRY);
            const auto stamped = ch_path.string() + ".query-generation";
            rewriteArchive(ch_path,
                           stamped,
                           {},
                           [&](auto &writer)
                           {
                               writer.WriteElementCount64(CH_GENERATION_ENTRY, 1);
                               writer.WriteFrom(CH_GENERATION_ENTRY, generation);
                           });
#ifdef _WIN32
            std::filesystem::copy_file(
                stamped, ch_path, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(stamped);
#else
            std::filesystem::rename(stamped, ch_path);
#endif
        }
        writeCompactDataset(input, output, {}, false);
        stage_created = true;
        const auto archive_path = stage / "query.archive";
        {
            tar::FileWriter writer(archive_path, tar::FileWriter::GenerateFingerprint);
            writer.WriteElementCount64(COMPACT_VERSION_ENTRY, 1);
            writer.WriteFrom(COMPACT_VERSION_ENTRY, COMPACT_DATASET_VERSION);
            writer.WriteElementCount64(MLD_GENERATION_ENTRY, 1);
            writer.WriteFrom(
                MLD_GENERATION_ENTRY,
                readQueryGeneration(source.GetPath(".osrm.mldgr"), MLD_GENERATION_ENTRY));
            writer.WriteElementCount64(CH_GENERATION_ENTRY, 1);
            writer.WriteFrom(CH_GENERATION_ENTRY,
                             readQueryGeneration(ch_path, CH_GENERATION_ENTRY));
            for (const auto &suffix : {".edges",
                                       ".turn_weight_penalties",
                                       ".turn_duration_penalties",
                                       ".mldgr",
                                       ".ebg_nodes",
                                       ".icd",
                                       ".partition",
                                       ".geometry",
                                       ".cell_metrics",
                                       ".hsgr",
                                       ".datasource_names"})
            {
                const auto path = std::string(suffix) == ".datasource_names"
                                      ? source.GetPath(".osrm.datasource_names")
                                      : std::filesystem::path(output.string() + suffix);
                if (!std::filesystem::exists(path))
                    continue;
                tar::FileReader reader(path, tar::FileReader::VerifyFingerprint);
                boost::iostreams::mapped_file_source mapped;
                const auto bytes = util::mmapFile<char>(path, mapped);
                std::vector<tar::FileReader::FileEntry> entries;
                reader.List(std::back_inserter(entries));
                for (const auto &entry : entries)
                {
                    if (entry.name == "osrm_fingerprint.meta" ||
                        entry.name == MLD_GENERATION_ENTRY ||
                        entry.name == std::string(MLD_GENERATION_ENTRY) + ".meta" ||
                        entry.name == CH_GENERATION_ENTRY ||
                        entry.name == std::string(CH_GENERATION_ENTRY) + ".meta")
                        continue;
                    if (entry.offset > bytes.size() || entry.size > bytes.size() - entry.offset)
                        throw util::exception("Archive entry exceeds file bounds: " + entry.name);
                    writer.WriteFrom(entry.name, bytes.data() + entry.offset, entry.size);
                }
            }
            writer.Close();
        }
        const auto query_path = source.GetPath(".osrm.query");
#ifdef _WIN32
        std::filesystem::remove(query_path);
#endif
        std::filesystem::rename(archive_path, query_path);
        std::filesystem::remove_all(stage);
        util::Log() << "Prepared compact query archive: " << query_path.string();
    }
    catch (...)
    {
        std::error_code error;
        if (stage_created)
            std::filesystem::remove_all(stage, error);
        throw;
    }
}
} // namespace osrm::storage
