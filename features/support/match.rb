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
  
  uri = generate_request_url ("match" + '?' + params)
  response = send_request uri, trace, options, timestamps
end

