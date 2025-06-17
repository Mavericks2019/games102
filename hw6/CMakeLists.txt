cmake_minimum_required(VERSION 3.5)
project(InterpolationTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

include_directories(${EIGEN3_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    basecanvaswidget.cpp
    parametriccurvecanvas.cpp
    cubicsplinecanvas.cpp
    beziercurvecanvas.cpp
    bsplinecanvas.cpp
    polygoncanvas.cpp 
    objmodelcanvas.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Core
    ${OpenCV_LIBS}
)