#ifndef OSRM_EXTRACTOR_AREA_DIJKSTRA_HPP
#define OSRM_EXTRACTOR_AREA_DIJKSTRA_HPP

#include "index_priority_queue.hpp"

#include <cmath>
#include <limits>
#include <map>
#include <vector>

namespace osrm::extractor::area
{

/**
 * @brief Implements the Dijkstra shortest-path algorithm.
 *
 * @tparam vertex_t The type of a vertex.
 */
template <class vertex_t> class Dijkstra
{
    struct Edge
    {
        Edge(size_t other, double weight)
            : other{other}, weight{weight} {}; // for macos-x64 clang-14
        size_t other;
        double weight;
    };
    std::vector<vertex_t> vertices;
    std::map<vertex_t, size_t> seen_vertices;

    std::vector<double> distances;
    std::vector<size_t> predecessors;
    std::vector<std::vector<Edge>> adj;

    /**
     * @brief Initialize the data structures before each run.
     */
    void init_data()
    {
        double inf = std::numeric_limits<double>::infinity();
        distances.resize(vertices.size());
        predecessors.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            distances[i] = inf;
            predecessors[i] = i;
        }
    }

  public:
    /**
     * @brief Add one vertex to the graph.
     *
     * If a vertex is already present it will not be inserted again and the index of the
     * present vertex will be returned.
     *
     * @param v The vertex to add
     * @return  The index of the vertex.
     */
    size_t add_vertex(const vertex_t &v)
    {
        size_t pos = vertices.size();
        auto [_, inserted] = seen_vertices.emplace(v, pos);
        if (inserted)
        {
            vertices.push_back(v);
            return pos;
        }
        return seen_vertices.at(v);
    };

    /**
     * @brief Return the index of the vertex.
     *
     * @param v The vertex
     * @return size_t The index of the vertex.
     */
    size_t index_of(const vertex_t &v) { return seen_vertices.at(v); }

    /**
     * @brief Get the vertex object
     *
     * @param i The index of the vertex
     * @return const vertex_t& The vertex
     */
    const vertex_t &get_vertex(size_t i) { return vertices.at(i); };

    /**
     * @brief Add one edge to the graph.
     *
     * It is the caller's responsibility to avoid inserting duplicate edges.
     *
     * @param u The first vertex.
     * @param v The second vertex.
     * @param weight The weight of the edge.
     */
    void add_edge(const vertex_t &u, const vertex_t &v, double weight)
    {
        size_t iu = add_vertex(u);
        size_t iv = add_vertex(v);
        adj.resize(vertices.size());
        adj[iu].emplace_back(iv, weight);
        adj[iv].emplace_back(iu, weight);
    }

    const std::vector<size_t> &get_predecessors() { return predecessors; }
    const std::vector<double> &get_distances() { return distances; }

    /**
     * @brief Run the Dijkstra shortest-path algorithm starting at vertex s.
     *
     * After this function completes the predecessor for each vertex will be stored in
     * {@code predecessors} and the distance from {@code s} to each vertex will be
     * stored in {@code distances}.
     *
     * @param s The index of the "start" vertex.
     */
    void run(size_t s)
    {
        init_data();

        IndexPriorityQueue pq(vertices.size(),
                              [this](size_t u, size_t v) -> bool
                              { return distances[u] < distances[v]; });

        distances[s] = 0;
        pq.insert(s);

        while (!pq.empty())
        {
            size_t u = pq.pop();
            double dist_u = distances[u];
            for (Edge e : adj[u])
            {
                size_t v = e.other;
                if (dist_u + e.weight < distances[v])
                {
                    distances[v] = dist_u + e.weight;
                    predecessors[v] = u;
                    pq.insert_or_decrease(v);
                }
            }
        }
    }

    /**
     * @brief Return the number of vertices.
     */
    size_t num_vertices() { return vertices.size(); }

    /**
     * @brief Return the number of edges.
     */
    size_t num_edges()
    {
        size_t n = 0;
        for (auto &a : adj)
        {
            n += a.size();
        }
        return n / 2;
    }
};

} // namespace osrm::extractor::area

#endif
