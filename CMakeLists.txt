cmake_minimum_required(VERSION 3.25)
project(CoralController VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Strict warnings for our code; third-party headers are external (silenced).
    add_compile_options(/W4 /WX /permissive- /utf-8 /EHsc /external:anglebrackets /external:W0)
endif()
add_compile_definitions(_WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN NOMINMAX)

find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(boost_asio CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

enable_testing()
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(tests)