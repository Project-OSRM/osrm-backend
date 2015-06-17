require 'net/http'

def generate_request_url path
  if @http_method.eql? "POST"
    pos = path.index('?') - 1
    service = path[0..pos]
    uri = URI.parse "#{HOST}/#{service}"
  else
    uri = URI.parse "#{HOST}/#{path}"
  end
end

def send_request uri, waypoints=[], options={}, timestamps=[]
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    if @http_method.eql? "POST"
      datas = {}
      if waypoints.length > 0
        datas[:loc] = waypoints.compact.map { |w| "#{w.lat},#{w.lon}" }
      end
      if timestamps.length > 0
        datas[:t] = timestamps.compact.map { |t| "#{t}" }
      end
      datas.merge! options
      response = Net::HTTP.post_form uri, datas
    else
      response = Net::HTTP.get_response uri
    end
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end
