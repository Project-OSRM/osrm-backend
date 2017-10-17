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

    auto const getNamesFn = [&](auto const & r)
    {
      std::vector<std::string> names;
      for (auto i = r.first; i != r.second; ++i)
        if (i->second.m_id != e.m_id && !i->second.m_name.empty())
          names.push_back(i->second.m_name);

      std::sort(names.begin(), names.end());
      return names;
    };

    auto const names1 = getNamesFn(m.equal_range(e.m_n1));
    auto const names2 = getNamesFn(m.equal_range(e.m_n2));

    // Internal edge with the name should be connected with any other neibour edge with the same name.
    if (!e.m_name.empty())
    {
      if (!std::binary_search(names1.begin(), names1.end(), e.m_name) &&
          !std::binary_search(names2.begin(), names2.end(), e.m_name))
      {
        e.m_internal = false;
        continue;
      }
    }

    // Custom set_intersection function for using with equal strings.
    auto const setIntersectionFn = [](auto const & v1, auto const & v2, auto it)
    {
      auto i1 = v1.begin();
      auto i2 = v2.begin();

      while (i1 != v1.end() && i2 != v2.end())
      {
        if (*i1 == *i2)
        {
          *it++ = *i1;
          ++i1;
          ++i2;
        }
        else if (*i1 < *i2)
          ++i1;
        else
          ++i2;
      }

      return it;
    };

    std::vector<std::string> intersection;
    setIntersectionFn(names1, names2, std::back_inserter(intersection));

    // Internal edge connects minimum 2 pairs of equal-name edges.
    if (intersection.size() >= 2)
      continue;

    /// @todo
//    // Internal edge is U-turn edge.
//    if (intersection.size() == 1 && intersection.front() != e.m_name)
//      continue;

    e.m_internal = false;
  }
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

std::vector<NodeID> CrossingCheck::GetResult()
{
  std::sort(m_results.begin(), m_results.end());
  m_results.erase(std::unique(m_results.begin(), m_results.end()), m_results.end());

  std::cout << "Results count = " << m_results.size() << std::endl;

  return m_results;
}

}
}
}
