cmake_minimum_required(VERSION 3.5)
project(OBJModelViewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED) # 添加OpenGL模块
find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${EIGEN3_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    objmodelcanvas.cpp
    basecanvaswidget.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Core
    Qt5::OpenGL # 链接OpenGL模块
    pthread
    ${OpenCV_LIBS}
)