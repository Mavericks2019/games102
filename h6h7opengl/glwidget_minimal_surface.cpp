#include "glwidget.h"
#include <vector>
#include <queue>
#include <fstream> // 用于输出到文件


void GLWidget::performCotangentWeightsIteration(int iterations, float lambda) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
        }
    }
    
    // 执行迭代
    for (int iter = 0; iter < iterations; iter++) {
        // 存储每个顶点的新位置
        std::vector<Mesh::Point> newPositions(openMesh.n_vertices());
        
        // 计算每个顶点的新位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            
            // 边界顶点保持固定
            if (isBoundary[idx]) {
                newPositions[idx] = openMesh.point(vh);
                continue;
            }
            
            Mesh::Point weightedSum(0, 0, 0);
            float totalWeight = 0.0f;
            bool validWeights = false;
            
            // 遍历顶点的出站半边
            for (auto heh : openMesh.voh_range(vh)) {
                if (!heh.is_valid()) continue;
                
                auto vj = openMesh.to_vertex_handle(heh);
                
                // 获取半边所属的面
                auto fh = openMesh.face_handle(heh);
                if (!fh.is_valid()) continue;
                
                // 获取下一个半边
                auto next_heh = openMesh.next_halfedge_handle(heh);
                auto vk = openMesh.to_vertex_handle(next_heh);
                
                // 获取对面的面（如果存在）
                auto opp_heh = openMesh.opposite_halfedge_handle(heh);
                auto opp_fh = openMesh.face_handle(opp_heh);
                
                float cot_alpha = 0.0f;
                float cot_beta = 0.0f;
                
                // 计算第一个余切值（当前面）
                cot_alpha = cotangent(openMesh.point(vk), 
                                      openMesh.point(vh), 
                                      openMesh.point(vj));
                
                // 计算第二个余切值（对面）
                if (opp_fh.is_valid()) {
                    auto opp_next_heh = openMesh.next_halfedge_handle(opp_heh);
                    auto opp_vertex = openMesh.to_vertex_handle(opp_next_heh);
                    Mesh::Point vl = openMesh.point(opp_vertex);
                    
                    cot_beta = cotangent(vl, 
                                        openMesh.point(vh), 
                                        openMesh.point(vj));
                }
                
                // 计算权重
                float weight = cot_alpha + cot_beta;
                
                // 避免负权重导致不稳定
                if (weight > 0) {
                    weightedSum += weight * openMesh.point(vj);
                    totalWeight += weight;
                    validWeights = true;
                }
            }
            
            // 计算新位置
            if (validWeights && totalWeight > EPSILON) {
                Mesh::Point centroid = weightedSum / totalWeight;
                newPositions[idx] = openMesh.point(vh) + lambda * (centroid - openMesh.point(vh));
            } else {
                // 如果无法计算有效权重，使用均匀拉普拉斯作为后备
                Mesh::Point sum(0, 0, 0);
                int count = 0;
                for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
                    sum += openMesh.point(*vv_it);
                    count++;
                }
                if (count > 0) {
                    newPositions[idx] = openMesh.point(vh) + lambda * (sum / count - openMesh.point(vh));
                } else {
                    newPositions[idx] = openMesh.point(vh);
                }
            }
        }
        
        // 更新顶点位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            openMesh.set_point(vh, newPositions[idx]);
        }
    }
    
}

void GLWidget::performUniformLaplacianIteration(int iterations, float lambda) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
        }
    }
    
    // 执行迭代
    for (int iter = 0; iter < iterations; iter++) {
        // 存储每个顶点的新位置
        std::vector<Mesh::Point> newPositions(openMesh.n_vertices());
        
        // 计算每个顶点的新位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            
            // 边界顶点保持固定
            if (isBoundary[idx]) {
                newPositions[idx] = openMesh.point(vh);
                continue;
            }
            
            // 计算邻接顶点的平均位置
            Mesh::Point sum(0, 0, 0);
            int count = 0;
            for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
                sum += openMesh.point(*vv_it);
                count++;
            }
            
            if (count > 0) {
                Mesh::Point avg = sum / count;
                newPositions[idx] = openMesh.point(vh) + lambda * (avg - openMesh.point(vh));
            } else {
                newPositions[idx] = openMesh.point(vh);
            }
        }
        
        // 更新顶点位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            openMesh.set_point(vh, newPositions[idx]);
        }
    }
}

void GLWidget::performCotangentWithAreaIteration(int iterations, float lambda) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
        }
    }
    
    // 执行迭代
    for (int iter = 0; iter < iterations; iter++) {
        // 存储每个顶点的新位置
        std::vector<Mesh::Point> newPositions(openMesh.n_vertices());
        
        // 计算每个顶点的新位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            
            // 边界顶点保持固定
            if (isBoundary[idx]) {
                newPositions[idx] = openMesh.point(vh);
                continue;
            }
            
            Mesh::Point weightedSum(0, 0, 0);
            float totalWeight = 0.0f;
            bool validWeights = false;
            
            // 计算混合面积
            float A_mixed = calculateMixedArea(vh);
            
            // 遍历顶点的出站半边
            for (auto heh : openMesh.voh_range(vh)) {
                if (!heh.is_valid()) continue;
                
                auto vj = openMesh.to_vertex_handle(heh);
                
                // 获取半边所属的面
                auto fh = openMesh.face_handle(heh);
                if (!fh.is_valid()) continue;
                
                // 获取下一个半边
                auto next_heh = openMesh.next_halfedge_handle(heh);
                auto vk = openMesh.to_vertex_handle(next_heh);
                
                // 获取对面的面（如果存在）
                auto opp_heh = openMesh.opposite_halfedge_handle(heh);
                auto opp_fh = openMesh.face_handle(opp_heh);
                
                float cot_alpha = 0.0f;
                float cot_beta = 0.0f;
                
                // 计算第一个余切值（当前面）
                cot_alpha = cotangent(openMesh.point(vk), 
                                      openMesh.point(vh), 
                                      openMesh.point(vj));
                
                // 计算第二个余切值（对面）
                if (opp_fh.is_valid()) {
                    auto opp_next_heh = openMesh.next_halfedge_handle(opp_heh);
                    auto opp_vertex = openMesh.to_vertex_handle(opp_next_heh);
                    Mesh::Point vl = openMesh.point(opp_vertex);
                    
                    cot_beta = cotangent(vl, 
                                        openMesh.point(vh), 
                                        openMesh.point(vj));
                }
                
                // 计算权重
                float weight = cot_alpha + cot_beta;
                
                // 避免负权重导致不稳定
                if (weight > 0) {
                    weightedSum += weight * openMesh.point(vj);
                    totalWeight += weight;
                    validWeights = true;
                }
            }
            
            // 计算新位置（带面积归一化）
            if (validWeights && totalWeight > EPSILON && lambda / A_mixed > 200 && A_mixed > 10 * EPSILON) {
                Mesh::Point centroid = weightedSum / totalWeight;
                // Laplace-Beltrami算子：Δf = (1/4A) * Σ(cotα + cotβ)(f_j - f_i)
                Mesh::Point displacement = (centroid - openMesh.point(vh)) / (4 * A_mixed);
                newPositions[idx] = openMesh.point(vh) + lambda * displacement;
            } else {
                // 如果无法计算有效权重，使用均匀拉普拉斯作为后备
                Mesh::Point sum(0, 0, 0);
                int count = 0;
                for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
                    sum += openMesh.point(*vv_it);
                    count++;
                }
                if (count > 0) {
                    newPositions[idx] = openMesh.point(vh) + lambda * (sum / count - openMesh.point(vh));
                } else {
                    newPositions[idx] = openMesh.point(vh);
                }
            }
        }
        
        // 更新顶点位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            openMesh.set_point(vh, newPositions[idx]);
        }
    }
}

void GLWidget::performEigenSparseSolverIteration() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    int boundaryCount = 0;
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
            boundaryCount++;
        }
    }

    // 关键检查：必须有边界顶点
    if (boundaryCount == 0) {
        std::cerr << "错误：网格没有边界顶点！无法求解极小曲面问题。" << std::endl;
        return;
    }

    // 准备数据：计算每个顶点的余切权重
    std::vector<std::map<int, float>> weights(openMesh.n_vertices());
    
    for (auto vh : openMesh.vertices()) {
        int i = vh.idx();
        if (isBoundary[i]) continue;
        
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
            
            // // 截断负权重确保数值稳定性
            // if (weight < 0) weight = 0.0f;
            weights[i][j] = weight;
        }
    }

    // 构建线性方程组
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    int n = openMesh.n_vertices();
    SpMat A(n, n);  // 系统矩阵
    VectorXf bx(n), by(n), bz(n);
    VectorXf x(n), y(n), z(n);
    
    bx.setZero();
    by.setZero();
    bz.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n * 10);  // 预分配空间
    
    // ===== 与原始代码一致的数学形式 =====
    for (int i = 0; i < n; i++) {
        if (isBoundary[i]) {
            // 边界顶点：固定位置
            triplets.push_back(Triplet(i, i, 1.0f));
            bx[i] = openMesh.point(Mesh::VertexHandle(i))[0];
            by[i] = openMesh.point(Mesh::VertexHandle(i))[1];
            bz[i] = openMesh.point(Mesh::VertexHandle(i))[2];
        } else {
            // 内部顶点：使用 -L 形式
            float totalWeight = 0.0f;
            
            // 处理邻接顶点权重
            for (const auto& [j, w] : weights[i]) {
                // A(i,j) = w_ij (正权重)
                triplets.push_back(Triplet(i, j, w));
                totalWeight += w;
            }
            
            // 对角线元素 A(i,i) = -Σw_ij
            triplets.push_back(Triplet(i, i, -totalWeight));
            
            // 方程右侧为0
            bx[i] = 0.0f;
            by[i] = 0.0f;
            bz[i] = 0.0f;
        }
    }
    
    // 设置稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();  // 优化存储格式
    
    // 使用稀疏矩阵专用求解器 - BiCGSTAB
    Eigen::BiCGSTAB<SpMat> solver;
    
    // 配置求解器参数
    solver.setMaxIterations(1000);  // 最大迭代次数
    solver.setTolerance(1e-6);      // 收敛容差
    
    // 计算预处理器（可选，但能显著加速收敛）
    // Eigen::DiagonalPreconditioner<float> precond;
    // solver.preconditioner() = &precond;
    
    // 求解三个坐标分量
    solver.compute(A);
    if (solver.info() != Success) {
        std::cerr << "矩阵分解失败! 错误代码: " << solver.info() << std::endl;
        return;
    }
    
    x = solver.solve(bx);
    if (solver.info() != Success) {
        std::cerr << "X坐标求解失败! 错误: " 
                  << (solver.info() == NoConvergence ? "未收敛" : "数值错误")
                  << ", 迭代次数: " << solver.iterations()
                  << ", 估计误差: " << solver.error() << std::endl;
    }
    
    y = solver.solve(by);
    if (solver.info() != Success) {
        std::cerr << "Y坐标求解失败! 错误: " 
                  << (solver.info() == NoConvergence ? "未收敛" : "数值错误")
                  << ", 迭代次数: " << solver.iterations()
                  << ", 估计误差: " << solver.error() << std::endl;
    }
    
    z = solver.solve(bz);
    if (solver.info() != Success) {
        std::cerr << "Z坐标求解失败! 错误: " 
                  << (solver.info() == NoConvergence ? "未收敛" : "数值错误")
                  << ", 迭代次数: " << solver.iterations()
                  << ", 估计误差: " << solver.error() << std::endl;
    }
    
    // 输出求解统计信息
    std::cout << "\n===== 求解统计 =====" << std::endl;
    std::cout << "系统规模: " << n << " 个顶点" << std::endl;
    std::cout << "边界顶点: " << boundaryCount << " 个" << std::endl;
    std::cout << "X坐标求解: 迭代 " << solver.iterations() 
              << " 次, 误差 " << solver.error() << std::endl;
    
    // 更新顶点位置
    for (int i = 0; i < n; i++) {
        if (!isBoundary[i]) {
            Mesh::Point newPos(x[i], y[i], z[i]);
            openMesh.set_point(Mesh::VertexHandle(i), newPos);
        }
    }
    
    // 更新法线和曲率
    openMesh.update_normals();
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    update();
}

void GLWidget::performMinimalSurfaceIteration(int iterations, float lambda) {
    switch (iterationMethod) {
    case UniformLaplacian:
        performUniformLaplacianIteration(iterations, lambda);
        break;
    case CotangentWeights:
        performCotangentWeightsIteration(iterations, lambda);
        break;
    case CotangentWithArea: // 新增的带面积迭代
        performCotangentWithAreaIteration(iterations, lambda);
        break;
    case EigenSparseSolver: // 新增的Eigen求解方法
        performEigenSparseSolverIteration();
        break;
    }
    
    // 更新法线
    openMesh.update_normals();
    
    // 重新计算曲率
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    update();
}