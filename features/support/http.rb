require 'net/http'

# Converts an array [["param","val1"], ["param","val2"]] into ?param=val1&param=val2
def params_to_url params
  kv_pairs = params.map { |kv| kv[0].to_s + "=" + kv[1].to_s }
  url = kv_pairs.size > 0 ? ("?" + kv_pairs.join("&")) : ""
  return url
end

# Converts an array [["param","val1"], ["param","val2"]] into ["param"=>["val1", "val2"]]
def params_to_map params
  result = {}
  params.each do |pair|
    if not result.has_key? pair[0]
      result[pair[0]] = []
    end
    result[pair[0]] << [pair[1]]
  end
end

def send_request base_uri, parameters
  Timeout.timeout(OSRM_TIMEOUT) do
    if @http_method.eql? "POST"
      uri = URI.parse base_uri
      @query = uri.to_s
      response = Net::HTTP.post_form uri, (params_to_map parameters)
    else
      uri = URI.parse base_uri+(params_to_url parameters)
      @query = uri.to_s
      response = Net::HTTP.get_response uri
    end
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end
