#include "customizer/edge_based_graph.hpp"
#include "storage/tar.hpp"
#include "engine/datafacade/process_memory_allocator.hpp"
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <unordered_map>

struct TurnKey
{
    std::uint64_t Guidance;
    std::uint32_t Penalties;
    bool operator==(const TurnKey &) const = default;
};
struct TurnHash
{
    std::size_t operator()(const TurnKey &v) const
    {
        auto h = v.Guidance ^ (std::uint64_t(v.Penalties) * 0x9e3779b97f4a7c15ull);
        h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ull;
        h = (h ^ (h >> 27)) * 0x94d049bb133111ebull;
        return h ^ (h >> 31);
    }
};
struct Replacement
{
    const void *Data;
    std::size_t Bytes;
};
void rewrite(const std::filesystem::path &source,
             const std::filesystem::path &target,
             const std::unordered_map<std::string, Replacement> &replacements)
{
    if (std::filesystem::exists(target))
        throw std::runtime_error("Output exists");
    osrm::storage::tar::FileReader reader(source,
                                          osrm::storage::tar::FileReader::VerifyFingerprint);
    osrm::storage::tar::FileWriter writer(target, osrm::storage::tar::FileWriter::HasNoFingerprint);
    std::vector<osrm::storage::tar::FileReader::FileEntry> entries;
    reader.List(std::back_inserter(entries));
    for (const auto &entry : entries)
    {
        if (const auto it = replacements.find(entry.name); it != replacements.end())
            writer.WriteFrom(
                entry.name, static_cast<const char *>(it->second.Data), it->second.Bytes);
        else
        {
            std::vector<char> bytes(entry.size);
            reader.ReadInto(entry.name, bytes.data(), bytes.size());
            writer.WriteFrom(entry.name, bytes.data(), bytes.size());
        }
    }
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    const std::filesystem::path source = std::filesystem::absolute(argv[1]);
    const std::filesystem::path target = std::filesystem::absolute(argv[2]);
    if (std::filesystem::exists(source.string() + ".hsgr"))
        throw std::runtime_error("MLD-only conversion");
    if (!std::filesystem::create_directory(target.parent_path()))
        throw std::runtime_error("Output directory must be new");
    const auto start = std::chrono::steady_clock::now();
    osrm::engine::datafacade::ProcessMemoryAllocator allocator({source}, true);
    const auto &index = allocator.GetIndex();
    const std::array<std::string, 7> names = {"/common/turn_data/turn_instructions",
                                              "/common/turn_data/lane_data_ids",
                                              "/common/turn_data/entry_class_ids",
                                              "/common/turn_data/pre_turn_bearings",
                                              "/common/turn_data/post_turn_bearings",
                                              "/common/turn_penalty/weight",
                                              "/common/turn_penalty/duration"};
    const std::array<unsigned, 7> widths = {1, 2, 2, 1, 1, 2, 2};
    const auto count = index.GetBlockEntries(names[0]);
    std::array<const unsigned char *, 7> input;
    for (unsigned f = 0; f < 7; ++f)
    {
        if (index.GetBlockEntries(names[f]) != count ||
            index.GetBlockSize(names[f]) != count * widths[f])
            throw std::runtime_error("Tuple dimensions");
        input[f] = index.GetBlockPtr<unsigned char>(names[f]);
    }
    std::unordered_map<TurnKey, std::uint32_t, TurnHash> dictionary;
    dictionary.reserve(count / 16);
    std::vector<std::uint32_t> mapping(count);
    std::array<std::vector<unsigned char>, 7> fields;
    for (std::size_t i = 0; i < count; ++i)
    {
        TurnKey key{};
        unsigned offset = 0;
        for (unsigned f = 0; f < 5; ++f)
            for (unsigned b = 0; b < widths[f]; ++b)
                key.Guidance |= std::uint64_t(input[f][i * widths[f] + b]) << (offset++ * 8);
        offset = 0;
        for (unsigned f = 5; f < 7; ++f)
            for (unsigned b = 0; b < 2; ++b)
                key.Penalties |= std::uint32_t(input[f][i * 2 + b]) << (offset++ * 8);
        const auto [it, inserted] = dictionary.emplace(key, dictionary.size());
        mapping[i] = it->second;
        if (inserted)
            for (unsigned f = 0; f < 7; ++f)
                fields[f].insert(
                    fields[f].end(), input[f] + i * widths[f], input[f] + (i + 1) * widths[f]);
        for (unsigned f = 0; f < 7; ++f)
            if (std::memcmp(
                    input[f] + i * widths[f], fields[f].data() + mapping[i] * widths[f], widths[f]))
                throw std::runtime_error("Tuple mismatch");
    }
    using Edge = osrm::customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry;
    const auto edge_count = index.GetBlockEntries("/mld/multilevelgraph/edge_array");
    const auto original = index.GetBlockPtr<Edge>("/mld/multilevelgraph/edge_array");
    std::vector<Edge> edges(original, original + edge_count);
    for (std::size_t i = 0; i < edge_count; ++i)
    {
        if (original[i].data.turn_id >= mapping.size())
            throw std::runtime_error("Turn reference bounds");
        edges[i].data.turn_id = mapping[original[i].data.turn_id];
        if (edges[i].target != original[i].target)
            throw std::runtime_error("Changed target");
    }
    const std::uint64_t unique = dictionary.size();
    std::unordered_map<std::string, Replacement> replacements;
    for (unsigned f = 0; f < 7; ++f)
    {
        replacements.emplace(names[f], Replacement{fields[f].data(), fields[f].size()});
        replacements.emplace(names[f] + ".meta", Replacement{&unique, sizeof(unique)});
    }
    replacements.emplace("/mld/multilevelgraph/edge_array",
                         Replacement{edges.data(), edges.size() * sizeof(Edge)});
    const std::set<std::string> changed = {
        ".edges", ".turn_weight_penalties", ".turn_duration_penalties", ".mldgr"};
    for (const auto &p : std::filesystem::directory_iterator(source.parent_path()))
    {
        const auto name = p.path().filename().string();
        const auto prefix = source.filename().string();
        if (!name.starts_with(prefix + "."))
            continue;
        const auto extension = name.substr(prefix.size());
        const auto output = target.string() + extension;
        if (changed.contains(extension))
            rewrite(p.path(), output, replacements);
        else
            std::filesystem::create_symlink(p.path(), output);
    }
    std::cout << "{\"original_turns\":" << count << ",\"unique_turns\":" << unique
              << ",\"verified_graph_references\":" << edge_count
              << ",\"tuple_mismatches\":0,\"conversion_seconds\":"
              << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
              << "}" << std::endl;
}
