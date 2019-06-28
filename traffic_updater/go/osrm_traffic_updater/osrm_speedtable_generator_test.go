package main

import (
	"encoding/csv"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"strconv"
	"strings"
	"testing"
)

func TestGenerateSpeedTable(t *testing.T) {
	wayid2speed := make(map[uint64]int)
	loadMockTraffic("./testdata/mock-traffic.csv", wayid2speed)
	generateSpeedTable(wayid2speed, "./testdata/id-mapping.csv", "./testdata/target.csv")

	b1, err1 := ioutil.ReadFile("./testdata/target.csv")
	if err1 != nil {
		fmt.Print(err1)
	}
	str1 := string(b1)

	b2, err2 := ioutil.ReadFile("./testdata/expect.csv")
	if err2 != nil {
		fmt.Print(err2)
	}
	str2 := string(b2)

	if strings.Compare(str1, str2) != 0 {
		t.Error("TestGenerateSpeedTable failed\n")
	}
}

func loadMockTraffic(trafficPath string, wayid2speed map[uint64]int) {
	// load mock traffic file
	mockfile, err := os.Open(trafficPath)
	defer mockfile.Close()
	if err != nil {
		log.Fatal(err)
		fmt.Printf("Open pbf file of %v failed.\n", trafficPath)
		return
	}
	fmt.Printf("Open pbf file of %s succeed.\n", trafficPath)

	csvr := csv.NewReader(mockfile)
	for {
		row, err := csvr.Read()
		if err != nil {
			if err == io.EOF {
				err = nil
				break
			} else {
				fmt.Printf("Error during decoding mock traffic, row = %v\n", err)
				return
			}
		}

		var wayid uint64
		var speed int64
		if wayid, err = strconv.ParseUint(row[0], 10, 64); err != nil {
			fmt.Printf("#Error during decoding wayid, row = %v\n", row)
		}
		if speed, err = strconv.ParseInt(row[1], 10, 32); err != nil {
			fmt.Printf("#Error during decoding speed, row = %v\n", row)
		}

		wayid2speed[wayid] = (int)(speed)
	}
}
