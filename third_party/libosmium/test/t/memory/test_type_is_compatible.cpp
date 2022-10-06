#include "catch.hpp"

#include <osmium/memory/item_iterator.hpp>
#include <osmium/osm.hpp>

static_assert(osmium::memory::detail::type_is_compatible<osmium::Node>(osmium::item_type::node), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Way>(osmium::item_type::node), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Relation>(osmium::item_type::node), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Area>(osmium::item_type::node), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMObject>(osmium::item_type::node), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Changeset>(osmium::item_type::node), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMEntity>(osmium::item_type::node), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::memory::Item>(osmium::item_type::node), "");

static_assert(!osmium::memory::detail::type_is_compatible<osmium::Node>(osmium::item_type::way), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::Way>(osmium::item_type::way), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Relation>(osmium::item_type::way), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Area>(osmium::item_type::way), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMObject>(osmium::item_type::way), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Changeset>(osmium::item_type::way), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMEntity>(osmium::item_type::way), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::memory::Item>(osmium::item_type::way), "");

static_assert(!osmium::memory::detail::type_is_compatible<osmium::Node>(osmium::item_type::relation), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Way>(osmium::item_type::relation), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::Relation>(osmium::item_type::relation), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Area>(osmium::item_type::relation), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMObject>(osmium::item_type::relation), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Changeset>(osmium::item_type::relation), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMEntity>(osmium::item_type::relation), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::memory::Item>(osmium::item_type::relation), "");

static_assert(!osmium::memory::detail::type_is_compatible<osmium::Node>(osmium::item_type::area), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Way>(osmium::item_type::area), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Relation>(osmium::item_type::area), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::Area>(osmium::item_type::area), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMObject>(osmium::item_type::area), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Changeset>(osmium::item_type::area), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMEntity>(osmium::item_type::area), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::memory::Item>(osmium::item_type::area), "");

static_assert(!osmium::memory::detail::type_is_compatible<osmium::Node>(osmium::item_type::changeset), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Way>(osmium::item_type::changeset), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Relation>(osmium::item_type::changeset), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::Area>(osmium::item_type::changeset), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::OSMObject>(osmium::item_type::changeset), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::Changeset>(osmium::item_type::changeset), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::OSMEntity>(osmium::item_type::changeset), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::memory::Item>(osmium::item_type::changeset), "");

static_assert(osmium::memory::detail::type_is_compatible<osmium::OuterRing>(osmium::item_type::outer_ring), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::InnerRing>(osmium::item_type::inner_ring), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::WayNodeList>(osmium::item_type::way_node_list), "");

static_assert(!osmium::memory::detail::type_is_compatible<osmium::OuterRing>(osmium::item_type::inner_ring), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::InnerRing>(osmium::item_type::outer_ring), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::WayNodeList>(osmium::item_type::outer_ring), "");

static_assert(osmium::memory::detail::type_is_compatible<osmium::TagList>(osmium::item_type::tag_list), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::InnerRing>(osmium::item_type::inner_ring), "");

static_assert(!osmium::memory::detail::type_is_compatible<osmium::TagList>(osmium::item_type::inner_ring), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::InnerRing>(osmium::item_type::tag_list), "");

static_assert(osmium::memory::detail::type_is_compatible<osmium::RelationMemberList>(osmium::item_type::relation_member_list), "");
static_assert(osmium::memory::detail::type_is_compatible<osmium::RelationMemberList>(osmium::item_type::relation_member_list_with_full_members), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::RelationMemberList>(osmium::item_type::tag_list), "");

static_assert(osmium::memory::detail::type_is_compatible<osmium::ChangesetDiscussion>(osmium::item_type::changeset_discussion), "");
static_assert(!osmium::memory::detail::type_is_compatible<osmium::ChangesetDiscussion>(osmium::item_type::relation_member_list), "");

