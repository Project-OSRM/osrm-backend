#$:.unshift(File.dirname(__FILE__) + '/../../lib')
require 'cucumber/rake/task'

Cucumber::Rake::Task.new do |t|
  t.cucumber_opts = %w{--format pretty}
end

osm_data_country = 'denmark'
osm_data_area_name = 'kbh'
osm_data_area_bbox = 'top=55.6972 left=12.5222 right=12.624 bottom=55.6376'


desc "Rebuild and run tests"
task :default => [:build, :cucumber]

desc "Build using Scons"
task :build do
  system "scons"
end

namespace :data do
  desc "Download OSM data extract for Denmark, and crop test areas"
  task :download do
    raise "Error while downloading data." unless system "curl http://download.geofabrik.de/osm/europe/#{osm_data_country}.osm.pbf -o test/data/#{osm_data_country}.osm.pbf"
    raise "Error while cropping data." unless system "osmosis --read-pbf file=test/data/#{osm_data_country}.osm.pbf --bounding-box #{osm_data_area_bbox} --write-pbf file=test/data/#{osm_data_area_name}.osm.pbf omitmetadata=true"
  end
  
  desc "Reprocess OSM data"
  task :process do
    Dir.chdir "test" do   #we must be in the test_folder folder to use the speedprofile.ini in that folder
      raise "Error while extracting data." unless system "./osrm-extract data/#{osm_data_area_name}.osm.pbf"
      raise "Error while preparing data." unless system "./osrm-prepare data/#{osm_data_area_name}.osrm #{osm_data_area_name}.osrm.restrictions"
    end
  end

  desc "Delete preprocessing files"
  task :clean do
    File.delete *Dir.glob('test/data/*.osrm')
    File.delete *Dir.glob('test/data/*.osrm.*')
  end
end

desc "Launch the routing server"
task :run do
  Dir.chdir "test" do
    system "./osrm-routed"
  end
end

desc "Run all test"
task :test do
  puts "Test would go here..."
end
