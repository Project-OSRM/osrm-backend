package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"time"

	"github.com/golang/snappy"
)

func loadWay2NodeidsTable(filepath string, sources [TASKNUM]chan string) {
	startTime := time.Now()

	data := make(chan string)
	go load(filepath, data)
	convert(data, sources)

	endTime := time.Now()
	fmt.Printf("Processing time for loadWay2NodeidsTable takes %f seconds\n", endTime.Sub(startTime).Seconds())
}

func load(mappingPath string, data chan<- string) {
	defer close(data)

	f, err := os.Open(mappingPath)
	defer f.Close()
	if err != nil {
		log.Fatal(err)
		fmt.Printf("Open idsmapping file of %v failed.\n", mappingPath)
		return
	}
	fmt.Printf("Open idsmapping file of %s succeed.\n", mappingPath)

	scanner := bufio.NewScanner(snappy.NewReader(f))
	for scanner.Scan() {
		data <- (scanner.Text())
	}
}

// input data format
// wayid1, n1, n2
// wayid2, n3, n4, n5
func convert(data <-chan string, sources [TASKNUM]chan string) {
	for i := range sources {
		defer close(sources[i])
	}

	var count int
	for str := range data {
		chanIndex := count % TASKNUM
		count++
		sources[chanIndex] <- str
	}
}
