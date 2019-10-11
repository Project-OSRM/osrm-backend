#ifndef OSRM_CELLS_UPDATED_RECORD_HPP
#define OSRM_CELLS_UPDATED_RECORD_HPP

#include "util/log.hpp"
#include "updater/updater.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "tbb/concurrent_unordered_set.h"
#include <vector>
#include <unordered_set>
#include <iomanip>

namespace osrm
{
namespace customizer
{
namespace detail
{

class CellUpdateRecordImpl
{
//using CellIDSet = std::unordered_set<CellID>
using CellIDSet = tbb::concurrent_unordered_set<CellID>;
using MultiLevelCellIDSet = std::vector<CellIDSet>;
public:
    CellUpdateRecordImpl(const partitioner::MultiLevelPartition& mlp, bool incremental)
    : partition(mlp),
      isIncremental(incremental)
    {
        MultiLevelCellIDSet tmp(partition.GetNumberOfLevels() - 1, CellIDSet());
        cellsets = std::move(tmp);
    }

    void Collect(updater::NodeSetViewerPtr node_updated)
    {
        if (!isIncremental || !node_updated)
        {
            return;
        }

        for (const auto& n : *node_updated)
        {
            for (std::size_t level = 1; level < partition.GetNumberOfLevels(); ++level)
            {
                cellsets[level-1].insert(partition.GetCell(level, n));
            }
        }
    }

    bool Check(LevelID l, CellID c) const
    {
        // always return true for none-incremental mode
        if (!isIncremental)
        {
            return true;
        }

        if (l < 1 || (l - 1) >= static_cast<LevelID>(cellsets.size())) 
        {
            util::Log(logERROR) << "Incorrect level be passed to" 
                                << typeid(*this).name() << "::" << __func__;
            return false;
        }
        return (cellsets[l-1].find(c) != cellsets[l-1].end());
    }

    void Clear()
    {
        for (size_t i = 0; i < cellsets.size(); ++i)
        {
            cellsets[i].clear();
        }
    }

    std::string Statistic() const
    {
        if (!isIncremental) 
        {
            return "Nothing has been recorded in cell_update_record.\n";
        }
        else
        {
            std::ostringstream ss;

            int sumOfUpdates = 0;
            ss << "Cell Update Status(count for levels): (";
            for (size_t i = 0; i < cellsets.size(); ++i)
            {
                ss << cellsets[i].size();
                sumOfUpdates += cellsets[i].size();
                if (i != cellsets.size() - 1)
                {
                    ss << ",";
                }
            }
            ss << ") ";

            ss << "of (";
            int sumOfCells = 0;
            for (LevelID level = 1; level < partition.GetNumberOfLevels(); ++level)
            {
                ss << partition.GetNumberOfCells(level);
                sumOfCells += partition.GetNumberOfCells(level);
                if (level != (partition.GetNumberOfLevels() - 1))
                {
                    ss << ",";
                }
            }
            ss << ") be updated.";

            float percentage = (float)(sumOfUpdates * 100) / sumOfCells;
            ss <<std::setprecision(4) << "  About " << percentage << "% in total.\n";

            return ss.str();
        }
    }

private:
    MultiLevelCellIDSet                     cellsets;
    const partitioner::MultiLevelPartition  &partition;
    bool                                    isIncremental;
};

}

using CellUpdateRecord = detail::CellUpdateRecordImpl;

}
}

#endif
