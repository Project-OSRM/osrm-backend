

FROM wangyoucao577/osrm-backend-dev as builder

RUN mkdir -p /osrm-build
WORKDIR /workspace/

ARG BRANCH_NAME=master-telenav
RUN echo "Building branch ${BRANCH_NAME}" && \
  git clone -b ${BRANCH_NAME} https://github.com/Telenav/osrm-backend.git
  
RUN mkdir -p osrm-backend/build && \
  cd osrm-backend/build && \
  cmake .. -DENABLE_LTO=On && \
  cmake --build . -- -j && \
  cp /workspace/osrm-backend/build/osrm* /osrm-build/ && \
  cp -r /workspace/osrm-backend/profiles /osrm-build/

RUN cd /workspace/osrm-backend && \
  if [ -d "traffic_updater/go/osrm_traffic_updater" ]; then \
  cd traffic_updater/go/osrm_traffic_updater && \
  go install && \
  cd ../../../ \
  ;fi && \
  if [ -d "traffic_updater/go/wayid2nodeid_extractor" ]; then \
  cd traffic_updater/go/wayid2nodeid_extractor && \
  go install && \
  cd ../../../ \
  ;fi && \
  if [ -d "traffic_updater/go/snappy_command" ]; then \
  cd traffic_updater/go/snappy_command && \
  go install && \
  cd ../../../ \
  ;fi && \
  ls -lh /workspace/go/bin


FROM debian:stretch-slim as runstage

RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
  DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends libboost-program-options1.62.0 libboost-regex1.62.0 \
    libboost-date-time1.62.0 libboost-chrono1.62.0 libboost-filesystem1.62.0 \
    libboost-iostreams1.62.0 libboost-thread1.62.0 expat liblua5.2-0 libtbb2 curl ca-certificates && \
  rm -rf /var/lib/apt/lists/*

RUN mkdir -p /osrm-build /osrm-data /osrm-logs

COPY --from=builder /osrm-build /osrm-build/
COPY --from=builder /workspace/go/bin /osrm-build/
COPY ./docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh

# for shown on runtime
ARG IMAGE_TAG
ENV IMAGE_TAG ${IMAGE_TAG}

EXPOSE 5000
ENTRYPOINT ["/docker-entrypoint.sh"]

