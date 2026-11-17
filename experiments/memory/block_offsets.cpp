#include "conversion.hpp"
#include "customizer/edge_based_graph.hpp"
#include "encode_blocks.hpp"
int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    const auto start = std::chrono::steady_clock::now();
    const std::filesystem::path source = std::filesystem::absolute(argv[1]),
                                target = std::filesystem::absolute(argv[2]);
    osrm::engine::datafacade::ProcessMemoryAllocator allocator({source}, true);
    const auto &index = allocator.GetIndex();
    const std::string name = "/mld/multilevelgraph/node_array";
    using Node = osrm::customizer::MultiLevelEdgeBasedGraphView::NodeArrayEntry;
    const auto nodes = index.GetBlockPtr<Node>(name);
    const std::uint64_t count = index.GetBlockEntries(name);
    std::vector<std::uint32_t> values(count);
    for (std::size_t i = 0; i < count; ++i)
        values[i] = nodes[i].first_edge;
    const auto encoded = encodeBlocks(values.data(), count);
    cloneQueryData(
        source,
        target,
        ".mldgr",
        {name},
        [&](auto &writer)
        {
            const std::string prefix = "/mld/multilevelgraph/node_blocks";
            osrm::storage::serialization::write(writer, prefix + "/headers", encoded.headers);
            osrm::storage::serialization::write(writer, prefix + "/words", encoded.words);
            writer.WriteElementCount64(prefix + "/count", 1);
            writer.WriteFrom(prefix + "/count", count);
        });
    struct rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    std::cout << "{\"records\":" << count << ",\"mismatches\":0,\"encoded_bytes\":"
              << encoded.headers.size() * 16 + encoded.words.size() * 8
              << ",\"peak_rss_bytes\":" << usage.ru_maxrss << ",\"conversion_seconds\":"
              << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
              << "}" << std::endl;
}
