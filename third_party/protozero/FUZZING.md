
To do fuzz testing using [AFL](https://lcamtuf.coredump.cx/afl/) compile with
the AFL compiler wrappers:

    mkdir build
    cd build
    CC=afl-clang CXX=afl-clang++ cmake ..
    mkdir testcase_dir

You need some data to start the fuzzing. In this case I am using all the test
messages from the unit tests:

    find ../test/t/ -name data-\*.pbf -a -not -empty -exec cp {} testcase_dir/ \;

Then do the actual fuzzing:

    afl-fuzz -i testcase_dir -o findings_dir -- tools/pbf-decoder -

See the AFL documentation for more information.

This only checkes the reading side of Protozero!

