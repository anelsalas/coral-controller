# coral_core: all testable logic (no UI toolkit dependency).
add_library(coral_core STATIC
    device/BoardDetector.cpp
    device/UsbHardwareId.cpp
    device/WindowsPortEnumerator.cpp
    device/AsioSerialPort.cpp
    device/SerialReader.cpp
    device/ConnectionManager.cpp
    protocol/Base64.cpp
    protocol/TextFrameParser.cpp
    imaging/JpegDecoder.cpp
    imaging/LatestFrameBuffer.cpp
    imaging/SnapshotWriter.cpp
    filters/FilterParameter.cpp
    filters/FilterBase.cpp
    filters/FilterRegistry.cpp
    filters/FilterPipeline.cpp
    filters/ImageConversions.cpp
    filters/RotateFilter.cpp
    filters/FlipFilter.cpp
    filters/BlurFilter.cpp
    filters/EdgeFilter.cpp
    filters/ThresholdFilter.cpp
    filters/ColorMapFilter.cpp
    filters/BuiltinFilters.cpp
    filters/DefaultPipeline.cpp
    status/FpsCounter.cpp
    status/StatusModel.cpp
    ui/FilterPanelModel.cpp
    viewer/ViewerSession.cpp)
target_include_directories(coral_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(coral_core PUBLIC
    opencv_core opencv_imgproc opencv_imgcodecs
    Boost::asio
    setupapi)