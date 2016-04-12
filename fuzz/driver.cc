#include "server/api/parameters_parser.hpp"

#include "engine/api/base_parameters.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/tile_parameters.hpp"
#include "engine/api/trip_parameters.hpp"

#include <iterator>
#include <string>

/*
 * First pass at fuzzing the server, without any libosrm setup.
 * Later we want keep state across fuzz testing invocations via:
 *
 * struct State { State() { setup_osrm(); } };
 * static State state;
 */

extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data, unsigned long size)
{
    std::string in(reinterpret_cast<const char *>(data), size);

    auto first = begin(in);
    const auto last = end(in);

    (void)osrm::server::api::parseParameters<osrm::engine::api::RouteParameters>(first, last);

    return 0; /* Always return zero, sanitizers hard-abort */
}
