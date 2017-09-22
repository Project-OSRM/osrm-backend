#pragma once

#include "engine/datafacade.hpp"

namespace osrm
{
namespace engine
{
namespace plugins
{

class CrossingCheck
{
public:
  using DataFacadeT = DataFacade<routing_algorithms::mld::Algorithm>;

private:
  //using PointT = osrm::util::Coordinate;

  struct NodeT
  {
    //PointT m_pt;
    uint32_t m_id;

//    Node(double lat, double lon)
//      : m_pt(osrm::util::FloatLongitude{lon}, osrm::util::FloatLatitude{lat}), m_id(++g_constructID)
//    {
//    }

    bool IsEqual(NodeT const & rhs) const { return m_id == rhs.m_id; }

    bool operator< (NodeT const & rhs) const
    {
      return (m_id < rhs.m_id);
    }
  };

  struct EdgeT
  {
    uint32_t m_id;

    NodeT m_n1, m_n2;
    std::string m_name;

    double m_length;

    mutable bool m_internal;

    EdgeT() : m_internal(false) {}

//    EdgeT(NodeT n1, NodeT n2, std::string const & name)
//      : m_n1(n1), m_n2(n2), m_name(name), m_internal(false)
//    {
//    }

    bool operator< (EdgeT const & rhs) const
    {
      return (m_n1.IsEqual(rhs.m_n1) ? m_n2 < rhs.m_n2 : m_n1 < rhs.m_n1);
    }

    NodeT const & GetTarget(NodeT const & source) const
    {
      if (source.IsEqual(m_n1))
        return m_n2;
      else
      {
        BOOST_ASSERT(source.IsEqual(m_n2));
        return m_n1;
      }
    }

    double GetLength() const
    {
      return m_length;
    }
  };

  class DataFacadeSource
  {
    DataFacadeT const & m_facade;

  public:
    explicit DataFacadeSource(DataFacadeT const & facade) : m_facade(facade) {}

    std::vector<NodeID> ReadGeometry(NodeID node)
    {
      std::vector<NodeID> geometry;
      auto const geomIndex = m_facade.GetGeometryIndex(node);
      if (geomIndex.forward)
          geometry = m_facade.GetUncompressedForwardGeometry(geomIndex.id);
      else
          geometry = m_facade.GetUncompressedReverseGeometry(geomIndex.id);

      BOOST_ASSERT(geometry.size() > 1);
      return geometry;
    }

    EdgeT ConstructEdge(NodeID node, std::vector<NodeID> const & geometry)
    {
      EdgeT edge;
      edge.m_id = node;
      edge.m_n1 = { geometry.front() };
      edge.m_n2 = { geometry.back() };
      auto const name = m_facade.GetNameForID(m_facade.GetNameIndex(node));
      edge.m_name.assign(name.begin(), name.end());

      edge.m_length = 0.0;
      for (size_t i = 1; i < geometry.size(); ++i)
      {
        edge.m_length += osrm::util::coordinate_calculation::haversineDistance(
                      m_facade.GetCoordinateOfNode(geometry[i - 1]),
                      m_facade.GetCoordinateOfNode(geometry[i]));
      }

      return edge;
    }

    template <class FnT> void ForEachEdge(EdgeT const & e, NodeT const & n, FnT && fn)
    {
      auto const sourceGeometry = ReadGeometry(e.m_id);

      for (auto edge : m_facade.GetAdjacentEdgeRange(e.m_id))
      {
//        auto const & data = m_facade.GetEdgeData(edge);
//        if (!data.forward)
//          continue;

        NodeID const target = m_facade.GetTarget(edge);
        auto const targetGeometry = ReadGeometry(target);

        if (sourceGeometry.size() == targetGeometry.size() &&
            (std::equal(sourceGeometry.begin(), sourceGeometry.end(), targetGeometry.begin()) ||
             std::equal(sourceGeometry.begin(), sourceGeometry.end(), targetGeometry.rbegin())))
        {
          // skip UTurn on same node geometry
          continue;
        }

        if (targetGeometry.front() == n.m_id || targetGeometry.back() == n.m_id)
          fn(ConstructEdge(target, targetGeometry));
      }
    }
  };

  DataFacadeT const & m_facade;
  double m_maxInnerLength;

  std::vector<NodeID> m_results;

public:
  CrossingCheck(DataFacadeT const & facade, double maxInnerLength)
    : m_facade(facade), m_maxInnerLength(maxInnerLength)
  {
  }

  void ProcessNode(NodeID nodeID);

  std::vector<NodeID> GetResult()
  {
    std::sort(m_results.begin(), m_results.end());
    m_results.erase(std::unique(m_results.begin(), m_results.end()), m_results.end());
    return m_results;
  }

private:
  template <class SourceT>
  std::set<EdgeT> GetCrossEdges(EdgeT const & startEdge, NodeT startNode, SourceT & dataSource);

  void Reconsider(std::set<EdgeT> const & edges);

  size_t GetInternalCount(std::set<EdgeT> const & edges);
};

}
}
}
