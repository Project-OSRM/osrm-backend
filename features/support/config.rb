def speedprofile
  @speedprofile ||= reset_speedprofile
end

def reset_speedprofile
  @speedprofile = nil
  read_speedprofile DEFAULT_SPEEDPROFILE
end

def read_speedprofile profile
  @speedprofile = profile
end

def write_server_ini
  s=<<-EOF
Threads = 1
IP = 0.0.0.0
Port = 5000

hsgrData=#{@osm_file}.osrm.hsgr
nodesData=#{@osm_file}.osrm.nodes
edgesData=#{@osm_file}.osrm.edges
ramIndex=#{@osm_file}.osrm.ramIndex
fileIndex=#{@osm_file}.osrm.fileIndex
namesData=#{@osm_file}.osrm.names
EOF
  File.open( 'server.ini', 'w') {|f| f.write( s ) }
end

