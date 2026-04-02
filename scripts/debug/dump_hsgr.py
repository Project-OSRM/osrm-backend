"""
Contractior debugging script.

This script displays the content of `osrm.ebg` and `osrm.hsgr` files both in tabular
form and as `.dot` graph.

"""

import argparse
import fnmatch
import os
import struct
import tabulate
import tarfile

# ".osrm.ebg"
# /common/edge_based_edge_list
# struct EdgeBasedEdge
# {
#     struct EdgeData
#     {
#         NodeID turn_id; // ID of the edge based node (node based edge)
#         EdgeWeight weight;
#         EdgeDistance distance;
#         EdgeDuration::value_type duration : 30;
#         std::uint32_t forward : 1;
#         std::uint32_t backward : 1;
#
#         auto is_unidirectional() const { return !forward || !backward; }
#     };
#     NodeID source;
#     NodeID target;
#     EdgeData data;
# };


class EbgEdge:

    fields = "source target turn_id weight distance duration".split()
    headers = "source target turn_id weight distance duration forward backward".split()

    def __init__(self, iter):
        self.__dict__.update(zip(self.fields, iter))
        self.forward = self.duration & 0x40000000 > 0
        self.backward = self.duration & 0x80000000 > 0
        self.duration = self.duration & 0x3FFFFFFF

    @classmethod
    def make(cls, buffer):
        return sorted([cls(e) for e in struct.iter_unpack("<LLLlfL", buffer)])

    def __lt__(self, other):
        return (self.source, self.target, self.weight) < (
            other.source,
            other.target,
            other.weight,
        )

    def __iter__(self):
        return (self.__getattribute__(x) for x in self.headers)


def read_ebg_file(args):
    with tarfile.open(args.input + ".ebg", "r") as tar:
        root = "/common/edge_based_edge_list"
        array = tar.getmember(root)
        buffer = tar.extractfile(array).read()
    return EbgEdge.make(buffer)


# .osrm.hsgr
# /ch/metrics/{metric}/contracted_graph/node_array
# Compressed Sparse Row
# struct NodeArrayEntry
# {
#     EdgeIterator first_edge;
# };
# /ch/metrics/{metric}/contracted_graph/edge_array
# template <typename EdgeDataT> struct EdgeArrayEntry
# {
#     NodeID target;
#     EdgeDataT data;
# };
# struct EdgeData
# {
#     // this ID is either the middle node of the shortcut, or the ID of the edge based node (node
#     // based edge) storing the appropriate data. If `shortcut` is set to true, we get the middle
#     // node. Otherwise we see the edge based node to access node data.
#     NodeID turn_id : 31;
#     bool shortcut : 1;
#     EdgeWeight weight;
#     EdgeDuration::value_type duration : 30;
#     std::uint32_t forward : 1;
#     std::uint32_t backward : 1;
#     EdgeDistance distance;
# };


class HsgrEdge:

    fields = "target turn_id weight duration distance".split()
    headers = "source target turn_id weight duration distance shortcut forward backward".split()

    def __init__(self, iter):
        self.__dict__.update(zip(self.fields, iter))
        self.forward = self.duration & 0x40000000 > 0
        self.backward = self.duration & 0x80000000 > 0
        self.duration = self.duration & 0x3FFFFFFF
        self.shortcut = self.turn_id & 0x80000000 > 0
        self.turn_id = self.turn_id & 0x7FFFFFFF

    @classmethod
    def make(cls, buffer):
        return [cls(e) for e in struct.iter_unpack("<LLllf", buffer)]

    def __iter__(self):
        return (self.__getattribute__(x) for x in self.headers)

    def __lt__(self, other):
        return (self.source, self.target, self.weight) < (
            other.source,
            other.target,
            other.weight,
        )


# .osrm.ebg_nodes
# common/ebg_node_data/nodes
# struct EdgeBasedNode
# {
#     GeometryID geometry_id;
#     ComponentID component_id;
#     std::uint32_t annotation_id : 31;
#     std::uint32_t segregated : 1;
# };
# /common/ebg_node_data/annotations
# struct NodeBasedEdgeAnnotation
# {
#     NameID name_id;                        // 32 4
#     LaneDescriptionID lane_description_id; // 16 2
#     ClassData classes;                     // 8  1
#     TravelMode travel_mode : 4;            // 4
#     bool is_left_hand_driving : 1;         // 1
# };

# .osrm.names
# /common/names/values
# /common/names/blocks


def expand_nodes(ebg_nodes, ebg_edges):
    """ebg_nodes is a run-length-encoded list of nodes.

    The start nodes in the hsgr edge list are are sorted, ascending, with duplicates.
    The first entry in ebg_nodes contains the position where the first node starts in
    ebg_edges, the second entry the position where the second node starts, etc."""
    node = -1
    (next_node,) = next(ebg_nodes)
    for pos, e in enumerate(ebg_edges):
        while pos == next_node:
            node += 1
            (next_node,) = next(ebg_nodes)
        e.source = node
        yield e


def read_hsgr_file(args):
    with tarfile.open(args.input + ".hsgr", "r") as tar:
        found = 0
        root = f"/ch/metrics/{args.metric}/contracted_graph/"
        for member in tar:
            if fnmatch.fnmatch(member.name, root + "node_array"):
                node_buffer = tar.extractfile(member).read()
                found += 1
            if fnmatch.fnmatch(member.name, root + "edge_array"):
                edge_buffer = tar.extractfile(member).read()
                found += 1
    if found != 2:
        raise Exception(f"member {root} not found")
    nodes = struct.iter_unpack("<L", node_buffer)
    nodes2 = struct.iter_unpack("<L", node_buffer)
    return nodes, expand_nodes(nodes2, HsgrEdge.make(edge_buffer))


def join(params):
    return ",".join([f"{k}={v}" for k, v in params.items()])


def write_dot_file(ebg_edges, hsgr_edges):
    dot = args.dot
    dot.write("digraph {\nrankdir=BT;\n")  # not strict!
    labels = {}
    hsgr_edges = list(hsgr_edges)

    def edge_params(e):
        params = {
            "label": e.weight,
            "dir": "both",  # not really, but enables arrowtail
            # "constraint": "false",
        }
        if e.forward and e.backward:
            params["arrowhead"] = "normal"
            params["arrowtail"] = "dotnormal"
        elif e.forward:
            params["arrowhead"] = "normal"
            params["arrowtail"] = "dot"
        elif e.backward:
            params["arrowhead"] = "none"
            params["arrowtail"] = "dotnormal"
        return params

    def draw_edge(e, params, sub):
        dot.write(f"  {sub}{e.source}->{sub}{e.target}[{join(params)}];\n")
        labels[f"{sub}{e.source}"] = e.source
        labels[f"{sub}{e.target}"] = e.target

    dot.write("subgraph cluster_ebg {\n  label=ebg;\n")
    for e in ebg_edges:
        params = edge_params(e)
        draw_edge(e, params, "ebg")
    dot.write("}\n")

    dot.write("subgraph cluster_hsgr {\n  label=hsgr;\n")
    for e in sorted(hsgr_edges):
        params = edge_params(e)
        if e.shortcut:
            params["color"] = "red"
            params["label"] = f'"{e.turn_id}|{e.weight}"'
        draw_edge(e, params, "hsgr")
    dot.write("}\n")

    for id_, label in labels.items():
        params = {"label": label}
        dot.write(f"{id_}[{join(params)}];\n")
    dot.write(
        f"""
    labelloc="t";
    label="{os.path.basename(args.input)}";
    }}\n"""
    )


def print_table(rows, headers):
    print(tabulate.tabulate(rows, headers, tablefmt="github", floatfmt=".2f"))
    return


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument(
        "input",
        help="The input file",
        metavar="FILE",
    )

    parser.add_argument("--metric", help="The graph metric (default: *)", default="*")

    parser.add_argument(
        "--dot", help="Output a .dot file", type=argparse.FileType("wt")
    )

    return parser


if __name__ == "__main__":
    args = build_parser().parse_args()
    if args.input.endswith(".hsgr"):
        args.input = args.input[:-5]
    ebg_edges = read_ebg_file(args)
    hsgr_nodes, hsgr_edges = read_hsgr_file(args)

    if args.dot:
        write_dot_file(ebg_edges, hsgr_edges)
    else:
        print("ebg")
        print_table(ebg_edges, EbgEdge.headers)

        print("\nhsgr nodes")
        print_table(hsgr_nodes, "node_offset")

        print("\nhsgr")
        print_table(hsgr_edges, HsgrEdge.headers)
