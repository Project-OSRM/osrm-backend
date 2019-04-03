
# osrm runtime base image
ARG DOCKER_REGISTRY_PREEFIX=ec2d-dockerregistry-01.mypna.com:8083/
FROM ${DOCKER_REGISTRY_PREEFIX}centos7-osrm-soe

# built osrm binaries package, e.g. "http://{file_server}/osrm-artifacts.tar.gz"
ARG ARTIFACTS_PACKAGE

WORKDIR /osrm-bin
RUN cd /osrm-bin && \
    wget --progress=dot:mega ${ARTIFACTS_PACKAGE} -O osrm-artifacts.tar.gz --no-check-certificate && \
    tar -zxf osrm-artifacts.tar.gz

EXPOSE 5000

ENTRYPOINT ["/osrm-bin/osrm-routed"]

