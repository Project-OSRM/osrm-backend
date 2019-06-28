package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"time"
)

// todo:
//       Write data into more compressed format(parquet)
//       Statistic to avoid unmatched element
//       Multiple go routine for convert()
func generateSpeedTable(wayid2speed map[uint64]int, way2nodeidsPath string, target string) {
	startTime := time.Now()

	// format is: wayid, nodeid, nodeid, nodeid...
	source := make(chan string)
	// format is fromid, toid, speed
	sink := make(chan string)

	go load(way2nodeidsPath, source)
	go convert(source, sink, wayid2speed)
	write(target, sink)

	endTime := time.Now()
	fmt.Printf("Processing time for generate speed table takes %f seconds\n", endTime.Sub(startTime).Seconds())
}

func load(mappingPath string, source chan<- string) {
	defer close(source)

	f, err := os.Open(mappingPath)
	defer f.Close()
	if err != nil {
		log.Fatal(err)
		fmt.Printf("Open idsmapping file of %v failed.\n", mappingPath)
		return
	}
	fmt.Printf("Open idsmapping file of %s succeed.\n", mappingPath)

	scanner := bufio.NewScanner(f)
	for scanner.Scan() {
		source <- (scanner.Text())
	}
}

func convert(source <-chan string, sink chan<- string, wayid2speed map[uint64]int) {
	var err error
	defer close(sink)

	for str := range source {
		elements := strings.Split(str, ",")
		if len(elements) < 3 {
			fmt.Printf("Invalid string %s in wayid2nodeids mapping file\n", str)
			continue
		}

		var wayid uint64
		if wayid, err = strconv.ParseUint(elements[0], 10, 64); err != nil {
			fmt.Printf("#Error during decoding wayid, row = %v\n", elements)
			continue
		}

		if speed, ok := wayid2speed[wayid]; ok {
			var nodes []string = elements[1:]
			for i := 0; (i + 1) < len(nodes); i++ {
				var n1, n2 uint64
				if n1, err = strconv.ParseUint(nodes[i], 10, 64); err != nil {
					fmt.Printf("#Error during decoding nodeid, row = %v\n", elements)
					continue
				}
				if n2, err = strconv.ParseUint(nodes[i+1], 10, 64); err != nil {
					fmt.Printf("#Error during decoding nodeid, row = %v\n", elements)
					continue
				}

				var s string
				if speed >= 0 {
					s = fmt.Sprintf("%d,%d,%d\n", n1, n2, speed)
				} else {
					s = fmt.Sprintf("%d,%d,%d\n", n2, n1, -speed)
				}

				sink <- s
			}
		}
	}
}

func write(targetPath string, sink chan string) {
	outfile, err := os.OpenFile(targetPath, os.O_RDWR|os.O_CREATE, 0755)
	defer outfile.Close()
	defer outfile.Sync()
	if err != nil {
		log.Fatal(err)
		fmt.Printf("Open output file of %s failed.\n", targetPath)
		return
	}
	fmt.Printf("Open output file of %s succeed.\n", targetPath)

	w := bufio.NewWriter(outfile)
	defer w.Flush()
	for str := range sink {
		_, err := w.WriteString(str)
		if err != nil {
			log.Fatal(err)
			return
		}
	}
}
