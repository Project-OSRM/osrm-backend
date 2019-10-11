#include "updater/updater.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>
#include <set>

BOOST_AUTO_TEST_SUITE(concurrent_node_set_test)

using namespace osrm;


BOOST_AUTO_TEST_CASE(node_set_enable_test)
{
    updater::NodeSetPtr ns = std::make_shared<updater::NodeSet>();
    ns->insert(1);
    ns->insert(2);
    ns->insert(3);

    updater::NodeSetViewerPtr nsv = std::move(ns);
    BOOST_CHECK(ns == nullptr);
    BOOST_CHECK(nsv->size() == 3);
}

BOOST_AUTO_TEST_SUITE_END()
