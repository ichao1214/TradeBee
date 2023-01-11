cmake_minimum_required(VERSION 3.12)

project(TradeBee)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS -std=c++14)

set(CMAKE_INSTALL_RPATH "$ORIGIN" "$ORIGIN/lib" "/usr/local/lib64" "/usr/local/lib" "$ORIGIN/thirdparty/mysql-connector-c++/lib64/private" "./")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_C_COMPILER "/opt/rh/devtoolset-8/root/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/opt/rh/devtoolset-8/root/usr/bin/g++")

add_definitions(-Wall -ggdb)

IF (CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "CMAKE_BUILD_TYPE is Debug")
    set(CMAKE_BUILD_TYPE "Debug")
ELSEIF (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    message(STATUS "CMAKE_BUILD_TYPE is Release")
    set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_CXX_FLAGS "-O3")
ELSE ()
    message(WARNING "unknown CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})
ENDIF ()

include_directories(broker_api/ctp/include)
include_directories(thirdparty/cmdline)
include_directories(thirdparty/spdlog)
include_directories(thirdparty/mysql-connector-c++/include)
include_directories(thirdparty)
include_directories(comm)

link_directories(broker_api/ctp/lib)
link_directories(/usr/local/lib)
link_directories(thirdparty/mysql-connector-c++/lib64)
link_directories(thirdparty/mysql-connector-c++/lib64/private)


file(GLOB SOURCE */*.h */*.cpp
                 */*/*.h */*/*.cpp)

add_executable(TradeBee ${SOURCE})

target_link_libraries(TradeBee thosttraderapi_se thostmduserapi_se boost_filesystem pthread mysqlcppconn ssl)