require 'net/http'

def request_locate_url path, node
  @query = path

  uri = generate_request_url path
  response = send_request uri, [node]
end

def request_locate node
  request_locate_url "locate?loc=#{node.lat},#{node.lon}", node
end
