# Coral Dev Board Micro firmware for Coral Controller, built out-of-tree against the
# coralmicro SDK submodule (third_party/coralmicro). Build: scripts\build-firmware.ps1
cmake_minimum_required(VERSION 3.18)

get_filename_component(CORAL_MICRO_SDK "${CMAKE_CURRENT_LIST_DIR}/../third_party/coralmicro" REALPATH)
get_filename_component(CORAL_SHARED_DIR "${CMAKE_CURRENT_LIST_DIR}/../shared" REALPATH)
# The SDK toolchain file downloads Arm's official gcc-arm-none-eabi-9-2020-q2 (win32) into
# third_party/coralmicro/third_party/toolchain-win on first configure.
set(CMAKE_TOOLCHAIN_FILE ${CORAL_MICRO_SDK}/cmake/toolchain-arm-none-eabi-gcc.cmake)

project(coral_controller_firmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CORAL_MICRO_SDK})
add_subdirectory(${CORAL_MICRO_SDK} coralmicro)

set(CORAL_SHARED_PROTOCOL_SOURCES
    ${CORAL_SHARED_DIR}/protocol/CoralProtocol.cpp
    ${CORAL_SHARED_DIR}/protocol/Crc32.cpp
    ${CORAL_SHARED_DIR}/protocol/Payloads.cpp
    ${CORAL_SHARED_DIR}/protocol/PacketWriter.cpp
    ${CORAL_SHARED_DIR}/transport/PacedChunkSink.cpp)

add_subdirectory(camera_stream_binary)