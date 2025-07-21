#include "cgal_test.h"

double test_point_distance() {
    Point p(1.0, 2.0);
    Point q(4.0, 6.0);
    
    // 计算欧几里得距离平方
    double sq_dist = CGAL::squared_distance(p, q);
    
    // 验证结果 (3^2 + 4^2 = 25)
    if (std::abs(sq_dist - 25.0) > 1e-6) {
        throw std::runtime_error("Distance calculation error. Expected 25, got " + 
                                 std::to_string(sq_dist));
    }
    
    return sq_dist;
}

double test_polygon_area() {
    Polygon polygon;
    polygon.push_back(Point(0.0, 0.0));
    polygon.push_back(Point(3.0, 0.0));
    polygon.push_back(Point(3.0, 4.0));
    polygon.push_back(Point(0.0, 4.0));
    
    // 计算多边形面积 (3*4=12)
    double area = polygon.area();
    
    if (std::abs(area - 12.0) > 1e-6) {
        throw std::runtime_error("Area calculation error. Expected 12, got " + 
                                std::to_string(area));
    }
    
    return area;
}

Point test_triangle_centroid() {
    Point a(0.0, 0.0);
    Point b(6.0, 0.0);
    Point c(3.0, 9.0);
    
    // 计算三角形重心
    Point centroid = CGAL::centroid(a, b, c);
    
    // 验证结果 (重心坐标应为 (3,3))
    if (std::abs(centroid.x() - 3.0) > 1e-6 || 
        std::abs(centroid.y() - 3.0) > 1e-6) {
        throw std::runtime_error("Centroid calculation error. Expected (3,3), got (" +
                                std::to_string(centroid.x()) + "," +
                                std::to_string(centroid.y()) + ")");
    }
    
    return centroid;
}