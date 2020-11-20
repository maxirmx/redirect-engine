cmake_minimum_required(VERSION 3.0.0)
project(cpppgconnect VERSION 0.1.0)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

find_library(GLOG glog)
find_library(GFLAGS gflags)

FIND_PACKAGE( Boost 1.60 COMPONENTS date_time regex REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} 
        pq
        pqxx
        z
        ${GLOG}
        ${GFLAGS}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        pthread
        GeoIP)

include(CPack)


