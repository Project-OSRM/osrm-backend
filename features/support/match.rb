require 'net/http'

HOST = "http://127.0.0.1:#{OSRM_PORT}"

def request_matching trace=[], timestamps=[], options={}
  defaults = { 'output' => 'json' }
  locs = trace.compact.map { |w| "loc=#{w.lat},#{w.lon}" }
  ts = timestamps.compact.map { |t| "t=#{t}" }
  if ts.length > 0
    trace_params = locs.zip(ts).map { |a| a.join('&')}
  else
    trace_params = locs
  end
  params = (trace_params + defaults.merge(options).to_param).join('&')
  params = nil if params==""
  
  if options.has_key?("post")
    request_method = "POST"
    options.delete("post")
  else
    request_method = "GET"
  end
  if request_method.eql? "GET"
    uri = URI.parse ["#{HOST}/match", params].compact.join('?')
  elsif request_method.eql? "POST"
    uri = URI.parse "#{HOST}/match"
  end
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    if request_method.eql? "GET"
      Net::HTTP.get_response uri
    elsif request_method.eql? "POST"
      datas = {}
      datas[:loc] = trace.compact.map { |w| "#{w.lat},#{w.lon}" }
      if ts.length > 0
        datas[:t] = timestamps.compact.map { |t| "#{t}" }
      end
      datas.merge! options
      Net::HTTP.post_form uri, datas
    end
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

