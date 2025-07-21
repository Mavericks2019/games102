cmake_minimum_required(VERSION 3.10)
project(CGAL_Demo VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必需的依赖包
find_package(CGAL REQUIRED COMPONENTS Core)
find_package(Boost REQUIRED)

# 如果找不到CGAL，提供清晰的错误信息
if(NOT CGAL_FOUND)
    message(FATAL_ERROR "CGAL not found. Please install with: sudo apt install libcgal-dev")
endif()

# 包含CGAL的自动配置
include(${CGAL_USE_FILE})

# 添加可执行文件
add_executable(cgal-demo src/main.cpp src/cgal_test.cpp)

# 链接库
target_link_libraries(cgal-demo
    CGAL::CGAL
    ${CGAL_LIBRARIES}
    ${Boost_LIBRARIES}
)

# 安装规则（可选）
install(TARGETS cgal-demo DESTINATION bin)