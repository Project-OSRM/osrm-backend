def speedprofile
  @speedprofile ||= reset_speedprofile
end

def reset_speedprofile
  @speedprofile = {}
  read_speedprofile DEFAULT_SPEEDPROFILE
end

def read_speedprofile profile
  @speedprofile = {}
  @speedprofile_str = nil
  s = File.read "test/speedprofiles/#{profile}.ini"
  s.scan /(.*)=(.*)/ do |option|
    @speedprofile[option[0].strip] = option[1].strip
  end
end

def speedprofile_str
  @speedprofile_str ||= "[Scenario: #{@scenario_title}]\n" + @speedprofile.map { |k,v| "    #{k} = #{v}" }.join("\n")
end

def write_speedprofile
  File.open( 'speedprofile.ini', 'w') {|f| f.write( speedprofile_str ) }
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

