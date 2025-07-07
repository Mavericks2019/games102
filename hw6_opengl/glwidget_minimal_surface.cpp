#include "glwidget.h"
#include <vector>
#include <queue>

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