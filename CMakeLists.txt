cmake_minimum_required(VERSION 3.0.0)
project(joystick2redis VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -g ")
add_definitions(-DLINUX -std=c++11)
# set(CMAKE_BUILD_TYPE Debug)

set(LIMERO limero)
set(HIREDIS hiredis )



add_executable(joystick2redis)

target_sources(joystick2redis PRIVATE 
    src/joystick2redis.cpp 
    src/Joystick.cpp 
    src/LogFile.cpp
    src/config.cpp
    ${LIMERO}/src/StringUtility.cpp
    ${LIMERO}/src/printf.c
    ${LIMERO}/linux/Redis.cpp
    ${LIMERO}/linux/Log.cpp
    ${LIMERO}/linux/Sys.cpp
    ${LIMERO}/linux/limero.cpp
    ) 


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(GCC_FLAGS "-DLINUX")
include(CPack)

add_definitions(${GCC_FLAGS})

include_directories(src  
        limero/inc 
        limero/linux 
        ArduinoJson/src 
        ../paho.mqtt.c/src  
        ${HIREDIS}
        )

target_link_libraries(joystick2redis 
        -lpthread 
        -L../hiredis -l:libhiredis.a
        -lrt 
        -pthread 
        -latomic )

EXECUTE_PROCESS( COMMAND arch OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE )

if ( ${ARCH} STREQUAL "armv6l" OR ${ARCH} STREQUAL "armv7l")
        target_link_libraries(joystick2redis -lwiringPi)
	set(GCC_FLAGS "-DHAS_GPIO")
endif()

