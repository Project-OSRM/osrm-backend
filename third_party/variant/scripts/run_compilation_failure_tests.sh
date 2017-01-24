#!/bin/sh
#
#  Try to compile all programs in the test/compilation_failure directory.
#  Compilation must fail and the error message must match the pattern in the
#  corresponding .pattern file.
#

DIR="test/compilation_failure"
CXX=${CXX:-clang++}

if [ `uname -s` = "Darwin" ]; then
    CXXFLAGS="$CXXFLAGS -stdlib=libc++"
fi

error_msg() {
    if [ ! -z "$1" ]; then
	printf 'output was:\n=======\n%s\n=======\n' "$1"
    fi
}

exit_code=0
for test_code in $DIR/*.cpp; do
    name=`basename $test_code .cpp`

    result=`${CXX} -std=c++11 -c -o /dev/null -I./include ${CXXFLAGS} ${test_code} 2>&1`
    status=$?

    if [ $status = 1 ]; then
	expected=`sed -n -e '/@EXPECTED/s/.*: \+//p' ${test_code}`
	if echo $result | grep -q "$expected"; then
	    echo "$name [OK]"
	else
	    echo "$name [FAILED - wrong error message]"
	    echo "Expected error message: $expected"
	    error_msg "$result"
	    exit_code=1
	fi
    elif [ $status = 0 ]; then
	echo "$name [FAILED - compile was successful]"
	error_msg "$result"
	exit_code=1
    else
	echo "$name [FAILED - unknown error in compile]"
	error_msg "$result"
	exit_code=1
    fi
done

exit ${exit_code}
