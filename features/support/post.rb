require 'net/http'

HOST = "http://127.0.0.1:#{OSRM_PORT}"

def request_post_url service, param_string
  uri = URI.parse"#{HOST}/#{service}"
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    params = {}
    values = param_string.split("loc=")
    locs = []
    values.each do |value|
      locs << "#{value}".gsub(/[&]/, '')
    end
    locs.reject! { |c| c.empty? }
    params.merge!(loc: locs)
    Net::HTTP.post_form uri, params
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end
