#include "conversion.hpp"
#include "customizer/edge_based_graph.hpp"
int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    const auto start = std::chrono::steady_clock::now();
    const std::filesystem::path source = std::filesystem::absolute(argv[1]),
                                target = std::filesystem::absolute(argv[2]);
    osrm::engine::datafacade::ProcessMemoryAllocator allocator({source}, true);
    const auto &index = allocator.GetIndex();
    const std::string name = "/mld/multilevelgraph/edge_array";
    using Edge = osrm::customizer::MultiLevelEdgeBasedGraphView::EdgeArrayEntry;
    const auto edges = index.GetBlockPtr<Edge>(name);
    const auto count = index.GetBlockEntries(name);
    std::vector<std::uint8_t> packed(count * 6 + 8, 0);
    for (std::size_t i = 0; i < count; ++i)
    {
        if (edges[i].target >= (1u << 25) || edges[i].data.turn_id >= (1u << 22))
            throw std::runtime_error("Edge width overflow: retain original");
        const auto bits =
            std::uint64_t(edges[i].target) | (std::uint64_t(edges[i].data.turn_id) << 25);
        std::memcpy(packed.data() + i * 6, &bits, 6);
    }
    for (std::size_t i = 0; i < count; ++i)
    {
        std::uint64_t bits;
        std::memcpy(&bits, packed.data() + i * 6, 8);
        if ((bits & ((1ull << 25) - 1)) != edges[i].target ||
            ((bits >> 25) & ((1ull << 22) - 1)) != edges[i].data.turn_id)
            throw std::runtime_error("Edge roundtrip mismatch");
    }
    cloneQueryData(source,
                   target,
                   ".mldgr",
                   {name},
                   [&](auto &writer)
                   { osrm::storage::serialization::write(writer, name + "6", packed); });
    struct rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    std::cout << "{\"records\":" << count
              << ",\"mismatches\":0,\"peak_rss_bytes\":" << usage.ru_maxrss
              << ",\"conversion_seconds\":"
              << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
              << "}" << std::endl;
}
