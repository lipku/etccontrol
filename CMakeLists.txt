
cmake_minimum_required(VERSION 2.6)
project(ETC)

set(CMAKE_BUILD_TYPE "Debug")

## Target
set(ETC_SRCS main.cpp AsyncSerial.cpp BufferedAsyncSerial.cpp EtcRsu.cpp AsioTcp.cpp AsioTcpServer.cpp tinyxml2.cpp)
add_executable(etccontrol ${ETC_SRCS})
add_definitions(-lsqlite3)

## Link libraries
set(BOOST_LIBS thread date_time system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(etccontrol ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(etccontrol ${CMAKE_THREAD_LIBS_INIT})
