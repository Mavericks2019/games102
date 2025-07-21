#include <iostream>
#include "cgal_test.h"

int main() {
    try {
        // 测试点距离计算
        double dist = test_point_distance();
        std::cout << "✅ Point distance test passed\n";
        std::cout << "   Computed distance squared: " << dist << "\n\n";

        // 测试多边形面积计算
        double area = test_polygon_area();
        std::cout << "✅ Polygon area test passed\n";
        std::cout << "   Computed area: " << area << "\n\n";

        // 测试三角形重心
        Point centroid = test_triangle_centroid();
        std::cout << "✅ Triangle centroid test passed\n";
        std::cout << "   Centroid: (" << centroid.x() << ", " << centroid.y() << ")\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        std::cerr << "Possible solutions:" << std::endl;
        std::cerr << "1. Ensure WSL is updated: sudo apt update && sudo apt upgrade" << std::endl;
        std::cerr << "2. Install CGAL dependencies: sudo apt install libcgal-dev libboost-all-dev" << std::endl;
        std::cerr << "3. Check CMake configuration" << std::endl;
        return 1;
    }
}