def profile
  @profile ||= reset_profile
end

def reset_profile
  @profile = nil
  set_profile DEFAULT_SPEEDPROFILE
end

def set_profile profile
  @profile = profile
end

def write_server_ini
  s=<<-EOF
Threads = 1
IP = 0.0.0.0
Port = #{OSRM_PORT}

hsgrData=#{@osm_file}.osrm.hsgr
nodesData=#{@osm_file}.osrm.nodes
edgesData=#{@osm_file}.osrm.edges
ramIndex=#{@osm_file}.osrm.ramIndex
fileIndex=#{@osm_file}.osrm.fileIndex
namesData=#{@osm_file}.osrm.names
timestamp=#{@osm_file}.osrm.timestamp
EOF
  File.open( 'server.ini', 'w') {|f| f.write( s ) }
end

