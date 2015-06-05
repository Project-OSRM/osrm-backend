require 'net/http'

def request_nearest_url path
  @query = path
  
  uri = generate_request_url path
  response = send_simple_request uri, path
end

def request_nearest a
  request_nearest_url "nearest?loc=#{a}"
end
