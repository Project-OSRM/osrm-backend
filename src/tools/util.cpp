#include "storage/shared_data_index.hpp"
#include "storage/view_factory.hpp"

#include "engine/datafacade/mmap_memory_allocator.hpp"
#include "osrm/storage_config.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <csignal>
#include <cstdlib>

using namespace osrm;

void dump_ch(const std::string &filename) {
    std::cout << filename << std::endl;

    StorageConfig config(filename);
    engine::datafacade::MMapMemoryAllocator allocator(config);
    
    auto &index = allocator.GetIndex();

    //auto exclude_prefix = name + "/exclude/" + std::to_string(exclude_index);
    //auto edge_filter = make_vector_view<bool>(index, exclude_prefix + "/edge_filter");
    auto node_list = storage::make_vector_view<contractor::QueryGraphView::NodeArrayEntry>(
        index, "/ch/metrics/routability/contracted_graph/node_array");
    auto edge_list = storage::make_vector_view<contractor::QueryGraphView::EdgeArrayEntry>(
        index, "/ch/metrics/routability/contracted_graph/edge_array");

    std::cout << "digraph {" << std::endl;
    for (std::size_t i = 0; i < node_list.size(); i++)
    {
        for (auto edge = node_list[i].first_edge;
             edge < (i == node_list.size() - 1 ? edge_list.size() : node_list[i + 1].first_edge);
             edge++)
        {
            const auto &e = edge_list[edge];
            if (e.data.forward)
            {
                std::cout << i << " -> " << e.target;
                std::cout << "[";
                std::cout << "label=\"↑" << e.data.weight << "\",weight=\"" << e.data.weight
                          << "\"";
                if (e.data.maneuver_restricted)
                {
                    std::cout << ",color=red,penwidth=3.0";
                }
                std::cout << "];";
                std::cout << std::endl;
            }
            if (e.data.backward)
            {
                std::cout << e.target << " -> " << i;
                std::cout << "[";
                std::cout << "label=\"↓" << e.data.weight << "\",weight=\"" << e.data.weight
                          << "\"";
                if (e.data.maneuver_restricted)
                {
                    std::cout << ",color=red,penwidth=3.0";
                }
                std::cout << "];";
                std::cout << std::endl;
            }
        }
    }
    std::cout << "}" << std::endl;
}

int main(const int argc, const char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <command> <filename.osrm>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Commands:" << std::endl;
        std::cerr << "    dump-ch  - dump the CH graph as a GraphViz .dot file" << std::endl;
        return EXIT_FAILURE;
    }

    if ("dump-ch" != std::string(argv[1])) {
        util::Log(logERROR) << "Invalid command '" << argv[1] << "'" << std::endl;
        return EXIT_FAILURE;
    }

    boost::filesystem::path p(std::string(argv[2]) + ".hsgr");
    if (!boost::filesystem::exists(p)) {
        util::Log(logERROR) << "File '" << argv[2] << "' does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    // If we make it to here, do it
    dump_ch(std::string(argv[2]));
}