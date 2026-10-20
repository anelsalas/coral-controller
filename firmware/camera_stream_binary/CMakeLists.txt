set(CAMERA_STREAM_BINARY_VERSION "1.0.0")

add_executable_m7(camera_stream_binary
    camera_stream_binary.cc
    UsbConsoleSink.cc
    ${CORAL_SHARED_PROTOCOL_SOURCES}
)
target_include_directories(camera_stream_binary PRIVATE ${CORAL_SHARED_DIR})
target_compile_definitions(camera_stream_binary PRIVATE
    CAMERA_STREAM_BINARY_VERSION="${CAMERA_STREAM_BINARY_VERSION}")
target_link_libraries(camera_stream_binary
    libs_base-m7_freertos
    libs_jpeg_m7
)