#!/bin/sh

g++ -std=c++14 -L../build example.cpp -losrm -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lrt -lpthread -o example -I ../include
