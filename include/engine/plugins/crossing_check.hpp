#pragma once

#include "engine/datafacade.hpp"

#include <tuple>

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

    /// Iterate through neibour edges for edge 'e' on point 'n'.
    template <class FnT> void ForEachEdge(EdgeT const & e, NodeT const & n, FnT && fn)
    {
      // Use std::set and std::tuple key to avoid processing forward/reverse nodes with the same geometry.
      /// @todo For sure, need to check equal geometry or advice how to skip reverse nodes ...
      std::set<std::tuple<size_t, NodeID, NodeID>> processed;

      auto const makeGeometryKeyFn = [](auto const & g)
      {
        return std::make_tuple(g.size(), std::min(g.front(), g.back()),
                                         std::max(g.front(), g.back()));
      };

      processed.insert(makeGeometryKeyFn(ReadGeometry(e.m_id)));

      std::queue<uint32_t> q;
      q.push(e.m_id);

      // For the current algorithm, we need to get all connected nodes despite the forbidden turns.
      // Nothing better than iterating for all found edges in road graph around needed point.
      while (!q.empty())
      {
        auto const eID = q.front();
        q.pop();

        for (auto edge : m_facade.GetAdjacentEdgeRange(eID))
        {
          NodeID const target = m_facade.GetTarget(edge);

          auto const geometry = ReadGeometry(target);

          if ((geometry.front() == n.m_id || geometry.back() == n.m_id) &&
              processed.count(makeGeometryKeyFn(geometry)) == 0)
          {
            processed.insert(makeGeometryKeyFn(geometry));

            fn(ConstructEdge(target, geometry));

            q.push(target);
          }
        }
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

  std::vector<NodeID> GetResult();

private:
  template <class SourceT>
  std::set<EdgeT> GetCrossEdges(EdgeT const & startEdge, NodeT startNode, SourceT & dataSource);

  void Reconsider(std::set<EdgeT> const & edges);

  size_t GetInternalCount(std::set<EdgeT> const & edges);
};

}
}
}
