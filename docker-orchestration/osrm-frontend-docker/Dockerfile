FROM node

RUN apt-get update && apt-get install -y \
  nginx \
  git \
  && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# forward request and error logs to docker log collector
RUN ln -sf /dev/stdout /var/log/nginx/access.log \
	&& ln -sf /dev/stderr /var/log/nginx/error.log

ENV work_dir /app
WORKDIR ${work_dir}

# install OSRM-front
RUN git clone https://github.com/Project-OSRM/osrm-frontend.git .
COPY ./osrm_options.js ./src/leaflet_options.js

RUN npm install
RUN make

# configure nginx
COPY ./nginx.conf /etc/nginx/sites-enabled/default

EXPOSE 80

COPY ./start.sh ./start.sh
CMD ./start.sh
