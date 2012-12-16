Given /^the profile "([^"]*)"$/ do |profile|
  set_profile profile
end

Given /^a grid size of (\d+) meters$/ do |meters|
  set_grid_size meters
end

Given /^the node map$/ do |table|
  table.raw.each_with_index do |row,ri|
    row.each_with_index do |name,ci|
      unless name.empty?
        raise "*** node invalid name '#{name}', must be single characters" unless name.size == 1
        raise "*** invalid node name '#{name}', must me alphanumeric" unless name.match /[a-z0-9]/
        raise "*** duplicate node '#{name}'" if name_node_hash[name]
        node = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, ORIGIN[0]+ci*@zoom, ORIGIN[1]-ri*@zoom 
        node << { :name => name }
        node.uid = OSM_UID
        osm_db << node
        name_node_hash[name] = node
      end
    end
  end
end

Given /^the node locations$/ do |table|
  table.hashes.each do |row|
    name = row['node']
    raise "*** node invalid name '#{name}', must be single characters" unless name.size == 1
    raise "*** invalid node name '#{name}', must me alphanumeric" unless name.match /[a-z0-9]/
    raise "*** duplicate node '#{name}'" if name_node_hash[name]
    node = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, row['lon'].to_f, row['lat'].to_f 
    node << { :name => name }
    node.uid = OSM_UID
    osm_db << node
    name_node_hash[name] = node
  end
end

Given /^the nodes$/ do |table|
  table.hashes.each do |row|
    name = row.delete 'node'
    raise "***invalid node name '#{c}', must be single characters" unless name.size == 1
    node = find_node_by_name(name)
    raise "*** unknown node '#{c}'" unless node
    node << row
  end
end

Given /^the ways$/ do |table|
  table.hashes.each do |row|
    way = OSM::Way.new make_osm_id, OSM_USER, OSM_TIMESTAMP
    way.uid = OSM_UID
    
    nodes = row.delete 'nodes'
    raise "*** duplicate way '#{nodes}'" if name_way_hash[nodes]
    nodes.each_char do |c|
      raise "***invalid node name '#{c}', must be single characters" unless c.size == 1
      raise "*** ways cannot use numbered nodes, '#{name}'" unless c.match /[a-z]/
      node = find_node_by_name(c)
      raise "*** unknown node '#{c}'" unless node
      way << node
    end
    
    defaults = { 'highway' => 'primary' }
    tags = defaults.merge(row)

    if row['highway'] == '(nil)'
      tags.delete 'highway'
    end
    
    if row['name'] == nil
      tags['name'] = nodes
    elsif (row['name'] == '""') || (row['name'] == "''")
      tags['name'] = ''
    elsif row['name'] == '' || row['name'] == '(nil)'
      tags.delete 'name'
    else
      tags['name'] = row['name']
    end
    
    way << tags
    osm_db << way
    name_way_hash[nodes] = way
  end
end

Given /^the relations$/ do |table|
  table.hashes.each do |row|
    relation = OSM::Relation.new make_osm_id, OSM_USER, OSM_TIMESTAMP
    row.each_pair do |key,value|
      if key =~ /^node:(.*)/
        raise "***invalid relation node member '#{value}', must be single character" unless value.size == 1
        node = find_node_by_name(value)
        raise "*** unknown relation node member '#{value}'" unless node
        relation << OSM::Member.new( 'node', node.id, $1 )
      elsif key =~ /^way:(.*)/
        way = find_way_by_name(value)
        raise "*** unknown relation way member '#{value}'" unless way
        relation << OSM::Member.new( 'way', way.id, $1 )
      elsif key =~ /^(.*):(.*)/
        raise "*** unknown relation member type '#{$1}', must be either 'node' or 'way'"
      else
        relation << { key => value }
      end
    end
    relation.uid = OSM_UID
    osm_db << relation
  end
end

Given /^the defaults$/ do
end

