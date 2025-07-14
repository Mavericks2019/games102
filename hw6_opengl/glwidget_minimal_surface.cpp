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
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
        }
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
            
            // 关键修正：截断负权重确保数值稳定性
            if (weight < 0) weight = 0.0f;
            weights[i][j] = weight;
        }
    }

    // 构建线性方程组 Ax = b
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    int n = openMesh.n_vertices();
    SpMat A(n, n);
    VectorXf bx(n), by(n), bz(n);
    VectorXf x(n), y(n), z(n);
    
    bx.setZero();
    by.setZero();
    bz.setZero();
    
    std::vector<Triplet> triplets;
    
    // ===== 修正矩阵构建逻辑 =====
    for (int i = 0; i < n; i++) {
        if (isBoundary[i]) {
            // 边界顶点：固定位置（Dirichlet边界条件）
            triplets.push_back(Triplet(i, i, 1.0f));
            bx[i] = openMesh.point(Mesh::VertexHandle(i))[0];
            by[i] = openMesh.point(Mesh::VertexHandle(i))[1];
            bz[i] = openMesh.point(Mesh::VertexHandle(i))[2];
        } else {
            // 内部顶点：满足拉普拉斯方程 Lx = 0
            float totalWeight = 0.0f;
            
            // 处理邻接顶点权重
            for (const auto& [j, w] : weights[i]) {
                // L(i,j) = -w_ij
                triplets.push_back(Triplet(i, j, -w));
                totalWeight += w;
            }
            
            // 对角线元素 L(i,i) = Σw_ij
            triplets.push_back(Triplet(i, i, totalWeight));
            
            // 方程右侧为0 (Lx=0)
            bx[i] = 0.0f;
            by[i] = 0.0f;
            bz[i] = 0.0f;
        }
    }
    
    // 设置稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    
    // 使用Conjugate Gradient求解器
    ConjugateGradient<SpMat, Lower|Upper> cg;
    cg.setMaxIterations(500);
    cg.setTolerance(1e-5);
    cg.compute(A);
    
    // 求解三个坐标分量
    x = cg.solve(bx);
    y = cg.solve(by);
    z = cg.solve(bz);
    
    // ===== 打印矩阵和向量信息 =====
    std::cout << "\n===== 线性系统信息 =====" << std::endl;
    std::cout << "矩阵大小: " << n << "x" << n << std::endl;
    std::cout << "非零元素数量: " << A.nonZeros() << std::endl;
    
    // 打印矩阵A的前10行和前10列
    int printSize = std::min(10, n);
    std::cout << "\n系数矩阵A (前" << printSize << "行和前" << printSize << "列):" << std::endl;
    for (int i = 0; i < printSize; i++) {
        std::cout << "行 " << i << ": ";
        for (int j = 0; j < printSize; j++) {
            float val = A.coeff(i, j);
            if (val != 0) {
                std::cout << std::setw(8) << std::setprecision(4) << val << " ";
            } else {
                std::cout << "        0 ";
            }
        }
        std::cout << std::endl;
    }
    
    // 打印右侧向量B的前10个元素
    std::cout << "\n右侧向量Bx (X坐标):" << std::endl;
    for (int i = 0; i < printSize; i++) {
        std::cout << "[" << i << "]: " << std::setw(10) << std::setprecision(6) << bx(i) << " ";
        if (i % 5 == 4) std::cout << std::endl;
    }
    std::cout << "\n右侧向量By (Y坐标):" << std::endl;
    for (int i = 0; i < printSize; i++) {
        std::cout << "[" << i << "]: " << std::setw(10) << std::setprecision(6) << by(i) << " ";
        if (i % 5 == 4) std::cout << std::endl;
    }
    std::cout << "\n右侧向量Bz (Z坐标):" << std::endl;
    for (int i = 0; i < printSize; i++) {
        std::cout << "[" << i << "]: " << std::setw(10) << std::setprecision(6) << bz(i) << " ";
        if (i % 5 == 4) std::cout << std::endl;
    }
    
    // 打印解向量X的前10个元素
    std::cout << "\n解向量X (X坐标):" << std::endl;
    for (int i = 0; i < printSize; i++) {
        std::cout << "[" << i << "]: " << std::setw(10) << std::setprecision(6) << x(i) << " ";
        if (i % 5 == 4) std::cout << std::endl;
    }
    std::cout << "\n解向量Y (Y坐标):" << std::endl;
    for (int i = 0; i < printSize; i++) {
        std::cout << "[" << i << "]: " << std::setw(10) << std::setprecision(6) << y(i) << " ";
        if (i % 5 == 4) std::cout << std::endl;
    }
    std::cout << "\n解向量Z (Z坐标):" << std::endl;
    for (int i = 0; i < printSize; i++) {
        std::cout << "[" << i << "]: " << std::setw(10) << std::setprecision(6) << z(i) << " ";
        if (i % 5 == 4) std::cout << std::endl;
    }
    
    // 打印求解信息
    std::cout << "\n求解信息:" << std::endl;
    std::cout << "迭代次数: " << cg.iterations() << std::endl;
    std::cout << "估计误差: " << cg.error() << std::endl;
    
    // 保存完整矩阵到文件（用于进一步分析）
    std::ofstream matrixFile("matrix_info.txt");
    if (matrixFile.is_open()) {
        matrixFile << "===== 完整线性系统信息 =====" << std::endl;
        matrixFile << "矩阵大小: " << n << "x" << n << std::endl;
        matrixFile << "非零元素数量: " << A.nonZeros() << std::endl;
        matrixFile << "边界点数量: " << std::count(isBoundary.begin(), isBoundary.end(), true) << std::endl;
        
        matrixFile << "\n系数矩阵A (前100x100块):" << std::endl;
        int blockSize = std::min(100, n);
        for (int i = 0; i < blockSize; i++) {
            for (int j = 0; j < blockSize; j++) {
                matrixFile << std::setw(10) << std::setprecision(5) << A.coeff(i, j) << " ";
            }
            matrixFile << std::endl;
        }
        
        matrixFile << "\n右侧向量Bx:" << std::endl;
        for (int i = 0; i < n; i++) {
            matrixFile << bx(i) << " ";
            if (i % 10 == 9) matrixFile << std::endl;
        }
        
        matrixFile << "\n右侧向量By:" << std::endl;
        for (int i = 0; i < n; i++) {
            matrixFile << by(i) << " ";
            if (i % 10 == 9) matrixFile << std::endl;
        }
        
        matrixFile << "\n右侧向量Bz:" << std::endl;
        for (int i = 0; i < n; i++) {
            matrixFile << bz(i) << " ";
            if (i % 10 == 9) matrixFile << std::endl;
        }
        
        matrixFile << "\n解向量X:" << std::endl;
        for (int i = 0; i < n; i++) {
            matrixFile << x(i) << " ";
            if (i % 10 == 9) matrixFile << std::endl;
        }
        
        matrixFile << "\n解向量Y:" << std::endl;
        for (int i = 0; i < n; i++) {
            matrixFile << y(i) << " ";
            if (i % 10 == 9) matrixFile << std::endl;
        }
        
        matrixFile << "\n解向量Z:" << std::endl;
        for (int i = 0; i < n; i++) {
            matrixFile << z(i) << " ";
            if (i % 10 == 9) matrixFile << std::endl;
        }
        
        matrixFile << "\n顶点位置变化:" << std::endl;
        for (int i = 0; i < n; i++) {
            auto oldPos = openMesh.point(Mesh::VertexHandle(i));
            Mesh::Point newPos(x(i), y(i), z(i));
            float dist = (newPos - oldPos).norm();
            matrixFile << "顶点 " << i << ": (" << oldPos[0] << ", " << oldPos[1] << ", " << oldPos[2] << ") -> "
                       << "(" << newPos[0] << ", " << newPos[1] << ", " << newPos[2] << ") 变化: " << dist << std::endl;
        }
        
        matrixFile.close();
        std::cout << "完整矩阵信息已保存到 matrix_info.txt" << std::endl;
    } else {
        std::cerr << "无法打开文件 matrix_info.txt" << std::endl;
    }
    
    std::cout << "===== 线性系统信息输出结束 =====" << std::endl;

    // 更新顶点位置
    for (int i = 0; i < n; i++) {
        if (!isBoundary[i]) {
            Mesh::Point newPos(x(i), y(i), z(i));
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