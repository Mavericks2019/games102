cmake_minimum_required(VERSION 3.5)

project(OpenGLSquare LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理UI、资源和MOC文件
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找所需的Qt模块
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::OpenGL
)

# 在macOS上添加OpenGL库
if(APPLE)
    find_library(OPENGL_LIBRARY OpenGL)
    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARY})
endif()

# 在Linux和Windows上添加OpenGL库
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} OpenGL::GL)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
endif()