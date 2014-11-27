#ifndef SCHEMATIZED_SUBPATH_EMBEDDING_H_
#define SCHEMATIZED_SUBPATH_EMBEDDING_H_

#include <vector>

class SubPath;
class SchematizedPlane;
class SymbolicCoordinate;

/**
 * This class joins schematized subpaths to one big schematized path.
 */
class SchematizedSubPathEmbedding
{
public:
    SchematizedSubPathEmbedding(const SchematizedPlane& plane);

    void embedd(std::vector<SubPath>& subpaths, std::vector<SymbolicCoordinate>& mergedPath) const;

private:
    void computeDeltas(std::vector<SubPath>& subpaths) const;

    void mergeSubpaths(const std::vector<SubPath>& subpaths, std::vector<SymbolicCoordinate>& mergedPath) const;

    const SchematizedPlane& plane;
};

#endif
