#!/bin/sh

set -e

echo "upstream api_server { server ${API_PORT_5000_TCP_ADDR}:${API_PORT_5000_TCP_PORT}; }" > /etc/nginx/upstream.conf
nginx -g "daemon off;"
