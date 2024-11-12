cmake_minimum_required (VERSION 3.0)
project (app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS -Wall)
set(BOOST_USE_STATIC_LIBS ON)
set(BOOST_USE_MULTITHREADED ON)
set(BOOST_USE_STATIC_RUNTIME OFF)

#Boost configuration
set(BOOST_USE_STATIC_LIBS ON)
set(BOOST_USE_MULTITHREADED ON)
set(BOOST_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.60 REQUIRED COMPONENTS date_time regex context)

add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Thread support
find_package(Threads REQUIRED)

set(SOURCES
        main.cpp
        ApiHandler.cpp
        RedirectHandler.cpp
        DbEnsure.cpp
        include/DbEnsure.h
)

find_library(FOLLY folly REQUIRED)
find_library(SNAPPY snappy REQUIRED)
find_library(WANGLE wangle REQUIRED)
find_library(FIZZ fizz REQUIRED)
find_library(GFLAGS gflags REQUIRED)
find_library(GLOG glog REQUIRED)
find_library(ZLIB z REQUIRED)
find_library(SSL ssl REQUIRED)
find_library(CRYPTO crypto REQUIRED)
find_library(PROXYGENHTTPSERVER proxygenhttpserver REQUIRED)
find_library(PROXYGEN proxygen REQUIRED)
#find_library(GTEST ls /usr/local/sharegtest REQUIRED)
#find_library(GTEST_MAIN gtest_main REQUIRED)
find_library(JEMALLOC jemalloc REQUIRED)
find_library(EVENT event REQUIRED)
find_library(DOUBLE_CONVERSION double-conversion REQUIRED)
find_library(SODIUM sodium REQUIRED)
find_library(UNWIND unwind REQUIRED)
find_library(PQXX pqxx REQUIRED)
find_library(PQ pq REQUIRED)
find_library(GEOIP GeoIP REQUIRED)
find_library(LZMA lzma REQUIRED)
find_library(LZ4 lz4 REQUIRED)
find_library(BZIP2 bz2 REQUIRED)
find_library(ZSTD zstd REQUIRED)
find_library(FMT fmt REQUIRED)

# this is a GNU library, sometimes it emerges by itself from binutils
find_library(LIBIBERTY iberty)
if (NOT LIBIBERTY)
    set(LIBIBERTY "")
endif(NOT LIBIBERTY)


set(LIBS
    ${PROXYGENHTTPSERVER}    ${PROXYGEN}                 ${WANGLE}             ${FIZZ}
    ${FOLLY}                 ${GLOG}                     ${GFLAGS}             ${SSL}
    ${ZLIB}                  ${EVENT}                    ${CRYPTO}             ${Boost_LIBRARIES}
    ${PTHREAD}               ${CMAKE_THREAD_LIBS_INIT}   ${DOUBLE_CONVERSION}  ${SNAPPY}
    ${JEMALLOC}              ${SODIUM}                   ${UNWIND}             ${LIBIBERTY}
    ${PQXX}                  ${PQ}                       ${GEOIP}              ${ZLIB}
    ${LZMA}                  ${LZ4}                      ${BZIP2}              ${ZSTD}
    ${FMT}
)

add_executable(app ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
