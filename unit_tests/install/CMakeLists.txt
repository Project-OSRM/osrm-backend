cmake_minimum_required(VERSION 3.18)
project(OSRMInstalledHeaders LANGUAGES CXX)

add_library(installed-preprocessing-consumer OBJECT customization.cpp)
target_compile_features(installed-preprocessing-consumer PRIVATE cxx_std_20)
target_include_directories(installed-preprocessing-consumer PRIVATE
    "${OSRM_INSTALL_PREFIX}/include"
    "${OSRM_INSTALL_PREFIX}/include/osrm")
if(MSVC)
    target_compile_options(installed-preprocessing-consumer PRIVATE /W4 /WX)
else()
    target_compile_options(installed-preprocessing-consumer PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
