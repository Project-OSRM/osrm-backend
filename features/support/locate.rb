require 'net/http'

def request_locate_url path
  @query = path
  
  uri = generate_request_url path
  response = send_simple_request uri, path
end

def request_locate a
  request_locate_url "locate?loc=#{a}"
end
