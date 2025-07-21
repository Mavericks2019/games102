cmake_minimum_required(VERSION 3.10)
project(VoronoiIterationVisualizer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt5组件
find_package(Qt5 COMPONENTS Core Widgets OpenGL REQUIRED)

# 查找CGAL库
find_package(CGAL REQUIRED COMPONENTS Core)

# 包含CGAL模块
include(${CGAL_USE_FILE})

# 自动处理MOC文件
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::OpenGL
)

# 链接CGAL库
target_link_libraries(${PROJECT_NAME}
    CGAL::CGAL
)

# 在macOS上添加OpenGL库
if(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} OpenGL::GL)
endif()

# 在Linux上添加OpenGL库
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} OpenGL::GL)
endif()

# 在Windows上添加OpenGL库
if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
endif()