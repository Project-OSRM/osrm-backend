use super::osm::{OSMNode, OSMWay};
use xml_builder::{XMLBuilder, XMLElement, XMLVersion};

// TODO: better error handling in XML creation
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
            location: Location {
                longitude: 8.9876,
                latitude: 50.1234,
            },
            ..Default::default()
        };

        let mut node2 = OSMNode {
            id: 321,
            location: Location {
                longitude: 8.9876,
                latitude: 50.1234,
            },
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

        // println!("{actual}");
        assert_eq!(actual, expected);
    }
}
