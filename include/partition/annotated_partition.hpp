#ifndef OSRM_PARTITION_ANNOTATE_HPP_
#define OSRM_PARTITION_ANNOTATE_HPP_

#include "partition/bisection_graph.hpp"
#include "util/typedefs.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace osrm
{
namespace partition
{

// takes the result of a recursive bisection and turns it into an annotated partition for MLD. These
// annotated partitions provide a mapping from every node in the graph to a consecutively
// numbered cell in each level of the multi level partition. Instead of using the bisection directly
// (which can result in a unbalanced tree structure)
//
//            _____o______
//           /            \ 
//          o          ____o____
//         / \        /         \ 
//        a   b       o         _o_
//                   / \       /   \ 
//                  c   d     o     o
//                           / \   / \ 
//                          e  f   g  h
//
// we build a balanced structure that will result in a multi-cut on any level. We transform this
// layout into:
//
//            _____o__________
//           /        |       \ 
//          o         |        \ 
//         / \        |         \ 
//        a   b       o         _o_
//                   / \       /   \ 
//                  c   d     o     o
//                           / \   / \ 
//                          e  f   g  h
class AnnotatedPartition
{
  public:
    // Used to generate an implicit tree representation
    struct SizedID
    {
        BisectionID id;
        std::size_t count;

        bool operator<(const SizedID &other) const { return id < other.id; };
    };

    // Metrics that describe a single level
    struct LevelMetrics
    {
        std::size_t border_nodes;
        std::size_t border_arcs;

        // impresses imbalance, if not all nodes are in that cell anymore
        std::size_t contained_nodes;
        std::size_t number_of_cells;

        std::size_t max_border_nodes_per_cell;
        std::size_t max_border_arcs_per_cell;

        std::size_t total_memory_cells;
        std::vector<std::size_t> cell_sizes;

        std::ostream &print(std::ostream &os) const
        {
            os << "[level]\n"
               << "\t#border nodes: " << border_nodes << " #border arcs: " << border_arcs
               << " #cells: " << number_of_cells << " #contained nodes: " << contained_nodes << "\n"
               << "\tborder nodes: max: " << max_border_nodes_per_cell
               << " avg : " << static_cast<double>(border_nodes) / number_of_cells
               << " border arcs: max: " << max_border_arcs_per_cell << " "
               << " avg: " << static_cast<double>(border_arcs) / number_of_cells << "\n"
               << "\tmemory consumption: " << total_memory_cells / (1024.0 * 1024.0) << " MB."
               << "\n";
            os << "\tcell sizes:";
            for (auto s : cell_sizes)
                os << " " << s;
            os << std::endl;
            return os;
        }

        std::ostream &logMachinereadable(std::ostream &os,
                                         const std::string &identification,
                                         std::size_t depth,
                                         const bool print_header = false) const
        {
            if (print_header)
                os << "[" << identification << "] # depth cells total_nodes border_nodes "
                                               "max_border_nodes border_arcs max_border_arcs bytes "
                                               "cell_sizes*\n";

            os << "[" << identification << "] " << depth << " " << number_of_cells << " "
               << contained_nodes << " " << border_nodes << " " << max_border_nodes_per_cell << " "
               << border_arcs << " " << max_border_arcs_per_cell << " " << total_memory_cells;

            for (auto s : cell_sizes)
                os << " " << s;

            os << "\n";
            return os;
        }
    };

    AnnotatedPartition(const BisectionGraph &graph, const std::vector<BisectionID> &bisection_ids);

  private:
    // print distribution of level graph as it is
    void PrintBisection(const std::vector<SizedID> &implicit_tree,
                        const BisectionGraph &graph,
                        const std::vector<BisectionID> &bisection_ids) const;

    // find levels that offer good distribution of average cell sizes
    void SearchLevels(const std::vector<SizedID> &implicit_tree,
                      const BisectionGraph &graph,
                      const std::vector<BisectionID> &bisection_ids) const;

    // set cell_ids[i] == INFTY to exclude element
    LevelMetrics AnalyseLevel(const BisectionGraph &graph,
                              const std::vector<std::uint32_t> &cell_ids) const;

    std::vector<std::uint32_t>
    ComputeCellIDs(std::vector<std::pair<BisectionID, std::int32_t>> &prefixes,
                   const BisectionGraph &graph,
                   const std::vector<BisectionID> &bisection_ids) const;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_ANNOTATE_HPP_
