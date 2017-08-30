#!/bin/sh
#
#  run_benchmark_write_pbf.sh
#
#  Will read the input file and after reading it into memory completely,
#  write it to /dev/null. Because this will need the time to read *and* write
#  the file, it will report the times for reading and writing. You can
#  subtract the times needed for the "count" benchmark to (roughly) get the
#  write times.
#

set -e

BENCHMARK_NAME=write_pbf

. @CMAKE_BINARY_DIR@/benchmarks/setup.sh

CMD=$OB_DIR/osmium_benchmark_$BENCHMARK_NAME

echo "# file size num mem time cpu_kernel cpu_user cpu_percent cmd options"
for data in $OB_DATA_FILES; do
    filename=`basename $data`
    filesize=`stat --format="%s" --dereference $data`
    for n in $OB_SEQ; do
        $OB_TIME_CMD -f "$filename $filesize $n $OB_TIME_FORMAT" $CMD $data /dev/null 2>&1 >/dev/null | sed -e "s%$DATA_DIR/%%" | sed -e "s%$OB_DIR/%%"
    done
done

