cmake_minimum_required(VERSION 3.10)
project(PathTracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Gui OpenGL Widgets REQUIRED)

add_executable(PathTracer main.cpp)
target_link_libraries(PathTracer Qt5::Core Qt5::Gui Qt5::OpenGL Qt5::Widgets)