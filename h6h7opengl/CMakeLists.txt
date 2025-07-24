cmake_minimum_required(VERSION 3.5)
project(objViewer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt5组件
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)

# 添加着色器资源文件
set(SHADER_RESOURCES
    shaders.qrc
 )
qt5_add_resources(RESOURCE_FILES ${SHADER_RESOURCES})

find_package(OpenMesh REQUIRED)

# 添加Eigen库支持
find_package(Eigen3 REQUIRED)

# +++ 添加CGAL依赖 +++
# 首先查找GMP和MPFR（CGAL所需）
# find_package(GMP REQUIRED)
# find_package(MPFR REQUIRED)

# 查找CGAL库
find_package(CGAL REQUIRED COMPONENTS Core)

# 启用CGAL自动链接
include(${CGAL_USE_FILE})

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    glwidget/glwidget_core.cpp
    glwidget/glwidget_mesh_loader.cpp
    glwidget/glwidget_curvature.cpp
    glwidget/glwidget_minimal_surface.cpp
    glwidget/glwidget_loop_subdivision.cpp
    glwidget/glwidget_mesh_simplification.cpp
    glwidget/glwidget_parameteration.cpp
    glwidget/glwidget.h
    cvtwidget/cvtglwidget.cpp
    cvtwidget/cvtglwidget.h
    ${RESOURCE_FILES}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::OpenGL
    GL
    OpenMeshCore
    OpenMeshTools
    Eigen3::Eigen
    # +++ 添加CGAL相关库 +++
    CGAL::CGAL
    ${GMP_LIBRARIES}
    ${MPFR_LIBRARIES}
)

# 设置安装路径
install(TARGETS ${PROJECT_NAME} DESTINATION bin)