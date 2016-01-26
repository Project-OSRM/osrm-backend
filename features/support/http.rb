require 'net/http'

# Converts an array [["param","val1"], ["param","val2"]] into param=val1&param=val2
def params_to_string params
  kv_pairs = params.map { |kv| kv[0].to_s + "=" + kv[1].to_s }
  url = kv_pairs.size > 0 ? kv_pairs.join("&") : ""
  return url
end

def send_request base_uri, parameters
  Timeout.timeout(OSRM_TIMEOUT) do
    uri_string = base_uri
    params = params_to_string(parameters)
    if not params.eql? ""
      uri_string = uri_string + "?" + params
    end
    uri = URI.parse(uri_string)
    @query = uri.to_s
    if @http_method.eql? "POST"
      Net::HTTP.start(uri.hostname, uri.port) do |http|
        req = Net::HTTP::Post.new(uri.path)
        req.body = params_to_string parameters
        response = http.request(req)
      end
    else
      response = Net::HTTP.get_response uri
    end
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end
