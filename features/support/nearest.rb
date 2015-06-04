require 'net/http'

def request_nearest_url path, method={}
  @query = path
  
  if method.has_key?("post")
    request_method = "POST"
  else
    request_method = "GET"
  end
  if request_method.eql? "GET"
    uri = URI.parse "#{HOST}/#{path}"
  elsif request_method.eql? "POST"
    uri = URI.parse "#{HOST}/nearest"
  end
  Timeout.timeout(OSRM_TIMEOUT) do
    if request_method.eql? "GET"
      Net::HTTP.get_response uri
    elsif request_method.eql? "POST"
      path.slice!(0, 12)
      Net::HTTP.post_form uri, "loc" => path
    end
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

def request_nearest a, method
  request_nearest_url "nearest?loc=#{a}", method
end
