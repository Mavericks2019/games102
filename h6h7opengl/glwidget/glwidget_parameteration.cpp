#include <algorithm>
#include <cmath>
#include <map>
#include "glwidget.h"
#include <vector>
#include <queue>
#include <fstream>
#include <Eigen/SparseLU>

// 边界映射到圆形
void GLWidget::mapBoundaryToCircle() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 查找起始边界点
    Mesh::VertexHandle start_v;
    for(auto vh : openMesh.vertices()) {
        if(openMesh.is_boundary(vh)) {
            start_v = vh;
            break;
        }
    }
    
    // 获取有序边界顶点
    std::vector<Mesh::VertexHandle> boundary;
    Mesh::VertexHandle pre, now;
    boundary.push_back(start_v);
    now = start_v;
    
    // 查找下一个边界点
    for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
        if(openMesh.is_boundary(*vv_it)) {
            pre = now;
            now = *vv_it;
            break;
        }
    }
    
    // 遍历边界
    while(now != start_v) {
        boundary.push_back(now);
        Mesh::VertexHandle next;
        for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
            if(openMesh.is_boundary(*vv_it) && *vv_it != pre) {
                next = *vv_it;
                break;
            }
        }
        pre = now;
        now = next;
    }

    // 计算总弧长
    float arc_len = 0.0f;
    for(int i = 1; i < boundary.size(); ++i) {
        arc_len += (openMesh.point(boundary[i]) - openMesh.point(boundary[i-1])).norm();
    }
    arc_len += (openMesh.point(boundary[boundary.size()-1]) - openMesh.point(boundary[0])).norm();
    
    // 计算每段弧长对应的角度增量
    std::vector<float> delta;
    for(int i = 1; i < boundary.size(); ++i) {
        float seg_len = (openMesh.point(boundary[i]) - openMesh.point(boundary[i-1])).norm();
        delta.push_back(2.0f * M_PI * (seg_len / arc_len));
    }
    
    // 映射到单位圆
    float angle_now = 0.0f;
    for(size_t i = 0; i < boundary.size(); ++i) {
        float x = cos(angle_now);
        float y = sin(angle_now);
        openMesh.set_point(boundary[i], Mesh::Point(x, y, 0));
        if(i < boundary.size() - 1) {
            angle_now += delta[i];
        }
    }
}

// 边界映射到矩形
void GLWidget::mapBoundaryToRectangle() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 查找起始边界点
    Mesh::VertexHandle start_v;
    for(auto vh : openMesh.vertices()) {
        if(openMesh.is_boundary(vh)) {
            start_v = vh;
            break;
        }
    }
    
    // 获取有序边界顶点
    std::vector<Mesh::VertexHandle> boundary;
    Mesh::VertexHandle pre, now;
    boundary.push_back(start_v);
    now = start_v;
    
    // 查找下一个边界点
    for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
        if(openMesh.is_boundary(*vv_it)) {
            pre = now;
            now = *vv_it;
            break;
        }
    }
    
    // 遍历边界
    while(now != start_v) {
        boundary.push_back(now);
        Mesh::VertexHandle next;
        for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
            if(openMesh.is_boundary(*vv_it) && *vv_it != pre) {
                next = *vv_it;
                break;
            }
        }
        pre = now;
        now = next;
    }

    const int n = boundary.size();
    const float length = 1.0f; // 正方形边长
    
    // 计算四条边上的点数（尽可能平均分配）
    int side1 = n / 4;
    int side2 = n / 4;
    int side3 = n / 4;
    int side4 = n - 3 * (n / 4);
    
    // 设置四个角点
    openMesh.set_point(boundary[0], Mesh::Point(0.0f, 0.0f, 0.0f));
    openMesh.set_point(boundary[side1], Mesh::Point(0.0f, length, 0.0f));
    openMesh.set_point(boundary[side1 + side2], Mesh::Point(length, length, 0.0f));
    openMesh.set_point(boundary[side1 + side2 + side3], Mesh::Point(length, 0.0f, 0.0f));
    
    // 左边 (y: 0 → length)
    float delta = length / side1;
    for (int i = 1; i < side1; ++i) {
        float y = i * delta;
        openMesh.set_point(boundary[i], Mesh::Point(0.0f, y, 0.0f));
    }
    
    // 上边 (x: 0 → length)
    delta = length / side2;
    for (int i = 1; i < side2; ++i) {
        int idx = side1 + i;
        float x = i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(x, length, 0.0f));
    }
    
    // 右边 (y: length → 0)
    delta = length / side3;
    for (int i = 1; i < side3; ++i) {
        int idx = side1 + side2 + i;
        float y = length - i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(length, y, 0.0f));
    }
    
    // 下边 (x: length → 0)
    delta = length / side4;
    for (int i = 1; i < side4; ++i) {
        int idx = side1 + side2 + side3 + i;
        float x = length - i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(x, 0.0f, 0.0f));
    }
}

void GLWidget::normalizeMesh() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 计算边界框
    Mesh::Point min(1e9, 1e9, 0), max(-1e9, -1e9, 0);
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        min[0] = std::min(min[0], p[0]);
        min[1] = std::min(min[1], p[1]);
        max[0] = std::max(max[0], p[0]);
        max[1] = std::max(max[1], p[1]);
    }
    
    // 计算中心点和范围
    Mesh::Point center((min[0] + max[0]) / 2, (min[1] + max[1]) / 2, 0);
    float range_x = max[0] - min[0];
    float range_y = max[1] - min[1];
    
    // 获取视图尺寸
    int viewWidth = width();
    int viewHeight = height();
    float aspectRatio = static_cast<float>(viewWidth) / viewHeight;
    
    // 计算缩放因子 - 使用最小边
    float scaleFactor;
    if (aspectRatio > 1.0f) {
        // 宽屏 - 以高度为基准
        scaleFactor = 2.0f / (range_y > 0 ? range_y : 1.0f);
    } else {
        // 竖屏 - 以宽度为基准
        scaleFactor = 2.0f / (range_x > 0 ? range_x : 1.0f);
    }
    
    // 归一化所有顶点
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        p -= center; // 平移到中心
        p[0] *= scaleFactor;
        p[1] *= scaleFactor;
        p[2] = 0.0f;
        openMesh.set_point(vh, p);
    }
    
    // 更新纹理坐标（归一化后）
    updateTextureCoordinates();
}

// 求解参数化
void GLWidget::solveParameterization() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        isBoundary[vh.idx()] = openMesh.is_boundary(vh);
    }

    // 准备数据：计算每个顶点的余切权重
    std::vector<std::map<int, float>> weights(openMesh.n_vertices());
    for (auto vh : openMesh.vertices()) {
        int i = vh.idx();
        for (auto heh : openMesh.voh_range(vh)) {
            if (!openMesh.is_boundary(heh)) {
                auto vj = openMesh.to_vertex_handle(heh);
                int j = vj.idx();
                
                // 计算两个相邻三角形的角度
                auto from = openMesh.from_vertex_handle(heh);
                auto to = openMesh.to_vertex_handle(heh);
                auto next = openMesh.next_halfedge_handle(heh);
                auto opp_next = openMesh.next_halfedge_handle(openMesh.opposite_halfedge_handle(heh));
                
                auto p1 = openMesh.point(from);
                auto p2 = openMesh.point(to);
                auto p3 = openMesh.point(openMesh.to_vertex_handle(next));
                auto p4 = openMesh.point(openMesh.to_vertex_handle(opp_next));
                
                // 计算两个角度
                Eigen::Vector3f v1 = {p1[0]-p2[0], p1[1]-p2[1], p1[2]-p2[2]};
                Eigen::Vector3f v2 = {p3[0]-p2[0], p3[1]-p2[1], p3[2]-p2[2]};
                Eigen::Vector3f v3 = {p4[0]-p2[0], p4[1]-p2[1], p4[2]-p2[2]};
                
                float angle1 = acos(v1.dot(v2) / (v1.norm() * v2.norm()));
                float angle2 = acos(v1.dot(v3) / (v1.norm() * v3.norm()));
                
                // 计算余切权重
                float w1 = 1.0f / tan(angle1);
                float w2 = 1.0f / tan(angle2);
                float w = (w1 + w2) / 2.0f;  // 平均权重
                
                weights[i][j] = w;
            }
        }
    }

    // 构建线性方程组
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    int n = openMesh.n_vertices();
    SpMat A(n, n);
    VectorXf b_u(n), b_v(n);
    VectorXf x(n), y(n);
    
    b_u.setZero();
    b_v.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n * 10);
    
    for (int i = 0; i < n; i++) {
        if (isBoundary[i]) {
            // 边界顶点：固定位置
            triplets.push_back(Triplet(i, i, 1.0f));
            b_u[i] = openMesh.point(Mesh::VertexHandle(i))[0];
            b_v[i] = openMesh.point(Mesh::VertexHandle(i))[1];
        } else {
            // 内部顶点：使用余切权重
            float totalWeight = 0.0f;
            for (const auto& [j, w] : weights[i]) {
                triplets.push_back(Triplet(i, j, w));
                totalWeight += w;
            }
            triplets.push_back(Triplet(i, i, -totalWeight));
            b_u[i] = 0.0f;
            b_v[i] = 0.0f;
        }
    }
    
    // 设置稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    // 使用SparseLU求解器
    Eigen::SparseLU<SpMat> solver;
    solver.analyzePattern(A);
    solver.factorize(A);
    
    if (solver.info() != Eigen::Success) {
        std::cerr << "Matrix factorization failed!" << std::endl;
        return;
    }
    
    // 求解坐标
    x = solver.solve(b_u);
    y = solver.solve(b_v);
    
    // 更新顶点位置
    for (int i = 0; i < n; i++) {
        Mesh::Point newPos(x[i], y[i], 0.0f);
        openMesh.set_point(Mesh::VertexHandle(i), newPos);
    }
}

// 执行参数化
void GLWidget::performParameterization() {
    // 根据边界类型映射边界
    if (boundaryType == Circle) {
        mapBoundaryToCircle();
    } else {
        mapBoundaryToRectangle();
    }
    
    // 求解参数化
    solveParameterization();
    
    // 归一化网格到中心原点，范围[-1,1]
    normalizeMesh();
    
    paramTexCoords.clear();
    paramTexCoords.reserve(openMesh.n_vertices() * 2);
    
    // 计算最小最大值用于归一化
    float minX = 1e9, maxX = -1e9;
    float minY = 1e9, maxY = -1e9;
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        minX = std::min(minX, p[0]);
        maxX = std::max(maxX, p[0]);
        minY = std::min(minY, p[1]);
        maxY = std::max(maxY, p[1]);
    }
    
    // 归一化到[0,1]范围
    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        float u = (p[0] - minX) / rangeX;
        float v = (p[1] - minY) / rangeY;
        paramTexCoords.push_back(u);
        paramTexCoords.push_back(v);
    }
    
    // 重新计算曲率
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    update();
}