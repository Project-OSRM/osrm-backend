#$:.unshift(File.dirname(__FILE__) + '/../../lib')
require 'cucumber/rake/task'

Cucumber::Rake::Task.new do |t|
  t.cucumber_opts = %w{--format pretty}
end

areas = {
  :kbh => { :country => 'denmark', :bbox => 'top=55.6972 left=12.5222 right=12.624 bottom=55.6376' },
  :frd => { :country => 'denmark', :bbox => 'top=55.7007 left=12.4765 bottom=55.6576 right=12.5698' },
  :regh => { :country => 'denmark', :bbox => 'top=56.164 left=11.792 bottom=55.403 right=12.731' },
  :dk => { :country => 'denmark', :bbox => nil },
  :skaane => { :counry => 'sweden', :bbox => 'top=56.55 left=12.4 bottom=55.3 right=14.6' }
}
osm_data_area_name = ENV['area'].to_s.to_sym || :kbh
raise "Unknown data area." unless areas[osm_data_area_name]
osm_data_country = areas[osm_data_area_name][:country]
osm_data_area_bbox = areas[osm_data_area_name][:bbox]


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
    if osm_data_area_bbox
      raise "Error while cropping data." unless system "osmosis --read-pbf file=test/data/#{osm_data_country}.osm.pbf --bounding-box #{osm_data_area_bbox} --write-pbf file=test/data/#{osm_data_area_name}.osm.pbf omitmetadata=true"
    end
  end

  task :crop do
    if osm_data_area_bbox
      raise "Error while cropping data." unless system "osmosis --read-pbf file=test/data/#{osm_data_country}.osm.pbf --bounding-box #{osm_data_area_bbox} --write-pbf file=test/data/#{osm_data_area_name}.osm.pbf omitmetadata=true"
    end
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
