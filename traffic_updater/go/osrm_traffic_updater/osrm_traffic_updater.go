package main

import (
	"bufio"
	"context"
	"flag"
	"fmt"
	"os"
	"strconv"
	"time"

	"github.com/Telenav/osrm-backend/traffic_updater/go/gen-go/proxy"
	"github.com/apache/thrift/lib/go/thrift"
)

var flags struct {
	port          int
	ip            string
	csvFile       string
	highPrecision bool
}

func init() {
	flag.IntVar(&flags.port, "p", 6666, "traffic proxy listening port")
	flag.StringVar(&flags.ip, "c", "127.0.0.1", "traffic proxy ip address")
	flag.StringVar(&flags.csvFile, "f", "traffic.csv", "OSRM traffic csv file")
	flag.BoolVar(&flags.highPrecision, "d", true, "use high precision speeds, i.e. decimal")
}

func dumpFlowsToCsv(csvFile string, flows []*proxy.Flow) {

	if _, err := os.Stat(csvFile); err == nil {
		// csvFile exists, remove it
		rmErr := os.Remove(csvFile)
		if rmErr != nil {
			fmt.Println(rmErr)
			return
		}
	}

	f, err := os.OpenFile(csvFile, os.O_RDWR|os.O_CREATE, 0755)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer f.Close()
	writer := bufio.NewWriter(f)

	for i, flow := range flows {
		var osrmTrafficLine string
		if flags.highPrecision {
			osrmTrafficLine = fmt.Sprintf("%d,%d,%f\n", flow.FromId, flow.ToId, flow.Speed)
		} else {
			osrmTrafficLine = fmt.Sprintf("%d,%d,%d\n", flow.FromId, flow.ToId, int(flow.Speed))
		}

		// print first 10 lines for debug
		if i < 10 {
			fmt.Printf("[ %d ] %v\n", i, flow)
			fmt.Printf("[ %d ] %s\n", i, osrmTrafficLine)
		}

		// write to csv
		_, err := writer.WriteString(osrmTrafficLine)
		if err != nil {
			fmt.Println(err)
			return
		}
	}
	writer.Flush()
	f.Sync()
	fmt.Printf("total wrote to %s count: %d\n", csvFile, len(flows))
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
	startTime := time.Now()
	fmt.Println("getting flows")
	var defaultCtx = context.Background()
	flows, err := client.GetAllFlows(defaultCtx)
	if err != nil {
		fmt.Println("get flows failed:", err)
		return
	}
	fmt.Printf("got flows count: %d\n", len(flows))
	afterGotFlowTime := time.Now()
	fmt.Printf("get flows time used: %f seconds\n", afterGotFlowTime.Sub(startTime).Seconds())

	// dump to csv
	fmt.Println("dump flows to: " + flags.csvFile)
	dumpFlowsToCsv(flags.csvFile, flows)
	endTime := time.Now()
	fmt.Printf("dump csv time used: %f seconds\n", endTime.Sub(afterGotFlowTime).Seconds())

}
