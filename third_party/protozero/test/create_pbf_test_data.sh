#!/bin/sh
#
#  create_pbf_test_data.sh [TESTCASE]
#
#  This script creates the test data for the given test case in protobuf format
#  using the testcase.proto description and the testcase.cpp code.
#
#  If called without a test case it will iterate over all test cases generating
#  all data.
#
#  This program should be called with the "test" directory as current directory.
#

set -e

if [ -z "$CXX" ]; then
    echo "Please set CXX before running this script"
    exit 1
fi

if [ -z "$1" ]; then
    for dir in t/*; do
        $0 $dir
    done
fi

echo "Generating $1..."
cd $1
if [ -f testcase.proto ]; then
    protoc --cpp_out=. testcase.proto
    $CXX -std=c++11 -I../../include -o testcase testcase.cpp testcase.pb.cc -lprotobuf-lite -pthread
    ./testcase
fi
cd ../..

