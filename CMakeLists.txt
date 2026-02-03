cmake_minimum_required(VERSION 3.15)

project(camspec
    VERSION 0.1.0
    DESCRIPTION "C++ DNG + 24-patch ColorChecker color calibration pipeline"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Set OpenCV directory - clear cache if you moved OpenCV
set(OpenCV_DIR "C:/opencv-build" CACHE PATH "OpenCV config directory" FORCE)

# Find OpenCV - it should locate OpenCVConfig.cmake in C:/opencv-build
find_package(OpenCV REQUIRED)

# Print OpenCV info for debugging
message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

add_library(camspec_lib
    src/io.cpp
    src/chart.cpp
    src/calib.cpp
    src/refdata.cpp
    src/spectral.cpp
    src/profile.cpp
    src/pipeline.cpp
)

target_include_directories(camspec_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/tinydng
)

target_link_libraries(camspec_lib
    PUBLIC
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
        opencv_highgui
        Eigen3::Eigen
)

add_executable(camspec
    src/main.cpp
)

target_link_libraries(camspec
    PRIVATE camspec_lib
)

enable_testing()

add_executable(camspec_tests
    tests/identity_test.cpp
)

target_link_libraries(camspec_tests
    PRIVATE camspec_lib
)

add_test(NAME camspec_identity_test
         COMMAND camspec_tests)

