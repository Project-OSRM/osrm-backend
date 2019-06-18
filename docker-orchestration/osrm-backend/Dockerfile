

FROM wangyoucao577/osrm-backend-dev as builder
ARG BRANCH_NAME=master-telenav

RUN echo "Building branch ${BRANCH_NAME}" && \
  git clone -b ${BRANCH_NAME} https://github.com/Telenav/osrm-backend.git && \
  mkdir -p osrm-backend/build && \
  cd osrm-backend/build && \
  cmake .. -DENABLE_LTO=On && \
  cmake --build . -- -j && \
  mkdir /osrm-build && \
  mkdir /osrm-data && \
  cp /osrm-backend/build/osrm* /osrm-build/ && \
  cp -r /osrm-backend/profiles /osrm-build/


FROM wangyoucao577/osrm-backend-dev as gobuilder
ARG BRANCH_NAME=master-telenav

WORKDIR /workspace/go
RUN echo "Building branch ${BRANCH_NAME}" && \
  mkdir -p src/github.com/Telenav && \
  cd src/github.com/Telenav && \
  git clone -b ${BRANCH_NAME} https://github.com/Telenav/osrm-backend.git && \
  cd ../../../ && \
  if [ -d "src/github.com/Telenav/osrm-backend/traffic_updater/go/osrm_traffic_updater" ]; then \
  go install github.com/Telenav/osrm-backend/traffic_updater/go/gen-go/proxy && \
  go install github.com/Telenav/osrm-backend/traffic_updater/go/osrm_traffic_updater && \
  ls -lh bin/ \
  ;fi


FROM debian:stretch-slim as runstage

RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
  DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends libboost-program-options1.62.0 libboost-regex1.62.0 \
    libboost-date-time1.62.0 libboost-chrono1.62.0 libboost-filesystem1.62.0 \
    libboost-iostreams1.62.0 libboost-thread1.62.0 expat liblua5.2-0 libtbb2 curl ca-certificates && \
  rm -rf /var/lib/apt/lists/*

RUN mkdir /osrm-build

COPY --from=builder /osrm-build /osrm-build/
COPY --from=builder /osrm-data /osrm-data/
COPY --from=gobuilder /workspace/go/bin /osrm-build/

WORKDIR /osrm-build

# Publish
COPY ./docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh
ENTRYPOINT ["/docker-entrypoint.sh"]
