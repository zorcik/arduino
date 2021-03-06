cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

#-------------------------------------------------------------------------------
# User configuration
#-------------------------------------------------------------------------------
# Set Arduino sketchbook directory
if(WIN32)
    set(SKETCHBOOK_DIR  C:/Users/$ENV{USERNAME}/Documents/Arduino)
elseif(UNIX)
    set(SKETCHBOOK_DIR  $ENV{HOME}/Arduino)
endif()

# Configure one sketch
# set(SKETCH examples/AgingOffset/AgingOffset.ino)
# set(SKETCH examples/AlarmInterrupt/AlarmInterrupt.ino)
# set(SKETCH examples/AlarmPolling/AlarmPolling.ino)
set(SKETCH examples/GettingStarted/GettingStarted.ino)
# set(SKETCH examples/PrintDiagnostics/PrintDiagnostics.ino)
# set(SKETCH examples/ReadTimeInterrupt/ReadTimeInterrupt.ino)
# set(SKETCH examples/ReadTimePolled/ReadTimePolled.ino)
# set(SKETCH examples/SetDateTime/SetDateTime.ino)
# set(SKETCH examples/Temperature/Temperature.ino)
# set(SKETCH examples/Terminal/Terminal.ino)

# Configure board, default Arduino UNO
set(BOARD       uno)
# set(BOARD       nano)

if(${BOARD} STREQUAL nano)
    set(nano.upload.maximum_size 30720)
    set(nano.upload.maximum_data_size 2048)
    set(nano.upload.speed 57600)
    set(nano.build.mcu atmega328p)
endif()

# Configure serial port:
if(WIN32)
    set(SERIAL_PORT     COM3)
elseif(UNIX)
    set(SERIAL_PORT     /dev/ttyUSB0)
endif()

#-------------------------------------------------------------------------------
# Project configuation
#-------------------------------------------------------------------------------
# Project name
set(PROJECT_NAME        ErriezDS3231)

# Set library directory
set(LIB_SRC_DIR                     ${SKETCHBOOK_DIR}/libraries/${PROJECT_NAME}/src)
set(LIB_SERIAL_TERMINAL_SRC_DIR     ${SKETCHBOOK_DIR}/libraries/ErriezSerialTerminal/src)

# Configure project
project(${PROJECT_NAME})

# Configure include directories
include_directories(
        ${LIB_SRC_DIR}
        ${LIB_SERIAL_TERMINAL_SRC_DIR}
)

# Generate library
generate_arduino_library(
        ${PROJECT_NAME}Lib
        SRCS
            ${LIB_SRC_DIR}/ErriezDS3231.cpp
            ${LIB_SRC_DIR}/ErriezDS3231_debug.cpp
        HDRS
            ${LIB_SRC_DIR}/ErriezDS3231.h
            ${LIB_SRC_DIR}/ErriezDS3231_debug.h
)

generate_arduino_library(
        SerialTerminalLib
        SRCS
            ${LIB_SERIAL_TERMINAL_SRC_DIR}/ErriezSerialTerminal.cpp
        HDRS
            ${LIB_SERIAL_TERMINAL_SRC_DIR}/ErriezSerialTerminal.h
)

# Configure project
set(${CMAKE_PROJECT_NAME}_SKETCH    ${SKETCH})
set(${CMAKE_PROJECT_NAME}_LIBS      ${PROJECT_NAME}Lib SerialTerminalLib)
set(${CMAKE_PROJECT_NAME}_BOARD     ${BOARD})
set(${CMAKE_PROJECT_NAME}_PORT      ${SERIAL_PORT})
generate_arduino_firmware(${CMAKE_PROJECT_NAME})
