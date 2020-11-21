cmake_minimum_required (VERSION 3.0)
project (app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS -Wall)
set(BOOST_USE_STATIC_LIBS ON)
set(BOOST_USE_MULTITHREADED ON)
set(BOOST_USE_STATIC_RUNTIME OFF)

FIND_PACKAGE( Boost 1.60 COMPONENTS date_time regex REQUIRED ) 
link_directories(/usr/local/lib)

set(SOURCES main.cpp ApiHandler.cpp RedirectHandler.cpp)

find_library(FOLLY folly)
find_library(SNAPPY snappy)
find_library(WANGLE wangle)
find_package(Threads)
find_library(GFLAGS gflags)
find_library(GLOG glog)
find_library(Z z)
find_library(SSL ssl)
find_library(CRYPTO crypto)
find_library(PROXYGENHTTPSERVER proxygenhttpserver)
find_library(GTEST gtest)
find_library(GTEST_MAIN gtest_main)
find_library(DL dl)
find_library(jemalloc jemalloc)
find_library(EVENT event)
find_library(D double-conversion)

set(LIBS
        ${PROXYGENHTTPSERVER}
        ${WANGLE}
        ${FOLLY}
        ${ROCKSDB}
        ${GLOG}
        ${GFLAGS}
        ${SSL}
        ${Z}
        ${EVENT}
        ${CRYPTO}
        ${Boost_LIBRARIES}
        ${PTHREAD}
        ${CMAKE_THREAD_LIBS_INIT}
        ${D}
        ${SNAPPY}
        ${jemalloc}
		dl
		proxygen
		boost_context
		fizz
		sodium
		z
		folly
        pq
        pqxx
        GeoIP
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        lzma
        bz2)

add_executable(app ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
 
