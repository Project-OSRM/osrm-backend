# MLD partitioning: prune one-shell nodes before bisection

## Goal

Change MLD partitioning so that the node-based graph is pruned before recursive bisection:

1. Build a temporary undirected view of the compressed node-based graph (CNBG).
2. Iteratively peel degree-1 nodes.
3. Track each peeled node’s attachment root with union-find.
4. Run recursive bisection only on the surviving graph.
5. Copy the resulting `BisectionID`s back to all original nodes, assigning each peeled node the same ID as its attachment root.

This reduces the graph fed to bisection and keeps dangling shell nodes from influencing cuts.

## Agreed design decisions

- Use **iterative leaf peeling**.
- Use **undirected degree** for pruning: degree-1 means one unique neighbor in the temporary view.
- Build the pruning view as a **simple undirected graph**:
  - symmetrize the CNBG edges,
  - deduplicate neighbor pairs,
  - ignore self-loops.
- Use `DynamicGraph` internally for the pruning pass.
- Use a **directed** union-find link operation: `link(pendant, neighbor)`.
- Preserve the **original CNBG edge directionality** in the returned pruned edge list.
- Compact surviving nodes in **ascending original node id order**.
- Keep original degree-0 nodes as singleton survivors.
- If a node becomes isolated only because of peeling, keep it as a singleton survivor.
- Add **utility tests only** for the pruning helper and union-find.

## Files to touch

### New

- `include/partitioner/graph_utils.hpp`
- `unit_tests/partitioner/graph_utils.cpp`

### Existing

- `src/partitioner/partitioner.cpp`
- possibly small test helper updates if needed

## Proposed helper API

### `UnionFind`

Path-halving DSU with directed parent links:

```cpp
struct UnionFind
{
    explicit UnionFind(std::size_t n);

    NodeID find(NodeID x);
    void link(NodeID child, NodeID parent);
};
```

`link(child, parent)` must always make the parent the representative. Do not use rank-based union.

### `PrunedGraph`

```cpp
struct PrunedGraph
{
    std::vector<extractor::CompressedNodeBasedGraphEdge> edges;
    std::vector<NodeID> core_to_original;
    UnionFind pendant_uf;
};
```

`core_to_original[i]` maps the pruned node id back to the original node id.

## `pruneOneShell()` behavior

Input:

- sorted CNBG edge list
- number of original nodes

Output:

- pruned edge list remapped to 0-based pruned ids
- surviving node map
- union-find with attachment roots

Algorithm:

1. Build a temporary symmetrized, deduplicated `DynamicGraph<NoData>`.
2. Initialize a queue with all nodes of degree 1.
3. While the queue is non-empty:
   - pop a node `v`
   - skip if its current degree is no longer 1
   - find its sole neighbor `u`
   - `link(v, u)` in union-find
   - remove `v`’s incident edges from the `DynamicGraph`
   - if `u` becomes degree 1, push `u`
4. Collect all surviving nodes in ascending original id order.
5. Build a local `original_to_pruned[]` map.
6. Remap all surviving original CNBG edges to pruned ids.
7. Return `PrunedGraph`.

Implementation notes:

- Keep the temporary graph internal to the helper.
- Use `DynamicGraph::DeleteEdgesTo()` for edge removal during peeling.
- Use `DynamicGraph::InsertEdge()` when building the temporary undirected graph.
- Filter out self-loops before symmetrizing.

## Partitioning integration

In `getGraphBisection()`:

1. Read and group CNBG edges as today.
2. Call `pruneOneShell(edges, num_nodes)`.
3. Build `core_coords` from `pruned.core_to_original`.
4. Run `makeBisectionGraph(core_coords, adaptToBisectionEdge(std::move(pruned.edges)))`.
5. Run `RecursiveBisection` on the pruned graph.
6. Expand the pruned `BisectionID`s back to the original node space:
   - first copy ids for surviving nodes
   - then fill peeled nodes with the id of `pruned.pendant_uf.find(node)`

If no surviving nodes remain, handle it explicitly before constructing `RecursiveBisection`.

## Test plan

Create `unit_tests/partitioner/graph_utils.cpp` with tests for:

1. empty graph
2. single isolated node
3. pure path
4. star graph
5. triangle + pendant
6. chain attached to triangle
7. union-find root propagation
8. final `BisectionID` copy logic

Focus on the helper behavior, not the full partitioning pipeline.

## Acceptance criteria

- One-shell nodes are removed before recursive bisection.
- Attachment roots are tracked correctly through chains of pendants.
- Surviving nodes are compacted and remapped correctly.
- Peeled nodes receive the same final `BisectionID` as their attachment root.
- Existing CNBG directionality is preserved in the pruned output.
- New tests pass.

## Notes for the next engineer

- The pruning helper is intentionally narrow and partitioner-specific.
- The temporary graph is only for identifying and removing degree-1 shells.
- Do not change the public partitioning format unless the integration step requires it.
- Keep the implementation consistent with OSRM’s style: no broad exceptions, minimal comments, explicit `std::` prefixes, and header self-containment.
