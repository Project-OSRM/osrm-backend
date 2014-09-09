#ifndef SIMPLE_PATH_SCHEMATIZATION_H_
#define SIMPLE_PATH_SCHEMATIZATION_H_

#include "../Util/TimingUtil.h"

#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/SymbolicCoordinate.h"
#include "../DataStructures/SchematizedPlane.h"

#include "MinCostSchematization.h"
#include "SchematizedSubpathEmbedding.h"
#include "MonotoneDecomposition.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

#include <unordered_map>

class SimplePathSchematization
{
public:
    SimplePathSchematization(unsigned d, double min_length)
    : plane(d, min_length)
    {
    }

    /**
     * Variant that retains segment information.
     */
    void schematize(const std::vector<SegmentInformation>& segment_info,
                    std::vector<SegmentInformation>& schematized_info)
    {
        if (segment_info.size() < 2)
        {
            schematized_info = segment_info;
            return;
        }

        std::vector<SymbolicCoordinate> symbolic_path;
        double scaling;
        FixedPointCoordinate origin;
        transformFixedPointToSymbolic(segment_info, symbolic_path, scaling, origin,
            [](const SegmentInformation& s, FixedPointCoordinate& c)
            {
                c = s.location;
                return s.necessary;
            },
            [](const SegmentInformation& s)
            {
                return s.name_id;
            }
        );
        BOOST_ASSERT(segment_info.size() >= symbolic_path.size());

        // bounding box has height/width 0
        if (scaling == 0.0)
        {
            schematized_info = segment_info;
            return;
        }

        std::vector<SymbolicCoordinate> symbolic_schematized_path;
        computeSchematizedPath(symbolic_path, symbolic_schematized_path);
        transformSchematizedPath(segment_info, symbolic_schematized_path, scaling, origin, schematized_info);
    }

    void schematize(const std::vector<FixedPointCoordinate>& path,
                    std::vector<FixedPointCoordinate>& schematized_path) const
    {
        if (path.size() < 2)
        {
            schematized_path = path;
            return;
        }

        std::vector<SymbolicCoordinate> symbolic_path;
        double scaling;
        FixedPointCoordinate origin;
        transformFixedPointToSymbolic(path, symbolic_path, scaling, origin,
            [](const FixedPointCoordinate& c1, FixedPointCoordinate& c2)
            {
                c2 = c1;
                return true;
            },
            [](const FixedPointCoordinate& c)
            {
                return 0;
            }
        );

        // bounding box has height/width 0
        if (scaling == 0.0)
        {
            schematized_path = path;
            return;
        }

        BOOST_ASSERT(path.size() == symbolic_path.size());
        std::vector<SymbolicCoordinate> symbolic_schematized_path;
        computeSchematizedPath(symbolic_path, symbolic_schematized_path);
        transformSchematizedPath(symbolic_schematized_path, scaling, origin, schematized_path);
    }

private:
    void computeSchematizedPath(const std::vector<SymbolicCoordinate>& symbolic_path,
                         std::vector<SymbolicCoordinate>& symbolic_schematized_path) const
    {
        std::vector<SubPath> subpaths;
        MonotoneDecomposition::axisMonotoneDecomposition(symbolic_path, subpaths);

        MinCostSchematization min_cost_schematization(plane);

        unsigned sum_nodes = 0;
        // could be parallized
        for (auto& s : subpaths)
        {
            MonotoneDecomposition::transformToXMonotoneIncreasing(s);

            min_cost_schematization.schematize(s);

            MonotoneDecomposition::transformFromXMonotoneIncreasing(s);
            sum_nodes += s.nodes.size();
        }


        SchematizedSubPathEmbedding schematized_embedding(plane);
        schematized_embedding.embedd(subpaths, symbolic_schematized_path);
    }

    void transformSchematizedPath(const std::vector<SymbolicCoordinate>& symbolic_schematized_path,
                                  double scaling,
                                  const FixedPointCoordinate& origin,
                                  std::vector<FixedPointCoordinate>& schematized_path) const
    {
        // transfrom back to lat/lon
        schematized_path.resize(symbolic_schematized_path.size());
        std::transform(symbolic_schematized_path.begin(), symbolic_schematized_path.end(), schematized_path.begin(),
            [&scaling, &origin](SymbolicCoordinate s)
            {
                return transformSymbolicToFixed(s, scaling, origin);
            }
        );
    }

    void transformSchematizedPath(const std::vector<SegmentInformation>& segment_info,
                                  const std::vector<SymbolicCoordinate>& symbolic_schematized_path,
                                  double scaling,
                                  const FixedPointCoordinate& origin,
                                  std::vector<SegmentInformation>& schematized_path) const
    {
        // transfrom back to lat/lon
        schematized_path.resize(symbolic_schematized_path.size());
        std::transform(symbolic_schematized_path.begin(), symbolic_schematized_path.end(), schematized_path.begin(),
            [&scaling, &origin, &segment_info](SymbolicCoordinate sym)
            {
                SegmentInformation segment = segment_info[sym.original_idx];
                segment.location = transformSymbolicToFixed(sym, scaling, origin);
                return segment;
            }
        );
    }

private:
    SchematizedPlane plane;
};

#endif
