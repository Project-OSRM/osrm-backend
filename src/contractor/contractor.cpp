#include "contractor/contractor.hpp"
#include "contractor/crc32_processor.hpp"
#include "contractor/graph_contractor.hpp"
#include "contractor/graph_contractor_adaptors.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/node_based_edge.hpp"

#include "storage/io.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/graph_loader.hpp"
#include "util/integer_range.hpp"
#include "util/io.hpp"
#include "util/log.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/functional/hash.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

#include <tbb/blocked_range.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_sort.h>
#include <tbb/spin_mutex.h>

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace
{
struct Segment final
{
    std::uint64_t from, to;
    Segment() : from(0), to(0) {}
    Segment(const std::uint64_t from, const std::uint64_t to) : from(from), to(to) {}
    Segment(const OSMNodeID from, const OSMNodeID to)
        : from(static_cast<std::uint64_t>(from)), to(static_cast<std::uint64_t>(to))
    {
    }

    bool operator<(const Segment &rhs) const
    {
        return std::tie(from, to) < std::tie(rhs.from, rhs.to);
    }
    bool operator==(const Segment &rhs) const
    {
        return std::tie(from, to) == std::tie(rhs.from, rhs.to);
    }
};

struct SpeedSource final
{
    SpeedSource() : speed(0), weight(std::numeric_limits<double>::quiet_NaN()) {}
    unsigned speed;
    double weight;
    std::uint8_t source;
};

struct Turn final
{
    std::uint64_t from, via, to;
    Turn() : from(0), via(0), to(0) {}
    Turn(const std::uint64_t from, const std::uint64_t via, const std::uint64_t to)
        : from(from), via(via), to(to)
    {
    }
    Turn(const osrm::extractor::lookup::TurnIndexBlock &turn)
        : from(static_cast<std::uint64_t>(turn.from_id)),
          via(static_cast<std::uint64_t>(turn.via_id)), to(static_cast<std::uint64_t>(turn.to_id))
    {
    }
    bool operator<(const Turn &rhs) const
    {
        return std::tie(from, via, to) < std::tie(rhs.from, rhs.via, rhs.to);
    }
    bool operator==(const Turn &rhs) const
    {
        return std::tie(from, via, to) == std::tie(rhs.from, rhs.via, rhs.to);
    }
};

struct PenaltySource final
{
    PenaltySource() : duration(0.), weight(std::numeric_limits<double>::quiet_NaN()) {}
    double duration;
    double weight;
    std::uint8_t source;
};

template <typename T> inline bool is_aligned(const void *pointer)
{
    static_assert(sizeof(T) % alignof(T) == 0, "pointer can not be used as an array pointer");
    return reinterpret_cast<uintptr_t>(pointer) % alignof(T) == 0;
}

void CheckWeightsConsistency(
    const osrm::contractor::ContractorConfig &config,
    const std::vector<osrm::extractor::EdgeBasedEdge> &edge_based_edge_list)
{
    using Reader = osrm::storage::io::FileReader;
    using OriginalEdgeData = osrm::extractor::OriginalEdgeData;

    Reader geometry_file(config.geometry_path, Reader::HasNoFingerprint);
    const auto number_of_indices = geometry_file.ReadElementCount32();
    std::vector<unsigned> geometry_indices(number_of_indices);
    geometry_file.ReadInto(geometry_indices);

    const auto number_of_compressed_geometries = geometry_file.ReadElementCount32();
    BOOST_ASSERT(geometry_indices.back() == number_of_compressed_geometries);
    if (number_of_compressed_geometries == 0)
        return;

    std::vector<NodeID> geometry_node_list(number_of_compressed_geometries);
    std::vector<EdgeWeight> forward_weight_list(number_of_compressed_geometries);
    std::vector<EdgeWeight> reverse_weight_list(number_of_compressed_geometries);
    geometry_file.ReadInto(geometry_node_list.data(), number_of_compressed_geometries);
    geometry_file.ReadInto(forward_weight_list.data(), number_of_compressed_geometries);
    geometry_file.ReadInto(reverse_weight_list.data(), number_of_compressed_geometries);

    Reader edges_input_file(config.osrm_input_path.string() + ".edges", Reader::HasNoFingerprint);
    std::vector<OriginalEdgeData> current_edge_data(edges_input_file.ReadElementCount64());
    edges_input_file.ReadInto(current_edge_data);

    for (auto &edge : edge_based_edge_list)
    {
        BOOST_ASSERT(edge.edge_id < current_edge_data.size());
        auto geometry_id = current_edge_data[edge.edge_id].via_geometry;
        BOOST_ASSERT(geometry_id.id < geometry_indices.size());

        const auto &weights = geometry_id.forward ? forward_weight_list : reverse_weight_list;
        const int shift = static_cast<int>(geometry_id.forward);
        const auto first = weights.begin() + geometry_indices.at(geometry_id.id) + shift;
        const auto last = weights.begin() + geometry_indices.at(geometry_id.id + 1) - 1 + shift;
        EdgeWeight weight = std::accumulate(first, last, 0);

        BOOST_ASSERT(weight <= edge.weight);
    }
}

} // anon ns

BOOST_FUSION_ADAPT_STRUCT(Segment, (decltype(Segment::from), from)(decltype(Segment::to), to))
BOOST_FUSION_ADAPT_STRUCT(SpeedSource,
                          (decltype(SpeedSource::speed), speed)(decltype(SpeedSource::weight),
                                                                weight))
BOOST_FUSION_ADAPT_STRUCT(Turn,
                          (decltype(Turn::from), from)(decltype(Turn::via), via)(decltype(Turn::to),
                                                                                 to))
BOOST_FUSION_ADAPT_STRUCT(PenaltySource,
                          (decltype(PenaltySource::duration),
                           duration)(decltype(PenaltySource::weight), weight))

namespace osrm
{
namespace contractor
{
namespace
{
namespace qi = boost::spirit::qi;
}

// Functor to parse a list of CSV files using "key,value,comment" grammar.
// Key and Value structures must be a model of Random Access Sequence.
// Also the Value structure must have source member that will be filled
// with the corresponding file index in the CSV filenames vector.
template <typename Key, typename Value> struct CSVFilesParser
{
    using Iterator = boost::spirit::line_pos_iterator<boost::spirit::istream_iterator>;
    using KeyRule = qi::rule<Iterator, Key()>;
    using ValueRule = qi::rule<Iterator, Value()>;

    CSVFilesParser(std::size_t start_index, const KeyRule &key_rule, const ValueRule &value_rule)
        : start_index(start_index), key_rule(key_rule), value_rule(value_rule)
    {
    }

    // Operator returns a lambda function that maps input Key to boost::optional<Value>.
    auto operator()(const std::vector<std::string> &csv_filenames) const
    {
        try
        {
            tbb::spin_mutex mutex;
            std::vector<std::pair<Key, Value>> lookup;
            tbb::parallel_for(std::size_t{0},
                              csv_filenames.size(),
                              [&](const std::size_t idx) {
                                  auto local = ParseCSVFile(csv_filenames[idx], start_index + idx);

                                  { // Merge local CSV results into a flat global vector
                                      tbb::spin_mutex::scoped_lock _{mutex};
                                      lookup.insert(end(lookup),
                                                    std::make_move_iterator(begin(local)),
                                                    std::make_move_iterator(end(local)));
                                  }
                              });

            // With flattened map-ish view of all the files, make a stable sort on key and source
            // and unique them on key to keep only the value with the largest file index
            // and the largest line number in a file.
            // The operands order is swapped to make descending ordering on (key, source)
            std::stable_sort(begin(lookup), end(lookup), [](const auto &lhs, const auto &rhs) {
                return rhs.first < lhs.first ||
                       (rhs.first == lhs.first && rhs.second.source < lhs.second.source);
            });

            // Unique only on key to take the source precedence into account and remove duplicates.
            const auto it =
                std::unique(begin(lookup), end(lookup), [](const auto &lhs, const auto &rhs) {
                    return lhs.first == rhs.first;
                });
            lookup.erase(it, end(lookup));

            osrm::util::Log() << "In total loaded " << csv_filenames.size()
                              << " file(s) with a total of " << lookup.size() << " unique values";

            return [lookup](const Key &key) {
                using Result = boost::optional<Value>;
                const auto it = std::lower_bound(
                    lookup.begin(), lookup.end(), key, [](const auto &lhs, const auto &rhs) {
                        return rhs < lhs.first;
                    });
                return it != std::end(lookup) && !(it->first < key) ? Result(it->second) : Result();
            };
        }
        catch (const tbb::captured_exception &e)
        {
            throw osrm::util::exception(e.what() + SOURCE_REF);
        }
    }

  private:
    // Parse a single CSV file and return result as a vector<Key, Value>
    auto ParseCSVFile(const std::string &filename, std::size_t file_id) const
    {
        std::ifstream input_stream(filename, std::ios::binary);
        input_stream.unsetf(std::ios::skipws);

        boost::spirit::istream_iterator sfirst(input_stream), slast;
        Iterator first(sfirst), last(slast);

        BOOST_ASSERT(file_id <= std::numeric_limits<std::uint8_t>::max());
        ValueRule value_source =
            value_rule[qi::_val = qi::_1, boost::phoenix::bind(&Value::source, qi::_val) = file_id];
        qi::rule<Iterator, std::pair<Key, Value>()> csv_line =
            (key_rule >> ',' >> value_source) >> -(',' >> *(qi::char_ - qi::eol));
        std::vector<std::pair<Key, Value>> result;
        const auto ok = qi::parse(first, last, -(csv_line % qi::eol) >> *qi::eol, result);

        if (!ok || first != last)
        {
            const auto message =
                boost::format("CSV file %1% malformed on line %2%") % filename % first.position();
            throw osrm::util::exception(message.str() + SOURCE_REF);
        }

        osrm::util::Log() << "Loaded " << filename << " with " << result.size() << "values";

        return std::move(result);
    }

    const std::size_t start_index;
    const KeyRule key_rule;
    const ValueRule value_rule;
};

// Returns duration in deci-seconds
inline EdgeWeight ConvertToDuration(double distance_in_meters, double speed_in_kmh)
{
    if (speed_in_kmh <= 0.)
        return MAXIMAL_EDGE_DURATION;

    const double speed_in_ms = speed_in_kmh / 3.6;
    const double duration = distance_in_meters / speed_in_ms;
    return std::max<EdgeWeight>(1, static_cast<EdgeWeight>(std::round(duration * 10.)));
}

inline EdgeWeight ConvertToWeight(double weight, double weight_multiplier, EdgeWeight duration)
{
    if (std::isfinite(weight))
        return std::round(weight * weight_multiplier);

    return duration == MAXIMAL_EDGE_DURATION ? INVALID_EDGE_WEIGHT
                                             : duration * weight_multiplier / 10.;
}

// Returns updated edge weight
void GetNewWeight(const ContractorConfig &config,
                  const SpeedSource &value,
                  const double &segment_length,
                  const EdgeWeight current_duration,
                  const OSMNodeID from,
                  const OSMNodeID to,
                  EdgeWeight &new_segment_weight,
                  EdgeWeight &new_segment_duration)
{
    // Update the edge duration as distance/speed
    new_segment_duration = ConvertToDuration(segment_length, value.speed);

    // Update the edge weight or fallback to the new edge duration
    new_segment_weight =
        ConvertToWeight(value.weight, config.weight_multiplier, new_segment_duration);

    // The check here is enabled by the `--edge-weight-updates-over-factor` flag it logs a warning
    // if the new duration exceeds a heuristic of what a reasonable duration update is
    if (config.log_edge_updates_factor > 0 && current_duration != 0)
    {
        if (current_duration >= (new_segment_duration * config.log_edge_updates_factor))
        {
            auto new_secs = new_segment_duration / 10.;
            auto old_secs = current_duration / 10.;
            auto approx_original_speed = (segment_length / old_secs) * 3.6;
            auto speed_file = config.segment_speed_lookup_paths.at(value.source - 1);
            util::Log(logWARNING) << "[weight updates] Edge weight update from " << old_secs
                                  << "s to " << new_secs << "s  New speed: " << value.speed
                                  << " kph"
                                  << ". Old speed: " << approx_original_speed << " kph"
                                  << ". Segment length: " << segment_length << " m"
                                  << ". Segment: " << from << "," << to << " based on "
                                  << speed_file;
        }
    }
}

int Contractor::Run()
{
    if (config.core_factor > 1.0 || config.core_factor < 0)
    {
        throw util::exception("Core factor must be between 0.0 to 1.0 (inclusive)" + SOURCE_REF);
    }

    TIMER_START(preparing);

    util::Log() << "Reading node weights.";
    std::vector<EdgeWeight> node_weights;
    std::string node_file_name = config.osrm_input_path.string() + ".enw";

    {
        storage::io::FileReader node_file(node_file_name,
                                          storage::io::FileReader::VerifyFingerprint);
        node_file.DeserializeVector(node_weights);
    }
    util::Log() << "Done reading node weights.";

    util::Log() << "Loading edge-expanded graph representation";

    std::vector<extractor::EdgeBasedEdge> edge_based_edge_list;

    EdgeID max_edge_id = LoadEdgeExpandedGraph(config, edge_based_edge_list, node_weights);

#if !defined(NDEBUG)
    if (config.turn_penalty_lookup_paths.empty())
    { // don't check weights consistency with turn updates that can break assertion
        // condition with turn weight penalties negative updates
        CheckWeightsConsistency(config, edge_based_edge_list);
    }
#endif

    // Contracting the edge-expanded graph

    TIMER_START(contraction);
    std::vector<bool> is_core_node;
    std::vector<float> node_levels;
    if (config.use_cached_priority)
    {
        ReadNodeLevels(node_levels);
    }

    util::DeallocatingVector<QueryEdge> contracted_edge_list;
    { // own scope to not keep the contractor around
        GraphContractor graph_contractor(max_edge_id + 1,
                                         adaptToContractorInput(std::move(edge_based_edge_list)),
                                         std::move(node_levels),
                                         std::move(node_weights));
        graph_contractor.Run(config.core_factor);
        graph_contractor.GetEdges(contracted_edge_list);
        graph_contractor.GetCoreMarker(is_core_node);
        graph_contractor.GetNodeLevels(node_levels);
    }
    TIMER_STOP(contraction);

    util::Log() << "Contraction took " << TIMER_SEC(contraction) << " sec";

    std::size_t number_of_used_edges = WriteContractedGraph(max_edge_id, contracted_edge_list);
    WriteCoreNodeMarker(std::move(is_core_node));
    if (!config.use_cached_priority)
    {
        WriteNodeLevels(std::move(node_levels));
    }

    TIMER_STOP(preparing);

    const auto nodes_per_second =
        static_cast<std::uint64_t>((max_edge_id + 1) / TIMER_SEC(contraction));
    const auto edges_per_second =
        static_cast<std::uint64_t>(number_of_used_edges / TIMER_SEC(contraction));

    util::Log() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";
    util::Log() << "Contraction: " << nodes_per_second << " nodes/sec and " << edges_per_second
                << " edges/sec";

    util::Log() << "finished preprocessing";

    return 0;
}

EdgeID
Contractor::LoadEdgeExpandedGraph(const ContractorConfig &config,
                                  std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                                  std::vector<EdgeWeight> &node_weights)
{
    if (config.segment_speed_lookup_paths.size() + config.turn_penalty_lookup_paths.size() > 255)
        throw util::exception("Limit of 255 segment speed and turn penalty files each reached" +
                              SOURCE_REF);

    util::Log() << "Opening " << config.edge_based_graph_path;

    auto mmap_file = [](const std::string &filename, boost::interprocess::mode_t mode) {
        using boost::interprocess::file_mapping;
        using boost::interprocess::mapped_region;

        try
        {
            const file_mapping mapping{filename.c_str(), mode};
            mapped_region region{mapping, mode};
            region.advise(mapped_region::advice_sequential);
            return region;
        }
        catch (const std::exception &e)
        {
            util::Log(logERROR) << "Error while trying to mmap " + filename + ": " + e.what();
            throw;
        }
    };

    const auto edge_based_graph_region =
        mmap_file(config.edge_based_graph_path, boost::interprocess::read_only);

    const bool update_edge_weights = !config.segment_speed_lookup_paths.empty();
    const bool update_turn_penalties = !config.turn_penalty_lookup_paths.empty();

    const auto turn_penalties_index_region = [&] {
        if (update_edge_weights || update_turn_penalties)
        {
            return mmap_file(config.turn_penalties_index_path, boost::interprocess::read_only);
        }
        return boost::interprocess::mapped_region();
    }();

    const auto edge_segment_region = [&] {
        if (update_edge_weights || update_turn_penalties)
        {
            return mmap_file(config.edge_segment_lookup_path, boost::interprocess::read_only);
        }
        return boost::interprocess::mapped_region();
    }();

// Set the struct packing to 1 byte word sizes.  This prevents any padding.  We only use
// this struct once, so any alignment penalty is trivial.  If this is *not* done, then
// the struct will be padded out by an extra 4 bytes, and sizeof() will mean we read
// too much data from the original file.
#pragma pack(push, r1, 1)
    struct EdgeBasedGraphHeader
    {
        util::FingerPrint fingerprint;
        std::uint64_t number_of_edges;
        EdgeID max_edge_id;
    };
#pragma pack(pop, r1)

    BOOST_ASSERT(is_aligned<EdgeBasedGraphHeader>(edge_based_graph_region.get_address()));
    const EdgeBasedGraphHeader graph_header =
        *(reinterpret_cast<const EdgeBasedGraphHeader *>(edge_based_graph_region.get_address()));

    const util::FingerPrint expected_fingerprint = util::FingerPrint::GetValid();
    if (!graph_header.fingerprint.IsValid())
    {
        util::Log(logERROR) << config.edge_based_graph_path << " does not have a valid fingerprint";
        throw util::exception("Invalid fingerprint");
    }

    if (!expected_fingerprint.IsDataCompatible(graph_header.fingerprint))
    {
        util::Log(logERROR) << config.edge_based_graph_path
                            << " is not compatible with this version of OSRM.";
        util::Log(logERROR) << "It was prepared with OSRM "
                            << graph_header.fingerprint.GetMajorVersion() << "."
                            << graph_header.fingerprint.GetMinorVersion() << "."
                            << graph_header.fingerprint.GetPatchVersion() << " but you are running "
                            << OSRM_VERSION;
        util::Log(logERROR) << "Data is only compatible between minor releases.";
        throw util::exception("Incompatible file version" + SOURCE_REF);
    }

    edge_based_edge_list.reserve(graph_header.number_of_edges);
    util::Log() << "Reading " << graph_header.number_of_edges << " edges from the edge based graph";

    auto segment_speed_lookup = CSVFilesParser<Segment, SpeedSource>(
        1, qi::ulong_long >> ',' >> qi::ulong_long, qi::uint_ >> -(',' >> qi::double_))(
        config.segment_speed_lookup_paths);

    auto turn_penalty_lookup = CSVFilesParser<Turn, PenaltySource>(
        1 + config.segment_speed_lookup_paths.size(),
        qi::ulong_long >> ',' >> qi::ulong_long >> ',' >> qi::ulong_long,
        qi::double_ >> -(',' >> qi::double_))(config.turn_penalty_lookup_paths);

    // If we update the edge weights, this file will hold the datasource information for each
    // segment; the other files will also be conditionally filled concurrently if we make an update
    std::vector<uint8_t> geometry_datasource;

    std::vector<extractor::QueryNode> internal_to_external_node_map;
    std::vector<unsigned> geometry_indices;
    std::vector<NodeID> geometry_node_list;
    std::vector<EdgeWeight> geometry_fwd_weight_list;
    std::vector<EdgeWeight> geometry_rev_weight_list;
    std::vector<EdgeWeight> geometry_fwd_duration_list;
    std::vector<EdgeWeight> geometry_rev_duration_list;

    const auto maybe_load_internal_to_external_node_map = [&] {
        if (!update_edge_weights)
            return;

        storage::io::FileReader nodes_file(config.node_based_graph_path,
                                           storage::io::FileReader::HasNoFingerprint);

        nodes_file.DeserializeVector(internal_to_external_node_map);
    };

    const auto maybe_load_geometries = [&] {
        if (!update_edge_weights)
            return;

        storage::io::FileReader geometry_file(config.geometry_path,
                                              storage::io::FileReader::HasNoFingerprint);
        const auto number_of_indices = geometry_file.ReadElementCount32();
        geometry_indices.resize(number_of_indices);
        geometry_file.ReadInto(geometry_indices.data(), number_of_indices);

        const auto number_of_compressed_geometries = geometry_file.ReadElementCount32();

        BOOST_ASSERT(geometry_indices.back() == number_of_compressed_geometries);
        geometry_node_list.resize(number_of_compressed_geometries);
        geometry_fwd_weight_list.resize(number_of_compressed_geometries);
        geometry_rev_weight_list.resize(number_of_compressed_geometries);
        geometry_fwd_duration_list.resize(number_of_compressed_geometries);
        geometry_rev_duration_list.resize(number_of_compressed_geometries);

        if (number_of_compressed_geometries > 0)
        {
            geometry_file.ReadInto(geometry_node_list.data(), number_of_compressed_geometries);
            geometry_file.ReadInto(geometry_fwd_weight_list.data(),
                                   number_of_compressed_geometries);
            geometry_file.ReadInto(geometry_rev_weight_list.data(),
                                   number_of_compressed_geometries);
            geometry_file.ReadInto(geometry_fwd_duration_list.data(),
                                   number_of_compressed_geometries);
            geometry_file.ReadInto(geometry_rev_duration_list.data(),
                                   number_of_compressed_geometries);
        }
    };

    // Folds all our actions into independently concurrently executing lambdas
    tbb::parallel_invoke(maybe_load_internal_to_external_node_map, maybe_load_geometries);

    if (update_edge_weights)
    {
        // Here, we have to update the compressed geometry weights
        // First, we need the external-to-internal node lookup table

        // This is a list of the "data source id" for every segment in the compressed
        // geometry container.  We assume that everything so far has come from the
        // profile (data source 0).  Here, we replace the 0's with the index of the
        // CSV file that supplied the value that gets used for that segment, then
        // we write out this list so that it can be returned by the debugging
        // vector tiles later on.
        geometry_datasource.resize(geometry_fwd_weight_list.size(), 0);

        // Now, we iterate over all the segments stored in the StaticRTree, updating
        // the packed geometry weights in the `.geometries` file (note: we do not
        // update the RTree itself, we just use the leaf nodes to iterate over all segments)
        using LeafNode = util::StaticRTree<extractor::EdgeBasedNode>::LeafNode;

        using boost::interprocess::mapped_region;

        auto region = mmap_file(config.rtree_leaf_path.c_str(), boost::interprocess::read_only);
        region.advise(mapped_region::advice_willneed);
        BOOST_ASSERT(is_aligned<LeafNode>(region.get_address()));

        const auto bytes = region.get_size();
        const auto first = static_cast<const LeafNode *>(region.get_address());
        const auto last = first + (bytes / sizeof(LeafNode));

        // vector to count used speeds for logging
        // size offset by one since index 0 is used for speeds not from external file
        using counters_type = std::vector<std::size_t>;
        std::size_t num_counters = config.segment_speed_lookup_paths.size() + 1;
        tbb::enumerable_thread_specific<counters_type> segment_speeds_counters(
            counters_type(num_counters, 0));
        const constexpr auto LUA_SOURCE = 0;

        tbb::parallel_for_each(first, last, [&](const LeafNode &current_node) {
            auto &counters = segment_speeds_counters.local();
            for (size_t i = 0; i < current_node.object_count; i++)
            {
                const auto &leaf_object = current_node.objects[i];

                const auto forward_begin = geometry_indices.at(leaf_object.packed_geometry_id);
                const auto u_index = forward_begin + leaf_object.fwd_segment_position;
                const auto v_index = forward_begin + leaf_object.fwd_segment_position + 1;

                const extractor::QueryNode &u =
                    internal_to_external_node_map[geometry_node_list[u_index]];
                const extractor::QueryNode &v =
                    internal_to_external_node_map[geometry_node_list[v_index]];

                const double segment_length = util::coordinate_calculation::greatCircleDistance(
                    util::Coordinate{u.lon, u.lat}, util::Coordinate{v.lon, v.lat});

                auto fwd_source = LUA_SOURCE, rev_source = LUA_SOURCE;
                if (auto value = segment_speed_lookup({u.node_id, v.node_id}))
                {
                    EdgeWeight new_segment_weight, new_segment_duration;
                    GetNewWeight(config,
                                 *value,
                                 segment_length,
                                 geometry_fwd_duration_list[u_index],
                                 u.node_id,
                                 v.node_id,
                                 new_segment_weight,
                                 new_segment_duration);

                    geometry_fwd_weight_list[v_index] = new_segment_weight;
                    geometry_fwd_duration_list[v_index] = new_segment_duration;
                    geometry_datasource[v_index] = value->source;
                    fwd_source = value->source;
                }

                if (auto value = segment_speed_lookup({v.node_id, u.node_id}))
                {
                    EdgeWeight new_segment_weight, new_segment_duration;
                    GetNewWeight(config,
                                 *value,
                                 segment_length,
                                 geometry_rev_duration_list[u_index],
                                 v.node_id,
                                 u.node_id,
                                 new_segment_weight,
                                 new_segment_duration);

                    geometry_rev_weight_list[u_index] = new_segment_weight;
                    geometry_rev_duration_list[u_index] = new_segment_duration;
                    geometry_datasource[u_index] = value->source;
                    rev_source = value->source;
                }

                // count statistics for logging
                counters[fwd_source] += 1;
                counters[rev_source] += 1;
            }
        }); // parallel_for_each

        counters_type merged_counters(num_counters, 0);
        for (const auto &counters : segment_speeds_counters)
        {
            for (std::size_t i = 0; i < counters.size(); i++)
            {
                merged_counters[i] += counters[i];
            }
        }

        for (std::size_t i = 0; i < merged_counters.size(); i++)
        {
            if (i == LUA_SOURCE)
            {
                util::Log() << "Used " << merged_counters[LUA_SOURCE]
                            << " speeds from LUA profile or input map";
            }
            else
            {
                // segments_speeds_counters has 0 as LUA, segment_speed_filenames not, thus we need
                // to susbstract 1 to avoid off-by-one error
                util::Log() << "Used " << merged_counters[i] << " speeds from "
                            << config.segment_speed_lookup_paths[i - 1];
            }
        }
    }

    const auto maybe_save_geometries = [&] {
        if (!update_edge_weights)
            return;

        // Now save out the updated compressed geometries
        std::ofstream geometry_stream(config.geometry_path, std::ios::binary);
        if (!geometry_stream)
        {
            const std::string message{"Failed to open " + config.geometry_path + " for writing"};
            throw util::exception(message + SOURCE_REF);
        }
        const unsigned number_of_indices = geometry_indices.size();
        const unsigned number_of_compressed_geometries = geometry_node_list.size();
        geometry_stream.write(reinterpret_cast<const char *>(&number_of_indices), sizeof(unsigned));
        geometry_stream.write(reinterpret_cast<char *>(&(geometry_indices[0])),
                              number_of_indices * sizeof(unsigned));
        geometry_stream.write(reinterpret_cast<const char *>(&number_of_compressed_geometries),
                              sizeof(unsigned));
        geometry_stream.write(reinterpret_cast<char *>(&(geometry_node_list[0])),
                              number_of_compressed_geometries * sizeof(NodeID));
        geometry_stream.write(reinterpret_cast<char *>(&(geometry_fwd_weight_list[0])),
                              number_of_compressed_geometries * sizeof(EdgeWeight));
        geometry_stream.write(reinterpret_cast<char *>(&(geometry_rev_weight_list[0])),
                              number_of_compressed_geometries * sizeof(EdgeWeight));
        geometry_stream.write(reinterpret_cast<char *>(&(geometry_fwd_duration_list[0])),
                              number_of_compressed_geometries * sizeof(EdgeWeight));
        geometry_stream.write(reinterpret_cast<char *>(&(geometry_rev_duration_list[0])),
                              number_of_compressed_geometries * sizeof(EdgeWeight));
    };

    const auto save_datasource_indexes = [&] {
        std::ofstream datasource_stream(config.datasource_indexes_path, std::ios::binary);
        if (!datasource_stream)
        {
            const std::string message{"Failed to open " + config.datasource_indexes_path +
                                      " for writing"};
            throw util::exception(message + SOURCE_REF);
        }
        std::uint64_t number_of_datasource_entries = geometry_datasource.size();
        datasource_stream.write(reinterpret_cast<const char *>(&number_of_datasource_entries),
                                sizeof(number_of_datasource_entries));
        if (number_of_datasource_entries > 0)
        {
            datasource_stream.write(reinterpret_cast<char *>(&(geometry_datasource[0])),
                                    number_of_datasource_entries * sizeof(uint8_t));
        }
    };

    const auto save_datastore_names = [&] {
        std::ofstream datasource_stream(config.datasource_names_path, std::ios::binary);
        if (!datasource_stream)
        {
            const std::string message{"Failed to open " + config.datasource_names_path +
                                      " for writing"};
            throw util::exception(message + SOURCE_REF);
        }
        datasource_stream << "lua profile" << std::endl;

        // Only write the filename, without path or extension.
        // This prevents information leakage, and keeps names short
        // for rendering in the debug tiles.
        for (auto const &name : config.segment_speed_lookup_paths)
        {
            datasource_stream << boost::filesystem::path(name).stem().string() << std::endl;
        }
    };

    tbb::parallel_invoke(maybe_save_geometries, save_datasource_indexes, save_datastore_names);

    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;

    const auto maybe_load_turn_weight_penalties = [&] {
        if (!update_edge_weights && !update_turn_penalties)
            return;
        using storage::io::FileReader;
        FileReader file(config.turn_weight_penalties_path, FileReader::HasNoFingerprint);
        file.DeserializeVector(turn_weight_penalties);
    };

    const auto maybe_load_turn_duration_penalties = [&] {
        if (!update_edge_weights && !update_turn_penalties)
            return;
        using storage::io::FileReader;
        FileReader file(config.turn_duration_penalties_path, FileReader::HasNoFingerprint);
        file.DeserializeVector(turn_duration_penalties);
    };

    tbb::parallel_invoke(maybe_load_turn_weight_penalties, maybe_load_turn_duration_penalties);

    if ((update_edge_weights || update_turn_penalties) && turn_duration_penalties.empty())
    { // Copy-on-write for duration penalties as turn weight penalties
        turn_duration_penalties = turn_weight_penalties;
    }

    // Mapped file pointer for turn indices
    const extractor::lookup::TurnIndexBlock *turn_index_blocks =
        reinterpret_cast<const extractor::lookup::TurnIndexBlock *>(
            turn_penalties_index_region.get_address());
    BOOST_ASSERT(is_aligned<extractor::lookup::TurnIndexBlock>(turn_index_blocks));

    // Mapped file pointers for edge-based graph edges
    auto edge_based_edge_ptr = reinterpret_cast<const extractor::EdgeBasedEdge *>(
        reinterpret_cast<char *>(edge_based_graph_region.get_address()) +
        sizeof(EdgeBasedGraphHeader));
    BOOST_ASSERT(is_aligned<extractor::EdgeBasedEdge>(edge_based_edge_ptr));

    auto edge_segment_byte_ptr = reinterpret_cast<const char *>(edge_segment_region.get_address());

    bool fallback_to_duration = true;
    for (std::uint64_t edge_index = 0; edge_index < graph_header.number_of_edges; ++edge_index)
    {
        // Make a copy of the data from the memory map
        extractor::EdgeBasedEdge inbuffer = edge_based_edge_ptr[edge_index];

        if (update_edge_weights || update_turn_penalties)
        {
            using extractor::lookup::SegmentHeaderBlock;
            using extractor::lookup::SegmentBlock;

            auto header = reinterpret_cast<const SegmentHeaderBlock *>(edge_segment_byte_ptr);
            BOOST_ASSERT(is_aligned<SegmentHeaderBlock>(header));
            edge_segment_byte_ptr += sizeof(SegmentHeaderBlock);

            auto first = reinterpret_cast<const SegmentBlock *>(edge_segment_byte_ptr);
            BOOST_ASSERT(is_aligned<SegmentBlock>(first));
            edge_segment_byte_ptr += sizeof(SegmentBlock) * (header->num_osm_nodes - 1);
            auto last = reinterpret_cast<const SegmentBlock *>(edge_segment_byte_ptr);

            // Find a segment with zero speed and simultaneously compute the new edge weight
            EdgeWeight new_weight = 0;
            EdgeWeight new_duration = 0;
            auto osm_node_id = header->previous_osm_node_id;
            bool skip_edge =
                std::find_if(first, last, [&](const auto &segment) {
                    auto segment_weight = segment.segment_weight;
                    auto segment_duration = segment.segment_duration;
                    if (auto value = segment_speed_lookup({osm_node_id, segment.this_osm_node_id}))
                    {
                        // If we hit a 0-speed edge, then it's effectively not traversible.
                        // We don't want to include it in the edge_based_edge_list.
                        if (value->speed == 0)
                            return true;

                        segment_duration = ConvertToDuration(segment.segment_length, value->speed);

                        segment_weight = ConvertToWeight(
                            value->weight, config.weight_multiplier, segment_duration);
                    }

                    // Update the edge weight and the next OSM node ID
                    osm_node_id = segment.this_osm_node_id;
                    new_weight += segment_weight;
                    new_duration += segment_duration;
                    return false;
                }) != last;

            // Update the node-weight cache. This is the weight of the edge-based-node only,
            // it doesn't include the turn. We may visit the same node multiple times, but
            // we should always assign the same value here.
            node_weights[inbuffer.source] = new_weight;

            // We found a zero-speed edge, so we'll skip this whole edge-based-edge which
            // effectively removes it from the routing network.
            if (skip_edge)
                continue;

            // Get the turn penalty and update to the new value if required
            const auto &turn_index = turn_index_blocks[edge_index];
            auto turn_weight_penalty = turn_weight_penalties[edge_index];
            auto turn_duration_penalty = turn_duration_penalties[edge_index];
            if (auto value = turn_penalty_lookup(turn_index))
            {
                turn_duration_penalty =
                    boost::numeric_cast<TurnPenalty>(std::round(value->duration * 10.));
                turn_weight_penalty = boost::numeric_cast<TurnPenalty>(
                    std::round(std::isfinite(value->weight)
                                   ? value->weight * config.weight_multiplier
                                   : turn_duration_penalty * config.weight_multiplier / 10.));

                const auto weight_min_value = static_cast<EdgeWeight>(header->num_osm_nodes);
                if (turn_weight_penalty + new_weight < weight_min_value)
                {
                    util::Log(logWARNING) << "turn penalty " << turn_weight_penalty << " for turn "
                                          << turn_index.from_id << ", " << turn_index.via_id << ", "
                                          << turn_index.to_id
                                          << " is too negative: clamping turn weight to "
                                          << weight_min_value;

                    turn_weight_penalty = weight_min_value - new_weight;
                }

                turn_duration_penalties[edge_index] = turn_duration_penalty;
                turn_weight_penalties[edge_index] = turn_weight_penalty;

                // Is fallback of duration to weight values allowed
                fallback_to_duration &= (turn_duration_penalty == turn_weight_penalty);
            }

            // Update edge weight
            inbuffer.weight = new_weight + turn_weight_penalty;
            inbuffer.duration = new_duration + turn_duration_penalty;
        }

        edge_based_edge_list.emplace_back(std::move(inbuffer));
    }

    if (update_turn_penalties)
    {
        if (fallback_to_duration)
        { // Turn duration penalties are identical to turn weight penalties
            // Save empty data vector, so turn weight penalties will be used by data facade.
            turn_duration_penalties.clear();
        }

        const auto save_penalties = [](const auto &filename, const auto &data) -> void {
            storage::io::FileWriter file(filename, storage::io::FileWriter::HasNoFingerprint);
            file.SerializeVector(data);
        };

        tbb::parallel_invoke(
            [&] { save_penalties(config.turn_weight_penalties_path, turn_weight_penalties); },
            [&] { save_penalties(config.turn_duration_penalties_path, turn_duration_penalties); });
    }

    util::Log() << "Done reading edges";
    return graph_header.max_edge_id;
}

void Contractor::ReadNodeLevels(std::vector<float> &node_levels) const
{
    storage::io::FileReader order_file(config.level_output_path,
                                       storage::io::FileReader::HasNoFingerprint);

    const auto level_size = order_file.ReadElementCount32();
    node_levels.resize(level_size);
    order_file.ReadInto(node_levels);
}

void Contractor::WriteNodeLevels(std::vector<float> &&in_node_levels) const
{
    std::vector<float> node_levels(std::move(in_node_levels));

    boost::filesystem::ofstream order_output_stream(config.level_output_path, std::ios::binary);

    unsigned level_size = node_levels.size();
    order_output_stream.write((char *)&level_size, sizeof(unsigned));
    order_output_stream.write((char *)node_levels.data(), sizeof(float) * node_levels.size());
}

void Contractor::WriteCoreNodeMarker(std::vector<bool> &&in_is_core_node) const
{
    std::vector<bool> is_core_node(std::move(in_is_core_node));
    std::vector<char> unpacked_bool_flags(std::move(is_core_node.size()));
    for (auto i = 0u; i < is_core_node.size(); ++i)
    {
        unpacked_bool_flags[i] = is_core_node[i] ? 1 : 0;
    }

    boost::filesystem::ofstream core_marker_output_stream(config.core_output_path,
                                                          std::ios::binary);
    unsigned size = unpacked_bool_flags.size();
    core_marker_output_stream.write((char *)&size, sizeof(unsigned));
    core_marker_output_stream.write((char *)unpacked_bool_flags.data(),
                                    sizeof(char) * unpacked_bool_flags.size());
}

std::size_t
Contractor::WriteContractedGraph(unsigned max_node_id,
                                 const util::DeallocatingVector<QueryEdge> &contracted_edge_list)
{
    // Sorting contracted edges in a way that the static query graph can read some in in-place.
    tbb::parallel_sort(contracted_edge_list.begin(), contracted_edge_list.end());
    const std::uint64_t contracted_edge_count = contracted_edge_list.size();
    util::Log() << "Serializing compacted graph of " << contracted_edge_count << " edges";

    const util::FingerPrint fingerprint = util::FingerPrint::GetValid();
    boost::filesystem::ofstream hsgr_output_stream(config.graph_output_path, std::ios::binary);
    hsgr_output_stream.write((char *)&fingerprint, sizeof(util::FingerPrint));
    const NodeID max_used_node_id = [&contracted_edge_list] {
        NodeID tmp_max = 0;
        for (const QueryEdge &edge : contracted_edge_list)
        {
            BOOST_ASSERT(SPECIAL_NODEID != edge.source);
            BOOST_ASSERT(SPECIAL_NODEID != edge.target);
            tmp_max = std::max(tmp_max, edge.source);
            tmp_max = std::max(tmp_max, edge.target);
        }
        return tmp_max;
    }();

    util::Log(logDEBUG) << "input graph has " << (max_node_id + 1) << " nodes";
    util::Log(logDEBUG) << "contracted graph has " << (max_used_node_id + 1) << " nodes";

    std::vector<util::StaticGraph<EdgeData>::NodeArrayEntry> node_array;
    // make sure we have at least one sentinel
    node_array.resize(max_node_id + 2);

    util::Log() << "Building node array";
    util::StaticGraph<EdgeData>::EdgeIterator edge = 0;
    util::StaticGraph<EdgeData>::EdgeIterator position = 0;
    util::StaticGraph<EdgeData>::EdgeIterator last_edge;

    // initializing 'first_edge'-field of nodes:
    for (const auto node : util::irange(0u, max_used_node_id + 1))
    {
        last_edge = edge;
        while ((edge < contracted_edge_count) && (contracted_edge_list[edge].source == node))
        {
            ++edge;
        }
        node_array[node].first_edge = position; //=edge
        position += edge - last_edge;           // remove
    }

    for (const auto sentinel_counter :
         util::irange<unsigned>(max_used_node_id + 1, node_array.size()))
    {
        // sentinel element, guarded against underflow
        node_array[sentinel_counter].first_edge = contracted_edge_count;
    }

    util::Log() << "Serializing node array";

    RangebasedCRC32 crc32_calculator;
    const unsigned edges_crc32 = crc32_calculator(contracted_edge_list);
    util::Log() << "Writing CRC32: " << edges_crc32;

    const std::uint64_t node_array_size = node_array.size();
    // serialize crc32, aka checksum
    hsgr_output_stream.write((char *)&edges_crc32, sizeof(unsigned));
    // serialize number of nodes
    hsgr_output_stream.write((char *)&node_array_size, sizeof(std::uint64_t));
    // serialize number of edges
    hsgr_output_stream.write((char *)&contracted_edge_count, sizeof(std::uint64_t));
    // serialize all nodes
    if (node_array_size > 0)
    {
        hsgr_output_stream.write((char *)&node_array[0],
                                 sizeof(util::StaticGraph<EdgeData>::NodeArrayEntry) *
                                     node_array_size);
    }

    // serialize all edges
    util::Log() << "Building edge array";
    std::size_t number_of_used_edges = 0;

    util::StaticGraph<EdgeData>::EdgeArrayEntry current_edge;
    for (const auto edge : util::irange<std::size_t>(0UL, contracted_edge_list.size()))
    {
        // some self-loops are required for oneway handling. Need to assertthat we only keep these
        // (TODO)
        // no eigen loops
        // BOOST_ASSERT(contracted_edge_list[edge].source != contracted_edge_list[edge].target ||
        // node_represents_oneway[contracted_edge_list[edge].source]);
        current_edge.target = contracted_edge_list[edge].target;
        current_edge.data = contracted_edge_list[edge].data;

        // every target needs to be valid
        BOOST_ASSERT(current_edge.target <= max_used_node_id);
#ifndef NDEBUG
        if (current_edge.data.weight <= 0)
        {
            util::Log(logWARNING) << "Edge: " << edge
                                  << ",source: " << contracted_edge_list[edge].source
                                  << ", target: " << contracted_edge_list[edge].target
                                  << ", weight: " << current_edge.data.weight;

            util::Log(logWARNING) << "Failed at adjacency list of node "
                                  << contracted_edge_list[edge].source << "/"
                                  << node_array.size() - 1;
            throw util::exception("Edge weight is <= 0" + SOURCE_REF);
        }
#endif
        hsgr_output_stream.write((char *)&current_edge,
                                 sizeof(util::StaticGraph<EdgeData>::EdgeArrayEntry));

        ++number_of_used_edges;
    }

    return number_of_used_edges;
}

} // namespace contractor
} // namespace osrm
