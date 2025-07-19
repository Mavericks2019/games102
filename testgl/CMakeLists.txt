cmake_minimum_required(VERSION 3.10)
project(PathTracingRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Widgets OpenGL REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(PathTracingRenderer
    main.cpp
    renderwidget.h
    renderwidget.cpp
)

target_include_directories(PathTracingRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 添加着色器文件
file(GLOB SHADERS "shaders/*")
target_sources(PathTracingRenderer PRIVATE ${SHADERS})

target_link_libraries(PathTracingRenderer
    Qt5::Core
    Qt5::Widgets
    Qt5::OpenGL
    GL
)

# 确保着色器文件被复制到构建目录
add_custom_command(TARGET PathTracingRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:PathTracingRenderer>/shaders
)