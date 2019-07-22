package main

import (
	"flag"
	"fmt"
	"io"
	"os"
	"strings"

	"github.com/golang/snappy"
)

const snappySuffix string = ".snappy"

var flags struct {
	input  string
	output string
	suffix string
}

func init() {
	flag.StringVar(&flags.input, "i", "", "Input file.")
	flag.StringVar(&flags.output, "o", "", "Output file.")
}

func main() {
	flag.Parse()

	inputCompressed := strings.HasSuffix(flags.input, snappySuffix)
	outputCompressed := strings.HasSuffix(flags.output, snappySuffix)

	if inputCompressed == outputCompressed {
		fmt.Printf("Error. Input and output must have one with .snappy suffix and one without.\n")
		return
	}

	fi, err1 := os.Open(flags.input)
	if err1 != nil {
		fmt.Printf("Open input file failed.\n")
		return
	}
	defer fi.Close()

	fo, err2 := os.OpenFile(flags.output, os.O_RDWR|os.O_CREATE, 0755)
	if err2 != nil {
		fmt.Printf("Open output file failed.\n")
		return
	}
	defer fo.Close()
	defer fo.Sync()

	buf := make([]byte, 128*1024)
	if inputCompressed {
		_, err := io.CopyBuffer(fo, snappy.NewReader(fi), buf)
		if err != nil {
			fmt.Printf("Decompression failed\n")
		}
	} else {
		_, err := io.CopyBuffer(snappy.NewWriter(fo), fi, buf)
		if err != nil {
			fmt.Printf("Compression failed\n")
		}
	}

}
