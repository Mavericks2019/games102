#include <algorithm>
#include <cmath>
#include <map>
#include "glwidget.h"
#include <vector>
#include <queue>
#include <fstream> // 用于输出到文件

// 边界映射到圆形
void GLWidget::mapBoundaryToCircle() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 收集边界顶点
    std::vector<Mesh::VertexHandle> boundaryVertices;
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            boundaryVertices.push_back(vh);
        }
    }
    if (boundaryVertices.empty()) return;

    // 按角度排序（逆时针）
    Mesh::Point center(0, 0, 0);
    for (auto vh : boundaryVertices) {
        center += openMesh.point(vh);
    }
    center /= boundaryVertices.size();

    // 排序函数
    auto angleSort = [&](const Mesh::VertexHandle& a, const Mesh::VertexHandle& b) {
        Mesh::Point pa = openMesh.point(a) - center;
        Mesh::Point pb = openMesh.point(b) - center;
        return atan2(pa[1], pa[0]) < atan2(pb[1], pb[0]);
    };
    std::sort(boundaryVertices.begin(), boundaryVertices.end(), angleSort);

    // 映射到单位圆
    const float PI = 3.14159265358979323846f;
    for (size_t i = 0; i < boundaryVertices.size(); i++) {
        float angle = 2 * PI * i / boundaryVertices.size();
        float x = -cos(angle);
        float y = -sin(angle);
        openMesh.set_point(boundaryVertices[i], Mesh::Point(x, y, 0));
    }
}

// 边界映射到矩形
void GLWidget::mapBoundaryToRectangle() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 收集边界顶点
    std::vector<Mesh::VertexHandle> boundaryVertices;
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            boundaryVertices.push_back(vh);
        }
    }
    if (boundaryVertices.empty()) return;

    // 按角度排序（逆时针）
    Mesh::Point center(0, 0, 0);
    for (auto vh : boundaryVertices) {
        center += openMesh.point(vh);
    }
    center /= boundaryVertices.size();

    // 排序函数
    auto angleSort = [&](const Mesh::VertexHandle& a, const Mesh::VertexHandle& b) {
        Mesh::Point pa = openMesh.point(a) - center;
        Mesh::Point pb = openMesh.point(b) - center;
        return atan2(pa[1], pa[0]) < atan2(pb[1], pb[0]);
    };
    std::sort(boundaryVertices.begin(), boundaryVertices.end(), angleSort);

    // 计算边界弦长
    float totalLength = 0.0f;
    std::vector<float> lengths;
    for (size_t i = 0; i < boundaryVertices.size(); i++) {
        size_t next = (i + 1) % boundaryVertices.size();
        Mesh::Point p1 = openMesh.point(boundaryVertices[i]);
        Mesh::Point p2 = openMesh.point(boundaryVertices[next]);
        float dist = (p2 - p1).norm();
        lengths.push_back(dist);
        totalLength += dist;
    }

    // 映射到矩形边界
    int n = boundaryVertices.size();
    int sides[4] = {n/4, n/4, n/4, n/4};
    int remainder = n % 4;
    for (int i = 0; i < remainder; i++) {
        sides[i]++;
    }

    // 初始化边界点
    int index = 0;
    float accum = 0.0f;
    
    // 左边 (x=-1, y从1到-1)
    float y = 1.0f;
    for (int i = 0; i < sides[0]; i++) {
        accum += lengths[index];
        float ratio = accum / totalLength;
        float yPos = 1.0f - 2.0f * ratio * (sides[0]/float(n));
        openMesh.set_point(boundaryVertices[index], Mesh::Point(-1.0f, yPos, 0));
        index++;
    }

    // 下边 (x从-1到1, y=-1)
    float x = -1.0f;
    for (int i = 0; i < sides[1]; i++) {
        accum += lengths[index];
        float ratio = accum / totalLength;
        float xPos = -1.0f + 2.0f * ratio * (sides[1]/float(n));
        openMesh.set_point(boundaryVertices[index], Mesh::Point(xPos, -1.0f, 0));
        index++;
    }

    // 右边 (x=1, y从-1到1)
    y = -1.0f;
    for (int i = 0; i < sides[2]; i++) {
        accum += lengths[index];
        float ratio = accum / totalLength;
        float yPos = -1.0f + 2.0f * ratio * (sides[2]/float(n));
        openMesh.set_point(boundaryVertices[index], Mesh::Point(1.0f, yPos, 0));
        index++;
    }

    // 上边 (x从1到-1, y=1)
    x = 1.0f;
    for (int i = 0; i < sides[3]; i++) {
        accum += lengths[index];
        float ratio = accum / totalLength;
        float xPos = 1.0f - 2.0f * ratio * (sides[3]/float(n));
        openMesh.set_point(boundaryVertices[index], Mesh::Point(xPos, 1.0f, 0));
        index++;
    }
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
        for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
            int j = vv_it->idx();
            float weight = 0.0f;
            
            auto heh = openMesh.find_halfedge(vh, *vv_it);
            if (!heh.is_valid()) continue;
            
            auto fh = openMesh.face_handle(heh);
            auto opp_heh = openMesh.opposite_halfedge_handle(heh);
            auto opp_fh = openMesh.face_handle(opp_heh);
            
            if (fh.is_valid()) {
                auto next_heh = openMesh.next_halfedge_handle(heh);
                auto vk = openMesh.to_vertex_handle(next_heh);
                weight += cotangent(openMesh.point(vk), openMesh.point(vh), openMesh.point(*vv_it));
            }
            
            if (opp_fh.is_valid()) {
                auto opp_next_heh = openMesh.next_halfedge_handle(opp_heh);
                auto vl = openMesh.to_vertex_handle(opp_next_heh);
                weight += cotangent(openMesh.point(vl), openMesh.point(vh), openMesh.point(*vv_it));
            }
            
            weights[i][j] = weight;
        }
    }

    // 构建线性方程组 (只求解x和y坐标)
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    int n = openMesh.n_vertices();
    SpMat A(n, n);
    VectorXf bx(n), by(n);
    VectorXf x(n), y(n);
    
    bx.setZero();
    by.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n * 10);
    
    for (int i = 0; i < n; i++) {
        if (isBoundary[i]) {
            // 边界顶点：固定位置
            triplets.push_back(Triplet(i, i, 1.0f));
            bx[i] = openMesh.point(Mesh::VertexHandle(i))[0];
            by[i] = openMesh.point(Mesh::VertexHandle(i))[1];
        } else {
            // 内部顶点：使用余切权重
            float totalWeight = 0.0f;
            for (const auto& [j, w] : weights[i]) {
                triplets.push_back(Triplet(i, j, w));
                totalWeight += w;
            }
            triplets.push_back(Triplet(i, i, -totalWeight));
            bx[i] = 0.0f;
            by[i] = 0.0f;
        }
    }
    
    // 设置稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    // 使用BiCGSTAB求解器
    Eigen::BiCGSTAB<SpMat> solver;
    solver.setMaxIterations(1000);
    solver.setTolerance(1e-6);
    
    // 求解x坐标
    solver.compute(A);
    x = solver.solve(bx);
    
    // 求解y坐标
    solver.compute(A); // 重新计算分解
    y = solver.solve(by);
    
    // 更新顶点位置 (z坐标设为0)
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
    
    // 重新计算曲率
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    update();
}