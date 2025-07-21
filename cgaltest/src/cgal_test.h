#pragma once
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/centroid.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_2 Point;
typedef Kernel::Vector_2 Vector;
typedef CGAL::Polygon_2<Kernel> Polygon;

// 测试1: 计算两点间距离平方
double test_point_distance();

// 测试2: 计算多边形面积
double test_polygon_area();

// 测试3: 计算三角形重心
Point test_triangle_centroid();