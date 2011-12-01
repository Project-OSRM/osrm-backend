
build_dir = "build"
compiler = 'g++'    #build osrm with the default gcc 4.2.
opt = ''            #don't use OpenMP    
proto = 'DataStructures/pbf-proto'
formats = "-L#{proto} -losmformat.pb.o -lfileformat.pb.o"

#assume libs are installed with homebrew
#call out to the brew binary to find folders with latest version
stxxl_prefix = `brew --prefix libstxxl`.strip
boost_prefix = `brew --prefix boost`.strip
stxxl = "-I#{stxxl_prefix}/include -L#{stxxl_prefix}/lib -lstxxl "
boost = "-L#{boost_prefix}/lib -lboost_system-mt -lboost_thread-mt -lboost_regex-mt -lboost_iostreams-mt "

xml2 = "-I/usr/include/libxml2 -lxml2 "
bz = "-lbz2 -lz "
pbf = "-lprotobuf "


task :default => 'compile'

desc "Compile OSRM"
task :compile => [build_dir, "protobuffer", "binaries"] do
  puts "Done."
end

directory build_dir


#compile protobuffer files
task "protobuffer" => ["#{proto}/fileformat.pb.o", "#{proto}/osmformat.pb.o" ]

file "#{proto}/fileformat.pb.o" => ["#{proto}/fileformat.pb.cc"] do |t|
  sh "#{compiler} -c #{proto}/fileformat.pb.cc -o #{proto}/fileformat.pb.o #{opt}"
end

file "#{proto}/osmformat.pb.o" => ["#{proto}/osmformat.pb.cc"] do |t|
  sh "#{compiler} -c #{proto}/osmformat.pb.cc -o #{proto}/osmformat.pb.o #{opt}"
end

file "#{proto}/fileformat.pb.cc" => ["#{proto}/fileformat.proto"] do |t|
  sh "protoc #{proto}/fileformat.proto -I=#{proto} --cpp_out=#{proto}"
end

file "#{proto}/osmformat.pb.cc" => ["#{proto}/osmformat.proto"] do |t|
  sh "protoc #{proto}/osmformat.proto -I=#{proto} --cpp_out=#{proto}"
end

#compile binaries
task "binaries" => ["#{build_dir}/osrm-extract", "#{build_dir}/osrm-prepare", "#{build_dir}/osrm-routed" ]

def cpp_files prerequisites
  prerequisites.select { |f| f =~ /.cpp$/ }
end

task :headers => FileList['typedefs.h', 'DataStructures/*.h', 'Contractor/*.h', 'Util/*.h']

file "#{build_dir}/osrm-extract" => :headers
file "#{build_dir}/osrm-extract" => ['extractor.cpp'] do |t|
  sh "#{compiler} -o #{t.name} #{cpp_files(t.prerequisites).join ' '} #{stxxl+xml2+boost+bz+pbf+formats} #{opt}"
end

file "#{build_dir}/osrm-prepare" => :headers
file "#{build_dir}/osrm-prepare" => ['createHierarchy.cpp', 'Contractor/EdgeBasedGraphFactory.cpp'] do |t|
  sh "#{compiler} -o #{t.name} #{cpp_files(t.prerequisites).join ' '} #{stxxl+xml2+boost} #{opt}"
end

file "#{build_dir}/osrm-routed" => :headers
file "#{build_dir}/osrm-routed" => FileList['Descriptors/*.h', 'Server/*.h', 'Plugins/*.h']
file "#{build_dir}/osrm-routed" => ['routed.cpp', 'Descriptors/DescriptionFactory.cpp'] do |t|
  sh "#{compiler} -o #{t.name} #{cpp_files(t.prerequisites).join ' '} #{stxxl+xml2+boost+bz} #{opt}"
end



