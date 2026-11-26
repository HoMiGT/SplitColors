cmake_minimum_required(VERSION 4.3)
project(SplitColors LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc)

add_executable(SplitColors main.cpp config.cpp image_processing.cpp)
target_include_directories(SplitColors PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(SplitColors PRIVATE ${OpenCV_LIBS})
