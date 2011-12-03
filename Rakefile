testdata_folder = "testdata"   #where to locate test data
sandbox = "sandbox"   #where to locate builds, server configs and test data
area_name = "amager"   #name of OSM data file

desc "Rebuild,  reprocess OSM data and run server"
task :default => [:build, :process, :run]

desc "Build using Scons"
task :build do
  system "scons"
end


file "#{sandbox}/#{area_name}.osm.pbf" => "#{testdata_folder}/#{area_name}.osm.pbf" do |t|
  raise unless system "cp #{t.prerequisites.join} #{t.name}"
end

desc "Process OSM test data"
task :process => ["#{sandbox}/#{area_name}.osm.pbf", :setup] do
  prev = Dir.pwd
  cd sandbox    #we must be in the sandbox folder to use the speedprofile.ini in that folder
  raise "Error while extracting data." unless system "./osrm-extract #{area_name}.osm.pbf"
  raise "Error while preparing data." unless system "./osrm-prepare #{area_name}.osrm #{area_name}.osrm.restrictions"
  cd prev
end

desc "Download fresh OSM for the test data"
task :download => :setup do
  start = Time.now
  country = 'denmark'
  bbox = 'top=55.6655 left=12.5589 bottom=55.6462 right=12.5963'
  area = area_name

  raise "Error while downloading data." unless system "curl http://download.geofabrik.de/osm/europe/#{country}.osm.pbf -o #{sandbox}/#{country}.osm.pbf"
  raise "Error while cropping data." unless system "osmosis --read-pbf file=#{sandbox}/#{country}.osm.pbf --bounding-box #{bbox} --write-pbf file=#{sandbox}/#{area}.osm.pbf omitmetadata=true"
end

desc "Setup server files"
task :setup => ["#{sandbox}/speedprofile.ini", "#{sandbox}/extractor.ini", "#{sandbox}/server.ini"]

file "#{sandbox}/speedprofile.ini" => "speedprofile.ini" do |t|
  system "cp #{t.prerequisites.join} #{t.name}"
end

file "#{sandbox}/extractor.ini" => "extractor.ini" do |t|
  system "cp #{t.prerequisites.join} #{t.name}"
end

file "#{sandbox}/server.ini" => "server.ini" do |t|
  #first time the file is copied, we adjusts server settings to point to data files in our sandbox folder
  text = File.read(t.prerequisites.join)
  text.gsub!('/opt/osm/germany', "#{Dir.pwd}/sandbox/#{area_name}")
  file = File.new( t.name, "w+")
  file.puts text
  file.close
end

desc "Run the OSRM server"
task :run => :setup do
  cd sandbox
  system "osrm-routed"
end

desc "Run all test"
task :test do
  puts "Test would go here..."
end