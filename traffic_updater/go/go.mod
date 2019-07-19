module github.com/Telenav/osrm-backend/traffic_updater/go

go 1.12

require (
	github.com/apache/thrift v0.12.0
	github.com/golang/protobuf v1.3.2 // indirect
	github.com/golang/snappy v0.0.1
	github.com/qedus/osmpbf v1.1.0
)

replace github.com/Telenav/osrm-backend/traffic_updater/go/gen-go/proxy => ./gen-go/proxy
