#include "util/timing_util.hpp"

#include "osrm/match_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include <boost/assert.hpp>

#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include <cstdlib>

int main(int argc, const char *argv[]) try
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " data.osrm\n";
        return EXIT_FAILURE;
    }

    using namespace osrm;

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {argv[1]};
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    OSRM osrm{config};

    // Route in monaco
    MatchParameters params;
    params.overview = RouteParameters::OverviewType::False;
    params.steps = false;

    using osrm::util::FloatCoordinate;
    using osrm::util::FloatLatitude;
    using osrm::util::FloatLongitude;

    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.422176599502563}, FloatLatitude{43.73754595167546}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.421715259552002}, FloatLatitude{43.73744517900973}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.421489953994752}, FloatLatitude{43.73738316497729}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.421286106109619}, FloatLatitude{43.737274640266}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.420910596847533}, FloatLatitude{43.73714285999499}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.420696020126342}, FloatLatitude{43.73699557581948}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.42049217224121}, FloatLatitude{43.73690255404829}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.420309782028198}, FloatLatitude{43.73672426191624}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.420159578323363}, FloatLatitude{43.7366622471372}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.420148849487305}, FloatLatitude{43.736623487867654}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419934272766113}, FloatLatitude{43.73647620241466}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419805526733398}, FloatLatitude{43.736228141885455}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419601678848267}, FloatLatitude{43.736142870841206}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419376373291015}, FloatLatitude{43.735956824504974}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419247627258301}, FloatLatitude{43.73574752168583}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419043779373169}, FloatLatitude{43.73566224995717}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418732643127442}, FloatLatitude{43.735406434042645}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418657541275024}, FloatLatitude{43.735321161828274}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418593168258667}, FloatLatitude{43.73521263337983}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418367862701416}, FloatLatitude{43.73508084857086}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418346405029297}, FloatLatitude{43.73484828643578}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.4180567264556885}, FloatLatitude{43.734437424456566}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417809963226318}, FloatLatitude{43.73414284243448}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417863607406615}, FloatLatitude{43.73375523230292}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417809963226318}, FloatLatitude{43.73386376339265}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417895793914795}, FloatLatitude{43.73365445325776}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418067455291747}, FloatLatitude{43.73343739012297}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41803526878357}, FloatLatitude{43.73319706930599}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418024539947509}, FloatLatitude{43.73295674752463}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417906522750854}, FloatLatitude{43.73284821479115}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417917251586914}, FloatLatitude{43.7327551865773}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417434453964233}, FloatLatitude{43.73281720540258}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.4173808097839355}, FloatLatitude{43.73307303237796}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41750955581665}, FloatLatitude{43.73328234454499}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417563199996948}, FloatLatitude{43.73352266501975}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41750955581665}, FloatLatitude{43.733770736756355}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417466640472412}, FloatLatitude{43.73409632935116}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417230606079102}, FloatLatitude{43.73428238146768}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41724133491516}, FloatLatitude{43.73405756842078}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.4169838428497314}, FloatLatitude{43.73449168940785}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41701602935791}, FloatLatitude{43.734615723397525}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41704821586609}, FloatLatitude{43.73487929477265}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41725206375122}, FloatLatitude{43.734949063471895}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.4173808097839355}, FloatLatitude{43.73533666587628}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41750955581665}, FloatLatitude{43.735623490040375}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417799234390259}, FloatLatitude{43.73577852955704}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.4180781841278085}, FloatLatitude{43.735972328388435}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41850733757019}, FloatLatitude{43.73608860738618}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418850660324096}, FloatLatitude{43.736228141885455}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419086694717407}, FloatLatitude{43.73636767605958}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419333457946777}, FloatLatitude{43.73664674343239}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419633865356444}, FloatLatitude{43.73676302112054}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419784069061279}, FloatLatitude{43.737096349241845}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.420030832290649}, FloatLatitude{43.73720487427631}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419601678848267}, FloatLatitude{43.73708084564945}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419333457946777}, FloatLatitude{43.73708084564945}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.419043779373169}, FloatLatitude{43.737158363571325}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418915033340454}, FloatLatitude{43.737305647346446}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41848587989807}, FloatLatitude{43.7374916894919}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.418271303176879}, FloatLatitude{43.73746843425534}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417960166931152}, FloatLatitude{43.73744517900973}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417885065078735}, FloatLatitude{43.737212626056944}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417563199996948}, FloatLatitude{43.73703433484817}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.4173057079315186}, FloatLatitude{43.73692580950463}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.417144775390625}, FloatLatitude{43.7367707729584}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416973114013672}, FloatLatitude{43.73653821738638}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416855096817017}, FloatLatitude{43.73639868360965}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.4167799949646}, FloatLatitude{43.736142870841206}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.41675853729248}, FloatLatitude{43.735848297208605}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416619062423706}, FloatLatitude{43.73567000193752}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416543960571288}, FloatLatitude{43.735406434042645}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416479587554932}, FloatLatitude{43.73529790574875}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416415214538574}, FloatLatitude{43.73515061703527}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416350841522218}, FloatLatitude{43.73490255101476}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416340112686156}, FloatLatitude{43.73475526132885}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416222095489501}, FloatLatitude{43.73446068087028}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416243553161621}, FloatLatitude{43.73430563794159}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.416050434112548}, FloatLatitude{43.73403431185051}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.415814399719239}, FloatLatitude{43.73382500231174}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.415750026702881}, FloatLatitude{43.73354592178871}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.415513992309569}, FloatLatitude{43.73347615145474}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{7.415342330932617}, FloatLatitude{43.733251335381205}});

    TIMER_START(routes);
    auto NUM = 100;
    for (int i = 0; i < NUM; ++i)
    {
        engine::api::ResultT result = json::Object();
        const auto rc = osrm.Match(params, result);
        auto &json_result = result.get<json::Object>();
        if (rc != Status::Ok ||
            json_result.values.at("matchings").get<json::Array>().values.size() != 1)
        {
            return EXIT_FAILURE;
        }
    }
    TIMER_STOP(routes);
    std::cout << (TIMER_MSEC(routes) / NUM) << "ms/req at " << params.coordinates.size()
              << " coordinate" << std::endl;
    std::cout << (TIMER_MSEC(routes) / NUM / params.coordinates.size()) << "ms/coordinate"
              << std::endl;

    return EXIT_SUCCESS;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
