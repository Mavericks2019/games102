cmake_minimum_required(VERSION 3.15)
project(CurveEditorTool)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 自动处理Qt的moc、uic和rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找Qt5库
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    control_point.h
    canvas.h
    canvas.cpp
    curve_designer.cpp
    curve_designer.h
)
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIRS})
# 链接Qt和OpenCV库
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Core
    ${OpenCV_LIBS}
    Eigen3::Eigen    
)