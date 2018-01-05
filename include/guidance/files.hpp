#ifndef OSRM_GUIDANCE_FILES_HPP
#define OSRM_GUIDANCE_FILES_HPP

#include "guidance/serialization.hpp"
#include "guidance/turn_data_container.hpp"

#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/serialization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace guidance
{
namespace files
{

// reads .osrm.edges
template <typename TurnDataT>
inline void readTurnData(const boost::filesystem::path &path, TurnDataT &turn_data)
{
    static_assert(std::is_same<guidance::TurnDataContainer, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataView, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataExternalContainer, TurnDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, turn_data);
}

// writes .osrm.edges
template <typename TurnDataT>
inline void writeTurnData(const boost::filesystem::path &path, const TurnDataT &turn_data)
{
    static_assert(std::is_same<guidance::TurnDataContainer, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataView, TurnDataT>::value ||
                      std::is_same<guidance::TurnDataExternalContainer, TurnDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, turn_data);
}
}
}
}

#endif
