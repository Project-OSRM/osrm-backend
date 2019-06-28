package main

import (
	"context"
	"flag"
	"fmt"
	"strconv"

	"github.com/Telenav/osrm-backend/traffic_updater/go/gen-go/proxy"
	"github.com/apache/thrift/lib/go/thrift"
)

var flags struct {
	port          int
	ip            string
	mappingFile   string
	csvFile       string
	highPrecision bool
}

func init() {
	flag.IntVar(&flags.port, "p", 6666, "traffic proxy listening port")
	flag.StringVar(&flags.ip, "c", "127.0.0.1", "traffic proxy ip address")
	flag.StringVar(&flags.mappingFile, "m", "wayid2nodeids.csv", "OSRM way id to node ids mapping table")
	flag.StringVar(&flags.csvFile, "f", "traffic.csv", "OSRM traffic csv file")
	flag.BoolVar(&flags.highPrecision, "d", false, "use high precision speeds, i.e. decimal")
}

func flows2map(flows []*proxy.Flow, m map[uint64]int) {
	for _, flow := range flows {
		wayid := (uint64)(flow.WayId)
		m[wayid] = int(flow.Speed)
	}
}

func main() {
	flag.Parse()

	var transport thrift.TTransport
	var err error

	// make socket
	targetServer := flags.ip + ":" + strconv.Itoa(flags.port)
	fmt.Println("connect traffic proxy " + targetServer)
	transport, err = thrift.NewTSocket(targetServer)
	if err != nil {
		fmt.Println("Error opening socket:", err)
		return
	}

	// Buffering
	transport, err = thrift.NewTFramedTransportFactoryMaxLength(thrift.NewTTransportFactory(), 1024*1024*1024).GetTransport(transport)
	if err != nil {
		fmt.Println("Error get transport:", err)
		return
	}
	defer transport.Close()
	if err := transport.Open(); err != nil {
		fmt.Println("Error opening transport:", err)
		return
	}

	// protocol encoder&decoder
	protocol := thrift.NewTCompactProtocolFactory().GetProtocol(transport)

	// create proxy client
	client := proxy.NewProxyServiceClient(thrift.NewTStandardClient(protocol, protocol))

	// get flows
	fmt.Println("getting flows")
	var defaultCtx = context.Background()
	flows, err := client.GetAllFlows(defaultCtx)
	if err != nil {
		fmt.Println("get flows failed:", err)
		return
	}
	fmt.Printf("got flows count: %d\n", len(flows))

	wayid2speed := make(map[uint64]int)
	flows2map(flows, wayid2speed)

	generateSpeedTable(wayid2speed, flags.mappingFile, flags.csvFile)
}
