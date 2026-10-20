# Shared between PC (this build) and firmware (firmware/CMakeLists.txt compiles the same sources).
add_library(coral_protocol STATIC
    protocol/CoralProtocol.cpp
    protocol/Crc32.cpp
    protocol/Payloads.cpp
    protocol/PacketWriter.cpp
    protocol/PacketDecoder.cpp
    protocol/SequenceTracker.cpp)
target_include_directories(coral_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})