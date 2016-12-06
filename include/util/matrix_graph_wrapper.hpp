#ifndef MATRIX_GRAPH_WRAPPER_H
#define MATRIX_GRAPH_WRAPPER_H

#include <cstddef>
#include <iterator>
#include <vector>

#include "util/typedefs.hpp"

namespace osrm
{
namespace util
{

// This Wrapper provides all methods that are needed for extractor::TarjanSCC, when the graph is
// given in a matrix representation (e.g. as output from a distance table call)

template <typename T> class MatrixGraphWrapper
{
  public:
    MatrixGraphWrapper(std::vector<T> table, const std::size_t number_of_nodes)
        : table_(std::move(table)), number_of_nodes_(number_of_nodes)
    {
    }

    std::size_t GetNumberOfNodes() const { return number_of_nodes_; }

    std::vector<T> GetAdjacentEdgeRange(const NodeID node) const
    {

        std::vector<T> edges;
        // find all valid adjacent edges and move to vector `edges`
        for (std::size_t i = 0; i < number_of_nodes_; ++i)
        {
            if (*(std::begin(table_) + node * number_of_nodes_ + i) != INVALID_EDGE_WEIGHT)
            {
                edges.push_back(i);
            }
        }
        return edges;
    }

    EdgeWeight GetTarget(const EdgeWeight edge) const { return edge; }

  private:
    const std::vector<T> table_;
    const std::size_t number_of_nodes_;
};
}
}

#endif // MATRIX_GRAPH_WRAPPER_H
