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

    // Match traces to the road network in our Berlin test dataset
    MatchParameters params;
    params.overview = RouteParameters::OverviewType::False;
    params.steps = false;

    using osrm::util::FloatCoordinate;
    using osrm::util::FloatLatitude;
    using osrm::util::FloatLongitude;

    // Grab trace, or: go to geojson.io, create linestring.
    // Extract coordinates: jq '.features[].geometry.coordinates[]' coordinates.json

    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.410401344299316}, FloatLatitude{52.522749270442254}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.410615921020508}, FloatLatitude{52.52284066124772}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.410787582397461}, FloatLatitude{52.522932051863044}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.411259651184082}, FloatLatitude{52.52333677944541}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.411538600921629}, FloatLatitude{52.52341511338546}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.411903381347656}, FloatLatitude{52.52374150329884}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.412246704101562}, FloatLatitude{52.523950391570665}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.410637378692625}, FloatLatitude{52.52398955801103}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.409242630004881}, FloatLatitude{52.52413316799366}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.407998085021973}, FloatLatitude{52.52448566323317}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.40705394744873}, FloatLatitude{52.52474676899426}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.406410217285156}, FloatLatitude{52.5249948180297}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.406989574432373}, FloatLatitude{52.525686736883024}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.407375812530518}, FloatLatitude{52.52628726139225}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.406217098236084}, FloatLatitude{52.52663973934549}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.405036926269531}, FloatLatitude{52.52696610529863}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.404350280761717}, FloatLatitude{52.52717497823596}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.404221534729004}, FloatLatitude{52.5265222470087}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.40383529663086}, FloatLatitude{52.526039219655445}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.402740955352783}, FloatLatitude{52.526300316181675}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.401474952697754}, FloatLatitude{52.52666584871098}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.400874137878418}, FloatLatitude{52.527370795712564}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.400616645812988}, FloatLatitude{52.52780159108807}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.399865627288817}, FloatLatitude{52.52756661231615}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.399114608764648}, FloatLatitude{52.52744912245876}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39802026748657}, FloatLatitude{52.527266359833675}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.398470878601072}, FloatLatitude{52.52648308282661}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.398964405059813}, FloatLatitude{52.52538647154948}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.398363590240479}, FloatLatitude{52.52542563670941}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39780569076538}, FloatLatitude{52.525347306354654}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.397247791290283}, FloatLatitude{52.525190645226104}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.396217823028564}, FloatLatitude{52.52494259729653}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.395531177520752}, FloatLatitude{52.52452482919627}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39482307434082}, FloatLatitude{52.524472607904364}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39359998703003}, FloatLatitude{52.5246814926995}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.392891883850098}, FloatLatitude{52.52490343170594}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.392398357391357}, FloatLatitude{52.5239765025348}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.391926288604736}, FloatLatitude{52.52310177678706}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39184045791626}, FloatLatitude{52.52222703362077}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39184045791626}, FloatLatitude{52.521169485041774}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39184045791626}, FloatLatitude{52.52039915585348}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.39205503463745}, FloatLatitude{52.519681040207885}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.392269611358643}, FloatLatitude{52.51900208371135}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.392527103424072}, FloatLatitude{52.51812725890996}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.392677307128904}, FloatLatitude{52.51750050804369}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.393385410308838}, FloatLatitude{52.51735687637764}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.394951820373535}, FloatLatitude{52.517474393230245}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.396711349487305}, FloatLatitude{52.51735687637764}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.398127555847168}, FloatLatitude{52.517696368649815}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.399629592895508}, FloatLatitude{52.51773554066627}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.400981426239014}, FloatLatitude{52.51829700239765}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.403105735778809}, FloatLatitude{52.51887151395141}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.40355634689331}, FloatLatitude{52.51966798345114}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.404908180236816}, FloatLatitude{52.52007274110608}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.40555191040039}, FloatLatitude{52.520529721073366}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.407869338989258}, FloatLatitude{52.52144366674759}});
    params.coordinates.push_back(
        FloatCoordinate{FloatLongitude{13.408942222595215}, FloatLatitude{52.52203119321206}});

    TIMER_START(routes);
    auto NUM = 100;
    for (int i = 0; i < NUM; ++i)
    {
        json::Object result;
        const auto rc = osrm.Match(params, result);
        if (rc != Status::Ok || result.values.at("matchings").get<json::Array>().values.size() != 1)
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
