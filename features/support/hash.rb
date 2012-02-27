require 'digest/sha1'

def hash_of_file path
  hash = Digest::SHA1.new
  open(path,'r') do |io|
    while !io.eof
      buf = io.readpartial 1024
      hash.update buf
    end
  end
  return hash.hexdigest
end

def speedprofile_hash
  @speedprofile_hash ||= Digest::SHA1.hexdigest speedprofile_str
end

def osm_hash
  @osm_hash ||= Digest::SHA1.hexdigest osm_str
end

def osm_hash
  @osm_hash ||= Digest::SHA1.hexdigest osm_str
end

def bin_extract_hash
  @bin_hash ||= hash_of_file '../osrm-extract'
end

def bin_prepare_hash
  @bin_hash ||= hash_of_file '../osrm-prepare'
end

def bin_routed_hash
  @bin_hash ||= hash_of_file '../osrm-routed'
end

#combine state of data, speedprofile and binaries into a hash that identifies the exact test scenario
def fingerprint
  @fingerprint ||= Digest::SHA1.hexdigest "#{bin_extract_hash}-#{bin_prepare_hash}-#{bin_routed_hash}-#{speedprofile_hash}-#{osm_hash}"
end

