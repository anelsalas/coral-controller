add_executable(CoralController WIN32
    main.cpp
    AppOptions.cpp
    ViewerApp.cpp
    ViewerUi.cpp
    Dx11Window.cpp
    FrameTexture.cpp)
target_link_libraries(CoralController PRIVATE coral_core imgui::imgui d3d11 dxgi)
# Keep a standard main(argc, argv) while using the GUI subsystem (no console window).
target_link_options(CoralController PRIVATE /ENTRY:mainCRTStartup)