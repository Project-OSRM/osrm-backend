#ifndef OSRM_CUSTOMIZATION_GRASP_CUSTOMIZATION_GRAPH_HPP
#define OSRM_CUSTOMIZATION_GRASP_CUSTOMIZATION_GRAPH_HPP

#include "util/static_graph.hpp"

namespace osrm
{
namespace customizer
{

struct GRASPData
{
    EdgeWeight weight;
};

using GRASPCustomizationGraph = util::StaticGraph<GRASPData>;
}
}

#endif
