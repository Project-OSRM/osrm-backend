require 'OSM/objects'       #osmlib gem
require 'OSM/Database'
require 'builder'

OSM_USER = 'osrm'
OSM_TIMESTAMP = '2012-01-01T00:00:00Z'
OSM_GENERATOR = 'osrm-test'
OSM_UID = 1
TEST_FOLDER = 'test'
DATA_FOLDER = 'data'
OSM_FILE = 'test'
LOG_FILE = 'test.log'

DEFAULT_SPEEDPROFILE = 'bicycle'

ORIGIN = [1,1]
ZOOM = 0.001

def must_reprocess
  @must_reprocess = true
end

def must_reprocess?
  @must_reprocess ||= true
end

def reprocess_if_needed
  if must_reprocess?
    raise "*** osrm-routed is running. Please stop it before running tests." if running?
    #puts "Reprocessing: #{@osm_db.nodes.size} nodes, #{@name_way_hash.size} ways, #{@relations.size} relations... "
    
    Dir.chdir TEST_FOLDER do
      write_speedprofile
      write_osm
      reprocess
    end
    @must_reprocess = false
  end
end

def reprocess
  file = "#{DATA_FOLDER}/#{OSM_FILE}"
  raise "*** osrm-extract failed. Please see test.log for more info." unless system "./osrm-extract #{file}.osm.pbf 1>>#{LOG_FILE} 2>>#{LOG_FILE}"
  raise "*** osrm-prepare failed. Please see test.log for more info." unless system "./osrm-prepare #{file}.osrm #{file}.restrictions 1>>#{LOG_FILE} 2>>#{LOG_FILE}"
end

def find_node_by_name s
  name_node_hash[s.to_s]
end

def find_way_by_name s
  name_way_hash[s.to_s] || name_way_hash[s.to_s.reverse]
end

def reset_data
  Dir.chdir "#{TEST_FOLDER}" do
    clear_log
    clear_data_files
  end
  osm_db.clear
  name_node_hash.clear
  name_way_hash.clear
  must_reprocess
  reset_speedprofile
end

def clear_data_files
  File.delete *Dir.glob("#{DATA_FOLDER}/test.*")
end

def clear_log
  File.delete *Dir.glob("*.log")
end

def speedprofile
  @speedprofile ||= reset_speedprofile
end

def reset_speedprofile
  @speedprofile = {}
  read_speedprofile DEFAULT_SPEEDPROFILE
end

def read_speedprofile profile
  @speedprofile = {}
  s = File.read "test/speedprofiles/#{profile}.ini"
  s.scan /(.*)=(.*)/ do |option|
    @speedprofile[option[0].strip] = option[1].strip
  end
end

def dump_speedprofile
  "[default]\n" + @speedprofile.map { |k,v| "\t#{k} = #{v}" }.join("\n")
end

def write_speedprofile
  File.open( 'speedprofile.ini', 'w') {|f| f.write( dump_speedprofile ) }
end

def osm_db
  @osm_db ||= OSM::Database.new
end

def name_node_hash
  @name_node_hash ||= {}
end

def name_way_hash
  @name_way_hash ||= {}
end

def write_osm
  xml = ''
  doc = Builder::XmlMarkup.new :indent => 2, :target => xml
  doc.instruct!
  osm_db.to_xml doc, OSM_GENERATOR
    
  #write .oms file
  file = "#{DATA_FOLDER}/#{OSM_FILE}"
  File.open( "#{file}.osm", 'w') {|f| f.write(xml) }
  
  #convert from .osm to .osm.pbf, which is the format osrm reads
  #convert. redirect stdout and stderr to a log file avoid output in the cucumber console
  unless system "osmosis --read-xml #{file}.osm --write-pbf #{file}.osm.pbf omitmetadata=true 1>>#{LOG_FILE} 2>>#{LOG_FILE}"
    raise "Failed to convert to proto buffer format. Please see #{file}.log for more info." 
  end
end


Given /^the speedprofile "([^"]*)"$/ do |profile|
  read_speedprofile profile
end


Given /^the speedprofile settings$/ do |table|
  table.raw.each do |row|
    speedprofile[ row[0] ] = row[1]
  end
end

Given /^the nodes$/ do |table|
  table.raw.each_with_index do |row,ri|
    row.each_with_index do |name,ci|
      unless name.empty?
        node = OSM::Node.new nil, OSM_USER, OSM_TIMESTAMP, ORIGIN[0]+ci*ZOOM, ORIGIN[1]-ri*ZOOM 
        node << { :name => name }
        node.uid = OSM_UID
        osm_db << node
        name_node_hash[name] = node
      end
    end
  end
end

Given /^the ways$/ do |table|
  must_reprocess
  table.hashes.each do |row|
    name = row.delete 'nodes'
    way = OSM::Way.new nil, OSM_USER, OSM_TIMESTAMP
    defaults = { 'highway' => 'primary' }
    way << defaults.merge( 'name' => name ).merge(row)
    way.uid = OSM_UID
    name.each_char { |c| way << find_node_by_name(c) }
    osm_db << way
    name_way_hash[name] = way
  end
end

Given /^the relations$/ do |table|
  must_reprocess
  table.hashes.each do |row|
    relation = OSM::Relation.new nil, OSM_USER, OSM_TIMESTAMP
    relation << { :type => :restriction, :restriction => 'no_left_turn' }
    relation << OSM::Member.new( 'way', find_way_by_name(row['from']).id, 'from' )
    relation << OSM::Member.new( 'way', find_way_by_name(row['to']).id, 'to' )
    relation << OSM::Member.new( 'node', find_node_by_name(row['via']).id, 'via' )
    relation.uid = OSM_UID
    osm_db << relation
  end
end

Given /^the defaults$/ do
end
