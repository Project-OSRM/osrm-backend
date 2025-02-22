#ifndef OSRM_EXTRACTOR_INTERSECTION_COORDINATE_EXTRACTOR_HPP_
#define OSRM_EXTRACTOR_INTERSECTION_COORDINATE_EXTRACTOR_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/query_node.hpp"

#include "util/coordinate.hpp"
#include "util/node_based_graph.hpp"

#include <utility>
#include <vector>

namespace osrm::extractor::intersection
{

class CoordinateExtractor
{
  public:
    CoordinateExtractor(const util::NodeBasedDynamicGraph &node_based_graph,
                        const extractor::CompressedEdgeContainer &compressed_geometries,
                        const std::vector<util::Coordinate> &node_coordinates);

    /* Find a interpolated coordinate a long the compressed geometries. The desired coordinate
     * should be in a certain distance. This method is dedicated to find representative coordinates
     * at turns.
     * Note: The segment between intersection and turn coordinate can be zero, if the OSM modelling
     * is unfortunate. See https://github.com/Project-OSRM/osrm-backend/issues/3470
     */
    [[nodiscard]] util::Coordinate
    GetCoordinateAlongRoad(const NodeID intersection_node,
                           const EdgeID turn_edge,
                           const bool traversed_in_reverse,
                           const NodeID to_node,
                           const std::uint8_t number_of_in_lanes) const;

    // Given a set of precomputed coordinates, select the representative coordinate along the road
    // that best describes the turn
    [[nodiscard]] util::Coordinate
    ExtractRepresentativeCoordinate(const NodeID intersection_node,
                                    const EdgeID turn_edge,
                                    const bool traversed_in_reverse,
                                    const NodeID to_node,
                                    const std::uint8_t intersection_lanes,
                                    std::vector<util::Coordinate> coordinates) const;

    // instead of finding only a single coordinate, we can also list all coordinates along a
    // road.
    [[nodiscard]] std::vector<util::Coordinate>
    GetCoordinatesAlongRoad(const NodeID intersection_node,
                            const EdgeID turn_edge,
                            const bool traversed_in_reverse,
                            const NodeID to_node) const;

    // wrapper in case of normal forward edges (traversed_in_reverse = false, to_node =
    // node_based_graph.GetTarget(turn_edge)
    [[nodiscard]] std::vector<util::Coordinate>
    GetForwardCoordinatesAlongRoad(const NodeID from, const EdgeID turn_edge) const;

    // a less precise way to compute coordinates along a route. Due to the heavy interaction of
    // graph traversal and turn instructions, we often don't care for high precision. We only want
    // to check for available connections in order, or find (with room for error) the straightmost
    // turn. This function will offer a bit more error potential but allow for much higher
    // performance
    [[nodiscard]] util::Coordinate GetCoordinateCloseToTurn(const NodeID from_node,
                                                            const EdgeID turn_edge,
                                                            const bool traversed_in_reverse,
                                                            const NodeID to_node) const;

    /* When extracting the coordinates, we first extract all coordinates. We don't care about most
     * of them, though.
     *
     * Our very first step trims the coordinates to a saller set, close to the intersection.. The
     * idea here is to filter all coordinates at the end of the road and consider only the formi
     * close to the intersection:
     *
     * a -------------- v ----------.
     *                                 .
     *                                   .
     *                                   .
     *                                   b
     *
     * For calculating the turn angle for the intersection at `a`, we do not care about the turn
     * between `v` and `b`. This calculation trims the coordinates to the ones immediately at the
     * intersection.
     *
     * The optional length cache needs to store the accumulated distance up to the respective
     * coordinate index [0,d(0,1),...]
     */
    [[nodiscard]] std::vector<util::Coordinate>
    TrimCoordinatesToLength(std::vector<util::Coordinate> coordinates,
                            const double desired_length,
                            const std::vector<double> &length_cache = {}) const;

    [[nodiscard]] std::vector<double>
    PrepareLengthCache(const std::vector<util::Coordinate> &coordinates, const double limit) const;

    /* when looking at a set of coordinates, this function allows trimming the vector to a smaller,
     * only containing coordinates up to a given distance along the path. The last coordinate might
     * be interpolated
     */
    [[nodiscard]] std::vector<util::Coordinate>
    TrimCoordinatesByLengthFront(std::vector<util::Coordinate> coordinates,
                                 const double desired_length) const;

    /*
     * to correct for the initial offset, we move the lookahead coordinate close
     * to the original road. We do so by subtracting the difference between the
     * turn coordinate and the offset coordinate from the lookahead coordinge:
     *
     * a ------ b ------ c
     *          |
     *          d
     *             \
     *                \
     *                   e
     *
     * is converted to:
     *
     * a ------ b ------ c
     *             \
     *                \
     *                   e
     *
     * for fixpoint `b`, vector_base `d` and vector_head `e`
     */
    [[nodiscard]] util::Coordinate GetCorrectedCoordinate(const util::Coordinate fixpoint,
                                                          const util::Coordinate vector_base,
                                                          const util::Coordinate vector_head) const;

    /* generate a uniform vector of coordinates in same range distances
     *
     * Turns:
     * x ------------ x -- x - x
     *
     * Into:
     * x -- x -- x -- x -- x - x
     */
    [[nodiscard]] std::vector<util::Coordinate>
    SampleCoordinates(const std::vector<util::Coordinate> &coordinates,
                      const double length,
                      const double rate) const;

    // find the coordinate at a specific distance in the vector
    util::Coordinate
    ExtractCoordinateAtLength(const double distance,
                              const std::vector<util::Coordinate> &coordinates) const;
    util::Coordinate ExtractCoordinateAtLength(const double distance,
                                               const std::vector<util::Coordinate> &coordinates,
                                               const std::vector<double> &length_cache) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const extractor::CompressedEdgeContainer &compressed_geometries;
    const std::vector<util::Coordinate> &node_coordinates;

    double ComputeInterpolationFactor(const double desired_distance,
                                      const double distance_to_first,
                                      const double distance_to_second) const;

    std::pair<util::Coordinate, util::Coordinate>
    RegressionLine(const std::vector<util::Coordinate> &coordinates) const;

    /* In an ideal world, the road would only have two coordinates if it goes mainly straigt. Since
     * OSM is operating on noisy data, we have some variations going straight.
     *
     *              b                       d
     * a ---------------------------------------------- e
     *                           c
     *
     * The road from a-e offers a lot of variation, even though it is mostly straight. Here we
     * calculate the distances of all nodes in between to the straight line between a and e. If the
     * distances inbetween are small, we assume a straight road. To calculate these distances, we
     * don't use the coordinates of the road itself but our just calculated regression vector
     */
    double GetMaxDeviation(std::vector<util::Coordinate>::const_iterator range_begin,
                           const std::vector<util::Coordinate>::const_iterator &range_end,
                           const util::Coordinate straight_begin,
                           const util::Coordinate straight_end) const;

    /*
     * the curve is still best described as looking at the very first vector for the turn angle.
     * Consider:
     *
     * |
     * a - 1
     * |       o
     * |         2
     * |          o
     * |           3
     * |           o
     * |           4
     *
     * The turn itself from a-1 would be considered as a 90 degree turn, even though the road is
     * taking a turn later.
     * In this situaiton we return the very first coordinate, describing the road just at the
     * turn.
     * As an added benefit, we get a straight turn at a curved road:
     *
     *            o   b   o
     *      o                   o
     *   o                         o
     *  o                           o
     *  o                           o
     *  a                           c
     *
     * The turn from a-b to b-c is straight. With every vector we go further down the road, the
     * turn
     * angle would get stronger. Therefore we consider the very first coordinate as our best
     * choice
     */
    bool IsCurve(const std::vector<util::Coordinate> &coordinates,
                 const std::vector<double> &segment_distances,
                 const double segment_length,
                 const double considered_lane_width,
                 const extractor::NodeBasedEdgeClassification &edge_data) const;

    /*
     * If the very first coordinate is within lane offsets and the rest offers a near straight line,
     * we use an offset coordinate.
     *
     * ----------------------------------------
     *
     * ----------------------------------------
     *   a -
     * ----------------------------------------
     *        \
     * ----------------------------------------
     *           \
     *             b --------------------c
     *
     * Will be considered a very slight turn, instead of the near 90 degree turn we see right here.
     */
    bool IsDirectOffset(const std::vector<util::Coordinate> &coordinates,
                        const std::size_t straight_index,
                        const double straight_distance,
                        const double segment_length,
                        const std::vector<double> &segment_distances,
                        const std::uint8_t considered_lanes) const;
};

} // namespace osrm::extractor::intersection

#endif // OSRM_EXTRACTOR_INTERSECTION_COORDINATE_EXTRACTOR_HPP_
