require 'OSM/objects'       #osmlib gem
require 'OSM/Database'
require 'builder'

OSM_USER = 'osrm'
OSM_GENERATOR = 'osrm-test'
OSM_UID = 1
TEST_FOLDER = 'test'
DATA_FOLDER = 'cache'
PREPROCESS_LOG_FILE = 'preprocessing.log'
LOG_FILE = 'fail.log'
OSM_TIMESTAMP = '2000-00-00T00:00:00Z'
DEFAULT_SPEEDPROFILE = 'bicycle'
WAY_SPACING = 10
DEFAULT_GRID_SIZE = 100   #meters

ORIGIN = [1,1]

def set_grid_size meters    
  @zoom = 0.001*(meters.to_f/111.21)
end

def build_ways_from_table table
  #add one unconnected way for each row
  table.hashes.each_with_index do |row,ri|
    #NOTE:
    #currently osrm crashes when processing an isolated oneway with just 2 nodes, so we use 4
    #this is relatated to the fact that a oneway deadend doesn't make a lot of sense
    
    #if we stack ways on different x coordinates, outability tests get messed up, because osrm might pick a neighboring way if the one test can't be used.
    #instead we place all lines as a string on the same y coordinate. this prevents using neightboring ways.
    
    #a few nodes...
    node1 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, ORIGIN[0]+(0+WAY_SPACING*ri)*@zoom, ORIGIN[1] 
    node2 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, ORIGIN[0]+(1+WAY_SPACING*ri)*@zoom, ORIGIN[1] 
    node3 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, ORIGIN[0]+(2+WAY_SPACING*ri)*@zoom, ORIGIN[1] 
    node4 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, ORIGIN[0]+(3+WAY_SPACING*ri)*@zoom, ORIGIN[1] 
    node1.uid = OSM_UID
    node2.uid = OSM_UID
    node3.uid = OSM_UID
    node4.uid = OSM_UID
    node1 << { :name => "a#{ri}" }
    node2 << { :name => "b#{ri}" }
    node3 << { :name => "c#{ri}" }
    node4 << { :name => "d#{ri}" }

    osm_db << node1
    osm_db << node2
    osm_db << node3
    osm_db << node4
    
    #...with a way between them
    way = OSM::Way.new make_osm_id, OSM_USER, OSM_TIMESTAMP
    way.uid = OSM_UID
    way << node1
    way << node2
    way << node3
    way << node4
    tags = row.dup
    tags.delete 'forw'
    tags.delete 'backw'
    tags['name'] = "w#{ri}"
    tags.reject! { |k,v| v=='' }
    way << tags
    osm_db << way
  end
end

def find_node_by_name s
  name_node_hash[s.to_s]
end

def find_way_by_name s
  name_way_hash[s.to_s] || name_way_hash[s.to_s.reverse]
end

def reset_data
  Dir.chdir TEST_FOLDER do
    #clear_log
    #clear_data_files
  end
  reset_speedprofile
  reset_osm
  @fingerprint = nil
end

def make_osm_id
  @osm_id = @osm_id+1
end

def reset_osm
  osm_db.clear
  name_node_hash.clear
  name_way_hash.clear
  @osm_str = nil
  @osm_hash = nil
  
  ##ID -1 causes trouble, so add a few nodes to avoid it
  #node = OSM::Node.new nil, OSM_USER, OSM_TIMESTAMP, 0,0 
  #node = OSM::Node.new nil, OSM_USER, OSM_TIMESTAMP, 0,0 
  @osm_id = 0
end

def clear_data_files
  File.delete *Dir.glob("#{DATA_FOLDER}/test.*")
end

def clear_log
  File.delete *Dir.glob("*.log")
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

def osm_str
  return @osm_str if @osm_str
  @osm_str = ''
  doc = Builder::XmlMarkup.new :indent => 2, :target => @osm_str
  doc.instruct!
  osm_db.to_xml doc, OSM_GENERATOR
  @osm_str
end

def write_osm  
  #write .oms file if needed
  Dir.mkdir DATA_FOLDER unless File.exist? DATA_FOLDER
  @osm_file = "#{DATA_FOLDER}/#{fingerprint}"
  unless File.exist?("#{@osm_file}.osm")
    File.open( "#{@osm_file}.osm", 'w') {|f| f.write(osm_str) }
  end
end

def convert_osm_to_pbf
  unless File.exist?("#{@osm_file}.osm.pbf")
    log_preprocess_info
    log "== Converting #{@osm_file}.osm to protobuffer format...", :preprocess
    #redirect stdout and stderr to a log file avoid output in the cucumber console
    unless system "osmosis --read-xml #{@osm_file}.osm --write-pbf #{@osm_file}.osm.pbf omitmetadata=true 1>>#{PREPROCESS_LOG_FILE} 2>>#{PREPROCESS_LOG_FILE}"
      raise "Failed to convert to proto buffer format. Please see #{hash}.log for more info." 
    end
    log '', :preprocess
  end
end

def extracted?
  File.exist?("#{@osm_file}.osrm") &&
  File.exist?("#{@osm_file}.osrm.names") &&
  File.exist?("#{@osm_file}.osrm.restrictions")
end

def prepared?
  base = "#{DATA_FOLDER}/#{fingerprint}"
  File.exist?("#{base}.osrm.hsgr")
end

def reprocess
  Dir.chdir TEST_FOLDER do
    write_speedprofile
    write_osm
    convert_osm_to_pbf
    unless extracted?
      log_preprocess_info
      log "== Extracting #{@osm_file}.osm...", :preprocess
      unless system "../osrm-extract #{@osm_file}.osm.pbf 1>>#{PREPROCESS_LOG_FILE} 2>>#{PREPROCESS_LOG_FILE}"
        log "*** Exited with code #{$?.exitstatus}.", :preprocess
        raise "*** osrm-extract exited with code #{$?.exitstatus}. The file preprocess.log might contain more info." 
      end
      log '', :preprocess
    end
    unless prepared?
      log_preprocess_info
      log "== Preparing #{@osm_file}.osm...", :preprocess
      unless system "../osrm-prepare #{@osm_file}.osrm #{@osm_file}.osrm.restrictions 1>>#{PREPROCESS_LOG_FILE} 2>>#{PREPROCESS_LOG_FILE}"
        log "*** Exited with code #{$?.exitstatus}.", :preprocess
        raise "*** osrm-prepare exited with code #{$?.exitstatus}. The file preprocess.log might contain more info." 
      end 
      log '', :preprocess
    end
    log_preprocess_done
    write_server_ini
  end
end

