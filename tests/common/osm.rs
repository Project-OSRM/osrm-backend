// TODO: consider trait for OSM entities
// TODO: better error handling in XML creation
// FIXME: today nodes are stored twice
// TODO: move node lookup by name to here
use std::collections::HashMap;

use xml_builder::{XMLBuilder, XMLElement, XMLVersion};

static OSM_USER: &str = "osrm";
static OSM_TIMESTAMP: &str = "2000-01-01T00:00:00Z";
static OSM_UID: &str = "1";

#[derive(Clone, Debug, Default)]
pub struct OSMNode {
    pub id: u64,
    pub lat: f64,
    pub lon: f64,
    pub tags: HashMap<String, String>,
}

impl OSMNode {
    // pub fn add_tag(&mut self, key: &str, value: &str) {
    //     self.tags.insert(key.into(), value.into());
    // }

    // pub fn set_id_(&mut self, id: u64) {
    //     self.id = id;
    // }

    // pub fn set_tags(&mut self, tags: HashMap<String, String>) {
    //     self.tags = tags
    // }

    pub fn to_xml(&self) -> XMLElement {
        let mut node = XMLElement::new("node");
        node.add_attribute("id", &self.id.to_string());
        node.add_attribute("version", "1");
        node.add_attribute("uid", OSM_UID);
        node.add_attribute("user", OSM_USER);
        node.add_attribute("timestamp", OSM_TIMESTAMP);
        node.add_attribute("lon", &format!("{:?}", self.lon));
        node.add_attribute("lat", &format!("{:?}", self.lat));

        if !self.tags.is_empty() {
            for (key, value) in &self.tags {
                let mut tags = XMLElement::new("tag");
                tags.add_attribute("k", key);
                tags.add_attribute("v", value);
                node.add_child(tags).unwrap();
            }
        }

        node
    }
}

#[derive(Clone, Debug, Default)]
pub struct OSMWay {
    pub id: u64,
    pub tags: HashMap<String, String>,
    pub nodes: Vec<OSMNode>,
    pub add_locations: bool,
}

impl OSMWay {
    pub fn add_node(&mut self, node: OSMNode) {
        self.nodes.push(node);
    }

    // pub fn set_tags(&mut self, tags: HashMap<String, String>) {
    //     self.tags = tags;
    // }

    pub fn to_xml(&self) -> XMLElement {
        let mut way = XMLElement::new("way");
        way.add_attribute("id", &self.id.to_string());
        way.add_attribute("version", "1");
        way.add_attribute("uid", OSM_UID);
        way.add_attribute("user", OSM_USER);
        way.add_attribute("timestamp", OSM_TIMESTAMP);

        assert!(self.nodes.len() >= 2);

        for node in &self.nodes {
            let mut nd = XMLElement::new("nd");
            nd.add_attribute("ref", &node.id.to_string());
            if self.add_locations {
                nd.add_attribute("lon", &format!("{:?}", node.lon));
                nd.add_attribute("lat", &format!("{:?}", node.lat));
            }
            way.add_child(nd).unwrap();
        }

        if !self.tags.is_empty() {
            for (key, value) in &self.tags {
                let mut tags = XMLElement::new("tag");
                tags.add_attribute("k", key);
                tags.add_attribute("v", value);
                way.add_child(tags).unwrap();
            }
        }

        way
    }
}

// #[derive(Clone, Debug)]
// struct Member {
//     id: u64,
//     member_type: String,
//     member_role: String,
// }

// #[derive(Clone, Debug)]
// struct OSMRelation {
//     id: u64,
//     osm_user: String,
//     osm_time_stamp: String,
//     osm_uid: String,
//     members: Vec<Member>,
//     tags: HashMap<String, String>,
// }

// impl OSMRelation {
//     fn add_member(&mut self, member_type: String, id: u64, member_role: String) {
//         self.members.push(Member {
//             id,
//             member_type,
//             member_role,
//         });
//     }

//     pub fn add_tag(&mut self, key: &str, value: &str) {
//         self.tags.insert(key.into(), value.into());
//     }
// }

#[derive(Debug, Default)]
pub struct OSMDb {
    nodes: Vec<(char, OSMNode)>,
    ways: Vec<OSMWay>,
    // relations: Vec<OSMRelation>,
}

impl OSMDb {
    pub fn add_node(&mut self, node: OSMNode) {
        let name = node.tags.get("name").unwrap();
        assert!(
            name.len() == 1,
            "name needs to be of length 1, but was \"{name}\""
        );
        self.nodes.push((name.chars().next().unwrap(), node));
    }

    pub fn find_node(&self, search_name: char) -> Option<&(char, OSMNode)> {
        // TODO: this is a linear search.
        self.nodes.iter().find(|(name, _node)| search_name == *name)
    }

    pub fn add_way(&mut self, way: OSMWay) {
        self.ways.push(way);
    }

    // pub fn add_relation(&mut self, relation: OSMRelation) {
    //     self.relations.push(relation);
    // }

    // pub fn clear(&mut self) {
    //     self.nodes.clear();
    //     self.ways.clear();
    //     // self.relations.clear();
    // }

    pub fn to_xml(&self) -> String {
        let mut xml = XMLBuilder::new()
            .version(XMLVersion::XML1_0)
            .encoding("UTF-8".into())
            .build();

        let mut osm = XMLElement::new("osm");
        osm.add_attribute("generator", "osrm-test");
        osm.add_attribute("version", "0.6");

        for (_, node) in &self.nodes {
            osm.add_child(node.to_xml()).unwrap();
        }

        for way in &self.ways {
            osm.add_child(way.to_xml()).unwrap();
        }

        xml.set_root_element(osm);

        let mut writer: Vec<u8> = Vec::new();
        xml.generate(&mut writer).unwrap();
        String::from_utf8(writer).unwrap()
    }

    // pub fn node_len(&self) -> usize {
    //     self.nodes.len()
    // }
    // pub fn way_len(&self) -> usize {
    //     self.ways.len()
    // }
    // pub fn relation_len(&self) -> usize {
    //     self.relations.len()
    // }
}

#[cfg(test)]
mod tests {

    #[test]
    fn empty_db_by_default() {
        use super::*;
        let osm_db = OSMDb::default();
        assert_eq!(0, osm_db.node_len());
        assert_eq!(0, osm_db.way_len());
        // assert_eq!(0, osm_db.relation_len());
    }

    #[test]
    fn osm_db_with_single_node() {
        use super::*;
        let mut osm_db = OSMDb::default();

        let mut node1 = OSMNode {
            id: 123,
            lat: 50.1234,
            lon: 8.9876,
            ..Default::default()
        };

        let mut node2 = OSMNode {
            id: 321,
            lat: 50.1234,
            lon: 8.9876,
            ..Default::default()
        };
        node1.add_tag("name", "a");
        node2.add_tag("name", "b");
        osm_db.add_node(node1.clone());
        osm_db.add_node(node2.clone());

        let mut way = OSMWay {
            id: 890,
            ..Default::default()
        };
        way.nodes.push(node1);
        way.nodes.push(node2);

        osm_db.add_way(way);

        let actual = osm_db.to_xml();
        let expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm generator=\"osrm-test\" version=\"0.6\">\n\t<node id=\"123\" version=\"1.0\" user=\"osrm\" timestamp=\"2000-01-01T00:00:00Z\" lon=\"8.9876\" lat=\"50.1234\">\n\t\t<tag name=\"a\" />\n\t</node>\n\t<node id=\"321\" version=\"1.0\" user=\"osrm\" timestamp=\"2000-01-01T00:00:00Z\" lon=\"8.9876\" lat=\"50.1234\">\n\t\t<tag name=\"b\" />\n\t</node>\n\t<way id=\"890\" version=\"1\" uid=\"1\" user=\"osrm\" timestamp=\"2000-01-01T00:00:00Z\">\n\t\t<nd ref=\"123\" />\n\t\t<nd ref=\"321\" />\n\t</way>\n</osm>\n";

        println!("{actual}");
        assert_eq!(actual, expected);
    }
}
