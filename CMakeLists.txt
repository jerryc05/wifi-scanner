cmake_minimum_required(VERSION 3.16)
project(wlan_scanner)

set(CMAKE_CXX_STANDARD 20)

add_executable(wlan_scanner main.cpp)
target_link_libraries(${PROJECT_NAME} wlanapi)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message("Adding GCC Flags...")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
-fdelete-dead-exceptions \
-fstrict-enums \
-ftrapv \
-funroll-loops \
-std=gnu++2a \
-Wall \
-Wcast-align \
-Wconversion \
-Wctor-dtor-privacy \
-Wdelete-non-virtual-dtor \
-Wdeprecated-copy \
-Wdouble-promotion \
-Wduplicated-branches \
-Wduplicated-cond \
-Wextra \
-Wfloat-equal \
-Winit-self \
-Wlogical-op \
-Wmisleading-indentation \
-Wnon-virtual-dtor \
-Wnull-dereference \
-Wold-style-cast \
-Woverloaded-virtual \
-Wpedantic \
-Wpointer-arith \
-Wshadow \
-Wsign-conversion \
-Wundef \
-Wunused \
-Wuseless-cast \
-Wwrite-strings \
")
endif ()
message("")

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    message("CMAKE IN DEBUG MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
elseif (CMAKE_BUILD_TYPE MATCHES "Release")
    message("CMAKE IN RELEASE MODE")
    if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
    endif ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -fexpensive-optimizations -march=native")
endif ()
message("")

message("[${CMAKE_CXX_FLAGS}]")
message("")