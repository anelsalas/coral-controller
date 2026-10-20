include(GoogleTest)

add_library(coral_test_support INTERFACE)
target_include_directories(coral_test_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(coral_test_support INTERFACE coral_core GTest::gtest GTest::gtest_main)

# Pure unit tests: no threads, no hardware.
add_executable(coral_unit_tests
    unit/BoardDetectorTest.cpp
    unit/TextFrameParserTest.cpp
    unit/JpegDecoderTest.cpp
    unit/LatestFrameBufferTest.cpp
    unit/FilterPipelineTest.cpp
    unit/BuiltinFiltersTest.cpp
    unit/FilterParameterTest.cpp
    unit/FilterPanelModelTest.cpp
    unit/FilterRegistryTest.cpp
    unit/DefaultPipelineTest.cpp
    unit/SnapshotWriterTest.cpp
    unit/FpsCounterTest.cpp
    unit/StatusModelTest.cpp
    unit/ProtocolDetectorTest.cpp
    unit/BinaryViewerSessionTest.cpp
    unit/protocol/ProtocolHeaderTest.cpp
    unit/protocol/Crc32Test.cpp
    unit/protocol/PacketCodecTest.cpp
    unit/protocol/SequenceTrackerTest.cpp
    unit/protocol/PacketWriterTest.cpp
    unit/transport/PacedChunkSinkTest.cpp)
target_link_libraries(coral_unit_tests PRIVATE coral_test_support Boost::crc)
gtest_discover_tests(coral_unit_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS unit)

# Integration tests: real threads, fake serial port / enumerator / clock.
add_executable(coral_integration_tests
    integration/ConnectionManagerTest.cpp)
target_link_libraries(coral_integration_tests PRIVATE coral_test_support)
gtest_discover_tests(coral_integration_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS integration)

# Hardware tests: need a connected Coral Dev Board Micro. Excluded from the default test presets.
add_executable(coral_hardware_tests hardware/HardwareViewerTest.cpp)
target_link_libraries(coral_hardware_tests PRIVATE coral_test_support)
gtest_discover_tests(coral_hardware_tests DISCOVERY_MODE PRE_TEST
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES LABELS hardware)