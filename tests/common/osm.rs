use std::collections::HashMap;

use xml_builder::XMLElement;

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
