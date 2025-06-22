cmake_minimum_required(VERSION 3.5)
project(ObjViewer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt5组件
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

# 设置安装路径
install(TARGETS ${PROJECT_NAME} DESTINATION bin)