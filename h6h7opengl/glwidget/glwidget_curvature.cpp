#include "glwidget.h"
#include <cmath>
#include <algorithm>


void GLWidget::calculateCurvatures()
{
    if (openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
            openMesh.data(vh).curvature = 0.0f; // 边界顶点曲率设为0
            continue;
        }
        
        // 计算高斯曲率
        float angleDefect = 2 * M_PI;
        float area = 0.0f;
        
        for (auto vf_it = openMesh.vf_begin(vh); vf_it != openMesh.vf_end(vh); ++vf_it) {
            // 计算角度
            auto heh = openMesh.halfedge_handle(*vf_it);
            while (openMesh.to_vertex_handle(heh) != vh) {
                heh = openMesh.next_halfedge_handle(heh);
            }
            
            auto v1 = openMesh.point(openMesh.from_vertex_handle(heh));
            auto v2 = openMesh.point(openMesh.to_vertex_handle(heh));
            auto heh_next = openMesh.next_halfedge_handle(heh);
            auto v3 = openMesh.point(openMesh.to_vertex_handle(heh_next));
            
            auto vec1 = (v1 - v2).normalize();
            auto vec2 = (v3 - v2).normalize();
            float angle = acos(std::max(-1.0f, std::min(1.0f, dot(vec1, vec2))));
            angleDefect -= angle;
            
            // 计算三角形面积
            auto cross = (v1 - v2) % (v3 - v2);
            area += cross.length() / 6.0f;
        }
        
        float gaussianCurvature = 0.0f;
        if (area > EPSILON) {
            gaussianCurvature = angleDefect / area;
        }
        
        // 计算混合面积用于平均曲率
        float A_mixed = calculateMixedArea(vh);
        
        // 计算平均曲率向量
        Mesh::Point H = computeMeanCurvatureVector(vh);
        
        // 平均曲率值是曲率向量长度的一半
        float meanCurvature = H.length() / 2.0f;
        
        // 计算最大曲率
        float maxCurvature = gaussianCurvature + meanCurvature;
        
        // 根据当前渲染模式设置曲率
        switch (currentRenderMode) {
        case GaussianCurvature:
            openMesh.data(vh).curvature = gaussianCurvature;
            break;
        case MeanCurvature:
            openMesh.data(vh).curvature = meanCurvature;
            break;
        case MaxCurvature:
            openMesh.data(vh).curvature = maxCurvature;
            break;
        default:
            openMesh.data(vh).curvature = 0.0f;
            break;
        }
    }
    
    // 归一化曲率值到 [0,1] 范围
    auto normalize = [](std::vector<float>& values) {
        if (values.empty()) return;
        
        auto minmax = std::minmax_element(values.begin(), values.end());
        float minVal = *minmax.first;
        float maxVal = *minmax.second;
        float range = maxVal - minVal;
        
        if (range > 0) {
            for (float& val : values) {
                val = (val - minVal) / range;
            }
        }
    };
    
    // 只归一化非边界顶点
    std::vector<float> curvatures;
    for (auto vh : openMesh.vertices()) {
        if (!isBoundary[vh.idx()]) {
            curvatures.push_back(openMesh.data(vh).curvature);
        }
    }
    
    // 归一化非边界顶点
    normalize(curvatures);
    
    // 将归一化值复制回顶点
    size_t idx = 0;
    for (auto vh : openMesh.vertices()) {
        if (!isBoundary[vh.idx()]) {
            openMesh.data(vh).curvature = curvatures[idx++];
        }
        // 边界顶点保持为0
    }
}

Mesh::Point GLWidget::computeMeanCurvatureVector(const Mesh::VertexHandle& vh) {
    Mesh::Point H(0, 0, 0);
    if(openMesh.is_boundary(vh)) {
        return H;
    }
    float A_mixed = calculateMixedArea(vh);
    if (A_mixed < EPSILON) {
        return H;
    }
    // 遍历邻接顶点
    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
        auto adjV = *vv_it;
        
        // 找到前一个顶点(pp)和后一个顶点(np)
        Mesh::VertexHandle pp, np;
        auto heh = openMesh.find_halfedge(vh, adjV);
        if (!heh.is_valid()) continue;
        
        // 获取前一个顶点(pp)
        if (!openMesh.is_boundary(heh)) {
            auto prev_heh = openMesh.prev_halfedge_handle(heh);
            pp = openMesh.from_vertex_handle(prev_heh);
        } else {
            auto opp_heh = openMesh.opposite_halfedge_handle(heh);
            if (!openMesh.is_boundary(opp_heh)) {
                auto prev_opp_heh = openMesh.prev_halfedge_handle(opp_heh);
                pp = openMesh.from_vertex_handle(prev_opp_heh);
            } else {
                continue; // 无效边界
            }
        }
        
        // 获取后一个顶点(np)
        if (!openMesh.is_boundary(heh)) {
            auto next_heh = openMesh.next_halfedge_handle(heh);
            np = openMesh.to_vertex_handle(next_heh);
        } else {
            auto opp_heh = openMesh.opposite_halfedge_handle(heh);
            if (!openMesh.is_boundary(opp_heh)) {
                auto next_opp_heh = openMesh.next_halfedge_handle(opp_heh);
                np = openMesh.to_vertex_handle(next_opp_heh);
            } else {
                continue; // 无效边界
            }
        }
        
        // 计算两个相邻三角形的面积
        float area1 = triangleArea(openMesh.point(vh), openMesh.point(adjV), openMesh.point(pp));
        float area2 = triangleArea(openMesh.point(vh), openMesh.point(adjV), openMesh.point(np));
        
        if (area1 > EPSILON && area2 > EPSILON) {
            // 计算余切权重
            auto vec1 = openMesh.point(adjV) - openMesh.point(pp);
            auto vec2 = openMesh.point(vh) - openMesh.point(pp);
            float cot_alpha = dot(vec1, vec2) / cross(vec1, vec2).length();
            
            auto vec3 = openMesh.point(adjV) - openMesh.point(np);
            auto vec4 = openMesh.point(vh) - openMesh.point(np);
            float cot_beta = dot(vec3, vec4) / cross(vec3, vec4).length();
            
            H += (cot_alpha + cot_beta) * (openMesh.point(vh) - openMesh.point(adjV));
        }
    }
    
    return H / (2.0f * A_mixed);
}

float GLWidget::triangleArea(const Mesh::Point& p0, const Mesh::Point& p1, const Mesh::Point& p2) {
    auto e1 = p1 - p0;
    auto e2 = p2 - p0;
    auto cross = e1 % e2;
    return cross.length() / 2.0f;
}

// 计算混合面积（参考GetAmixed函数）
float GLWidget::calculateMixedArea(const Mesh::VertexHandle& vh) {
    float A_mixed = 0.f;

    // 遍历邻接顶点（参考GetAmixed中的adjV）
    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
        auto adjV = *vv_it;
        
        // 找到共享边的下一个顶点np（参考GetAmixed中的np）
        Mesh::VertexHandle np;
        auto heh = openMesh.find_halfedge(vh, adjV);
        if (!heh.is_valid()) continue;
        
        if (!openMesh.is_boundary(heh)) {
            // 获取同一面上的下一个顶点
            auto next_heh = openMesh.next_halfedge_handle(heh);
            np = openMesh.to_vertex_handle(next_heh);
        } else {
            // 边界边处理：获取对边上的顶点
            auto opp_heh = openMesh.opposite_halfedge_handle(heh);
            if (!openMesh.is_boundary(opp_heh)) {
                auto next_opp_heh = openMesh.next_halfedge_handle(opp_heh);
                np = openMesh.to_vertex_handle(next_opp_heh);
            } else {
                continue; // 无效边界
            }
        }

        auto p_v = openMesh.point(vh);
        auto p_adjV = openMesh.point(adjV);
        auto p_np = openMesh.point(np);
        
        // 计算向量
        auto vec_adjV = p_adjV - p_v;
        auto vec_np = p_np - p_v;
        auto vec_adjV_np = p_np - p_adjV;
        auto vec_np_adjV = p_adjV - p_np;
        
        // 检查是否非钝角三角形（点积>=0）
        bool nonObtuse = 
            (vec_adjV | vec_np) >= 0.0f &&
            (p_v - p_adjV | p_np - p_adjV) >= 0.0f &&
            (p_v - p_np | p_adjV - p_np) >= 0.0f;
        
        if (nonObtuse) {
            // 锐角三角形处理
            float area = triangleArea(p_v, p_adjV, p_np);
            if (area > EPSILON) {
                // 计算余切权重
                float cotA = dot(vec_adjV, vec_np) / cross(vec_adjV, vec_np).length();
                float cotB = dot(vec_np, vec_adjV) / cross(vec_np, vec_adjV).length();
                
                // 计算距离平方
                float dist2_adjV = vec_adjV.sqrnorm();
                float dist2_np = vec_np.sqrnorm();
                
                A_mixed += (dist2_adjV * cotB + dist2_np * cotA) / 8.0f;
            }
        } else {
            // 钝角三角形处理
            float area = triangleArea(p_v, p_adjV, p_np);
            if (area > EPSILON) {
                // 检查顶点vh处的角是否钝角
                if ((vec_adjV | vec_np) < 0.0f) {
                    A_mixed += area / 2.0f;
                } else {
                    A_mixed += area / 4.0f;
                }
            }
        }
    }
    return A_mixed;
}

// 辅助函数：计算三角形中某个角的余切值
float GLWidget::cotangent(const Mesh::Point& a, const Mesh::Point& b, const Mesh::Point& c) {
    Mesh::Point vec1 = b - a;
    Mesh::Point vec2 = c - a;
    float dotProduct = vec1 | vec2;
    float crossNorm = (vec1 % vec2).norm();
    
    // 避免除以零
    if (fabs(crossNorm) < EPSILON) {
        return 0.0f;
    }
    
    return dotProduct / crossNorm;
}