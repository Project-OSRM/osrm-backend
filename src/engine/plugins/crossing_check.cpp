#include "engine/plugins/crossing_check.hpp"

namespace osrm
{
namespace engine
{
namespace plugins
{

/// @return Cross edges (internal and external) that belongs to one logical road cross.
template <class SourceT>
std::set<CrossingCheck::EdgeT>
CrossingCheck::GetCrossEdges(EdgeT const & startEdge, NodeT startNode, SourceT & dataSource)
{
  std::queue<std::pair<NodeT, EdgeT>> q;
  q.push(std::make_pair(startNode, startEdge));

  std::set<EdgeT> crossEdges;
  crossEdges.insert(startEdge);

  while (!q.empty())
  {
    auto const v = q.front();
    q.pop();

    dataSource.ForEachEdge(v.second, v.first, [&](EdgeT const & e)
    {
      auto itEdge = crossEdges.insert(e);
      if (!itEdge.second)
        return;

      if (e.GetLength() > m_maxInnerLength)
        return;

      /// @todo Can add some additional criteria on internal edge according to
      /// 'v.second' as input edge and 'e' as continuous edge.

      // Set edge as internal candidate.
      itEdge.first->m_internal = true;

      q.push(std::make_pair(e.GetTarget(v.first), e));
    });
  }

  return crossEdges;
}

/// Check internal candidates for valid internal flag.
void CrossingCheck::Reconsider(std::set<EdgeT> const & edges)
{
  std::multimap<NodeT, EdgeT> m;

  for (EdgeT const & e : edges)
  {
    m.insert(std::make_pair(e.m_n1, e));
    m.insert(std::make_pair(e.m_n2, e));
  }

  for (EdgeT const & e : edges)
  {
    if (!e.m_internal)
      continue;

    auto const r1 = m.equal_range(e.m_n1);
    auto const r2 = m.equal_range(e.m_n2);

    // Internal edge should connect crossing, not a single way.
    auto const sizeFn = [](auto const & r)
    {
      return std::distance(r.first, r.second);
    };

    if (sizeFn(r1) < 2 && sizeFn(r2) < 2)
    {
      e.m_internal = false;
      continue;
    }

    if (!e.m_name.empty())
    {
      auto const countEqFn = [&](auto const & r)
      {
        size_t count = 0;
        for (auto i = r.first; i != r.second; ++i)
          if (i->second.m_id != e.m_id && i->second.m_name == e.m_name)
            ++count;
        return count;
      };

      auto const countNEqFn = [&](auto const & r)
      {
        size_t count = 0;
        for (auto i = r.first; i != r.second; ++i)
          if (!i->second.m_name.empty() && i->second.m_name != e.m_name)
            ++count;
        return count;
      };

      size_t const countEq = countEqFn(r1) + countEqFn(r2);

      // Internal edge with the name should be connected with any other neibour edge with the same name.
      if (countEq == 0)
      {
        e.m_internal = false;
        continue;
      }

      size_t const countNE1 = countNEqFn(r1);
      size_t const countNE2 = countNEqFn(r2);

      // This hard check allows to avoid setting internal edge with connected "service" unnamed road.
      if (countNE1 == 0 || countNE2 == 0)
      {
        e.m_internal = false;
        continue;
      }

      // Internal edge with the name should have >= 2 other turn options or U-turn.
      if ((countNE1 + countNE2 < 2) && countEq < 3)
      {
        e.m_internal = false;
        continue;
      }
    }
    else
    {
      /// @todo Have some doubts here ...

      // Internal edge without name should connect named edges.
      auto const hasNameFn = [](auto const & r)
      {
        for (auto i = r.first; i != r.second; ++i)
          if (!i->second.m_name.empty())
            return true;
        return false;
      };

      if (!hasNameFn(r1) || !hasNameFn(r2))
      {
        e.m_internal = false;
        continue;
      }
    }
  }

  /// @todo Process other internal edge false positive cases.
}

size_t CrossingCheck::GetInternalCount(std::set<EdgeT> const & edges)
{
  return std::count_if(edges.begin(), edges.end(), [](EdgeT const & e)
  {
    return e.m_internal;
  });
}

void CrossingCheck::ProcessNode(NodeID nodeID)
{
  DataFacadeSource source(m_facade);

  EdgeT const edge = source.ConstructEdge(nodeID, source.ReadGeometry(nodeID));
  NodeT nodes[] = { edge.m_n1, edge.m_n2 };

  for (auto const & n : nodes)
  {
    auto const candidates = GetCrossEdges(edge, n, source);
    Reconsider(candidates);

    for (auto const & e : candidates)
      if (e.m_internal)
        m_results.push_back(e.m_id);
  }
}

}
}
}
