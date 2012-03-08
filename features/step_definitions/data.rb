Given /^the speedprofile "([^"]*)"$/ do |profile|
  read_speedprofile profile
end

Given /^the speedprofile settings$/ do |table|
  table.raw.each do |row|
    speedprofile[ row[0] ] = row[1]
  end
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
    
    if row['name'] == nil
      tags['name'] = nodes
    elsif (row['name'] == '""') || (row['name'] == "''")
      tags['name'] = ''
    elsif row['name'] == ''
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
    relation << { :type => :restriction, :restriction => row['restriction'] }
    from_way = find_way_by_name(row['from'])
    raise "*** unknown way '#{row['from']}'" unless from_way
    to_way = find_way_by_name(row['to'])
    raise "*** unknown way '#{row['to']}'" unless to_way
    relation << OSM::Member.new( 'way', from_way.id, 'from' )
    relation << OSM::Member.new( 'way', to_way.id, 'to' )
    c = row['via']
    unless c.empty?
      raise "*** node invalid name '#{c}', must be single characters" unless c.size == 1
      raise "*** via node cannot use numbered nodes, '#{c}'" unless c.match /[a-z]/
      via_node = find_node_by_name(c)
      raise "*** unknown node '#{row['via']}'" unless via_node
      relation << OSM::Member.new( 'node', via_node.id, 'via' )
    end
    relation.uid = OSM_UID
    osm_db << relation
  end
end

Given /^the defaults$/ do
end

