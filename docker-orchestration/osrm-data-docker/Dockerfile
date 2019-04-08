
# osrm runtime base image
ARG DOCKER_REGISTRY_PREEFIX=ec2d-dockerregistry-01.mypna.com:8083/
FROM ${DOCKER_REGISTRY_PREEFIX}centos7-osrm-soe

# built osrm mapdata files
ARG MAPDATA_PATH
ENV MAPDATA_PATH ${MAPDATA_PATH}

WORKDIR /mapdata
ADD ${MAPDATA_PATH} /mapdata/

# make the folder mountable
VOLUME /mapdata

