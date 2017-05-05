#include "util/static_rtree.hpp"
#include "util/web_mercator.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[])
{
    using namespace osrm;
    using EdgeDataT = int;
    using RTree = util::StaticRTree<EdgeDataT>;

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " filename.osrm" << std::endl;
        return EXIT_SUCCESS;
    }

    std::string node_file = argv[1];
    // using dummy <int> here, we don't need any templated types
    std::vector<RTree::TreeNode> m_search_tree;

    storage::io::FileReader tree_node_file(node_file + ".ramIndex",
                                           storage::io::FileReader::VerifyFingerprint);

    const auto tree_size = tree_node_file.ReadElementCount64();

    m_search_tree.resize(tree_size);
    tree_node_file.ReadInto(&m_search_tree[0], tree_size);

    struct nodeinfo
    {
        decltype(RTree::TreeNode::first_child_index) index;
        int depth;
    };
    // queue for breadth-first traversal
    std::queue<nodeinfo> queue;

    queue.push({0, 0});
    auto first = true;

    std::cout << "{\"type\":\"FeatureCollection\",\"features\":[";

    std::string strokes[] = {"red", "blue", "green", "orange", "yellow"};

    while (!queue.empty())
    {
        auto info = queue.front();
        queue.pop();

        auto node = m_search_tree[info.index];
        auto depth = info.depth;

        auto min_coord = util::web_mercator::toWGS84(
            {util::toFloating(node.minimum_bounding_rectangle.min_lon),
             util::toFloating(node.minimum_bounding_rectangle.min_lat)});
        auto max_coord = util::web_mercator::toWGS84(
            {util::toFloating(node.minimum_bounding_rectangle.max_lon),
             util::toFloating(node.minimum_bounding_rectangle.max_lat)});

        // Write out geojson showing the boundaries
        if (first)
        {
            first = false;
        }
        else
        {
            std::cout << "," << std::endl;
        }

        std::cout << "{\"type\":\"Feature\",\"properties\":{\"depth\":" << depth << ","
                  << "\"stroke\": \"" << strokes[depth] << "\","
                  << "\"fill-opacity\":0,"
                  << "\"stroke-opacity\":1,"
                  << "\"stroke-width\":2"
                  << "},\"geometry\":{";
        std::cout << " \"type\":\"Polygon\",\"coordinates\":[[";
        std::cout << "[" << min_coord.lon << "," << min_coord.lat << "],";
        std::cout << "[" << min_coord.lon << "," << max_coord.lat << "],";
        std::cout << "[" << max_coord.lon << "," << max_coord.lat << "],";
        std::cout << "[" << max_coord.lon << "," << min_coord.lat << "],";
        std::cout << "[" << min_coord.lon << "," << min_coord.lat << "]";
        std::cout << "]]}}";
        if (!node.is_leaf)
        {
            for (auto i = node.first_child_index; i < node.first_child_index + node.child_count;
                 i++)
            {
                queue.push({i, depth + 1});
            }
        }
    }
    std::cout << "]}" << std::endl;
}