sandbox = "sandbox"   #where to locate builds, server configs and test data
osm_data = "amager"   #name of OSM data file

desc "Recompile,  reprocess OSM data and run server"
task :default => [:compile, :process, :run]

desc "Compile"
task :compile do
  raise "Error while building." unless system "scons"
end


file "#{sandbox}/amager.osm.pbf" => "amager.osm.pbf" do |t|
  raise unless system "cp #{t.prerequisites.join} #{t.name}"
end

desc "Reprocess OSM test data"
task :process => ["#{sandbox}/amager.osm.pbf", :setup] do
  prev = Dir.pwd
  cd sandbox    #we must be in the sandbox folder to use the speedprofile.ini in that folder
  raise "Error while extracting data." unless system "./osrm-extract amager.osm.pbf"
  raise "Error while preparing data." unless system "./osrm-prepare amager.osrm amager.osrm.restrictions"
  cd prev
end

desc "Setup server files"
task :setup => ["#{sandbox}/speedprofile.ini", "#{sandbox}/extractor.ini", "#{sandbox}/server.ini"]

file "#{sandbox}/speedprofile.ini" => "speedprofile.ini" do |t|
  raise unless system "cp #{t.prerequisites.join} #{t.name}"
end

file "#{sandbox}/extractor.ini" => "extractor.ini" do |t|
  raise unless system "cp #{t.prerequisites.join} #{t.name}"
end

file "#{sandbox}/server.ini" => "server.ini" do |t|
  #first time the file is copied, we adjusts server settings to point to data files in our sandbox folder
  text = File.read(t.prerequisites.join)
  text.gsub!('/opt/osm/germany', "#{Dir.pwd}/sandbox/#{osm_data}")
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