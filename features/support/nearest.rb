require 'net/http'

def request_nearest_url path, node
  @query = path
  
  uri = generate_request_url path
  response = send_request uri, [node]
end

def request_nearest node
  request_nearest_url "nearest?loc=#{node.lat},#{node.lon}", node
end
