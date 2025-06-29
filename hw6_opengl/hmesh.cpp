#include "hmesh.h"
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cmath>

#define EPSILON 1E-4F

// 析构函数 - 清理内存
HMesh::~HMesh() {
    for (auto v : m_vertices) delete v;
    for (auto e : m_edges) delete e;
    for (auto f : m_faces) delete f;
}

// 构建半边数据结构
void HMesh::build(const std::vector<float>& vertices, 
                 const std::vector<unsigned int>& faces) {
    // 清理已有数据
    for (auto v : m_vertices) delete v;
    for (auto e : m_edges) delete e;
    for (auto f : m_faces) delete f;
    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();
    
    // 创建顶点
    for (size_t i = 0; i < vertices.size(); i += 3) {
        HVertex* v = new HVertex();
        v->position = QVector3D(vertices[i], vertices[i+1], vertices[i+2]);
        m_vertices.push_back(v);
    }
    
    // 半边映射表 (起点索引, 终点索引) -> 半边
    std::map<std::pair<size_t, size_t>, HEdge*> edgeMap;
    
    // 处理每个面
    for (size_t i = 0; i < faces.size(); i += 3) {
        HFace* face = new HFace();
        m_faces.push_back(face);
        
        std::vector<HEdge*> faceEdges;
        std::vector<size_t> indices = { faces[i], faces[i+1], faces[i+2] };
        
        // 为当前面创建三条半边
        for (size_t j = 0; j < 3; j++) {
            HEdge* edge = new HEdge();
            m_edges.push_back(edge);
            faceEdges.push_back(edge);
            
            size_t startIdx = indices[j];
            size_t endIdx = indices[(j+1)%3];
            
            edge->vertex = m_vertices[endIdx];
            edge->face = face;
            
            // 设置顶点指向的半边
            if (m_vertices[startIdx]->edge == nullptr) {
                m_vertices[startIdx]->edge = edge;
            }
            
            // 添加到映射表
            auto key = std::make_pair(startIdx, endIdx);
            edgeMap[key] = edge;
        }
        
        // 设置面中的半边循环关系
        for (size_t j = 0; j < 3; j++) {
            faceEdges[j]->next = faceEdges[(j+1)%3];
        }
        
        // 设置面的任意一条半边
        face->edge = faceEdges[0];
    }
    
    // 设置半边的对偶关系
    for (auto& pair : edgeMap) {
        size_t start = pair.first.first;
        size_t end = pair.first.second;
        auto twinKey = std::make_pair(end, start);
        
        auto it = edgeMap.find(twinKey);
        if (it != edgeMap.end()) {
            HEdge* edge = pair.second;
            HEdge* twin = it->second;
            
            edge->twin = twin;
            twin->twin = edge;
        }
    }
}

// 计算两个向量之间的角度
float HMesh::angleBetween(const QVector3D& v1, const QVector3D& v2) {
    float dot = QVector3D::dotProduct(v1, v2);
    float len1 = v1.length();
    float len2 = v2.length();
    
    if (len1 < EPSILON || len2 < EPSILON) return 0.0f;
    
    float cosTheta = dot / (len1 * len2);
    cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);
    return std::acos(cosTheta);
}

// 计算余切值
float HMesh::cotangent(const QVector3D& v1, const QVector3D& v2) {
    float dot = QVector3D::dotProduct(v1, v2);
    QVector3D cross = QVector3D::crossProduct(v1, v2);
    float crossLength = cross.length();
    return dot / (crossLength + EPSILON);
}

// 计算顶点的混合面积
float HMesh::calculateMixedArea(HVertex* vertex) {
    float area = 0.0f;
    if (!vertex->edge) return area; // 确保起点半边有效

    HEdge* startEdge = vertex->edge;
    HEdge* edge = startEdge;
    do {
        // 跳过无效面或空指针
        if (!edge || !edge->face) {
            edge = (edge && edge->twin) ? edge->twin->next : nullptr;
            continue;
        }

        // 安全获取三角形顶点
        HEdge* e0 = edge->face->edge;
        if (!e0 || !e0->next || !e0->next->next) {
            edge = (edge->twin) ? edge->twin->next : nullptr;
            continue;
        }
        
        HEdge* e1 = e0->next;
        HEdge* e2 = e1->next;

        // 确保顶点有效
        if (!e0->vertex || !e1->vertex || !e2->vertex) {
            edge = (edge->twin) ? edge->twin->next : nullptr;
            continue;
        }
        
        QVector3D v0 = e0->vertex->position;
        QVector3D v1 = e1->vertex->position;
        QVector3D v2 = e2->vertex->position;

        // 计算三角形面积
        QVector3D edge1 = v1 - v0;
        QVector3D edge2 = v2 - v0;
        float triArea = QVector3D::crossProduct(edge1, edge2).length() / 2.0f;

        // 检查三角形类型
        float dot1 = QVector3D::dotProduct(v1 - v0, v2 - v0);
        float dot2 = QVector3D::dotProduct(v0 - v1, v2 - v1);
        float dot3 = QVector3D::dotProduct(v0 - v2, v1 - v2);

        // 直接比较顶点指针
        if (dot1 >= 0 && dot2 >= 0 && dot3 >= 0) { // 锐角三角形
            float cotA = cotangent(v1 - v0, v2 - v0);
            float cotB = cotangent(v2 - v1, v0 - v1);
            float cotC = cotangent(v0 - v2, v1 - v2);

            // 安全顶点识别
            if (vertex == e0->vertex) {
                area += (cotB + cotC) * (v1 - v0).lengthSquared() / 8.0f;
            } else if (vertex == e1->vertex) {
                area += (cotA + cotC) * (v2 - v1).lengthSquared() / 8.0f;
            } else if (vertex == e2->vertex) {
                area += (cotA + cotB) * (v0 - v2).lengthSquared() / 8.0f;
            }
        } else { // 钝角三角形
            if (dot1 < 0) { // v0处钝角
                area += (vertex == e0->vertex) ? triArea / 2.0f : triArea / 4.0f;
            } else if (dot2 < 0) { // v1处钝角
                area += (vertex == e1->vertex) ? triArea / 2.0f : triArea / 4.0f;
            } else { // v2处钝角
                area += (vertex == e2->vertex) ? triArea / 2.0f : triArea / 4.0f;
            }
        }

        // 安全移动到下一个邻接半边
        edge = (edge->twin) ? edge->twin->next : nullptr;
    } while (edge && edge != startEdge); // 确保edge有效且未回到起点

    return area;
}

// 计算网格曲率
void HMesh::calculateCurvatures() {
    // 计算每个顶点的混合面积
    std::vector<float> mixedAreas(m_vertices.size(), 0.0f);
    for (size_t i = 0; i < m_vertices.size(); i++) {
        mixedAreas[i] = calculateMixedArea(m_vertices[i]);
    }
    
    // 计算高斯曲率和平均曲率
    for (size_t i = 0; i < m_vertices.size(); i++) {
        HVertex* vertex = m_vertices[i];
        if (mixedAreas[i] < EPSILON) continue;
        
        // 计算角度和 (用于高斯曲率)
        float angleSum = 0.0f;
        QVector3D meanCurvatureVec(0, 0, 0);
        
        HEdge* startEdge = vertex->edge;
        if (!startEdge) continue; // 确保起点半边有效
        
        HEdge* edge = startEdge;
        do {
            // 跳过无效边或面
            if (!edge || !edge->face) {
                edge = (edge && edge->twin) ? edge->twin->next : nullptr;
                continue;
            }
            
            // 安全获取三角形顶点
            HEdge* e0 = edge;
            HEdge* e1 = e0->next;
            if (!e1) continue;
            
            HEdge* e2 = e1->next;
            if (!e2) continue;
            
            // 确保所有顶点有效
            if (!e0->vertex || !e1->vertex || !e2->vertex) {
                edge = (edge->twin) ? edge->twin->next : nullptr;
                continue;
            }
            
            QVector3D v0 = e0->vertex->position;
            QVector3D v1 = e1->vertex->position;
            QVector3D v2 = e2->vertex->position;
            
            // 计算角度 (使用安全顶点)
            QVector3D vec1 = v1 - v0;
            QVector3D vec2 = v2 - v0;
            angleSum += angleBetween(vec1, vec2);
            
            // 计算平均曲率向量
            QVector3D edgeVec = v1 - v0;
            QVector3D oppVec = v2 - v0;
            float cotWeight = cotangent(edgeVec, oppVec);
            meanCurvatureVec += cotWeight * edgeVec;
            
            // 安全移动到下一个邻接半边
            edge = (edge->twin) ? edge->twin->next : nullptr;
        } while (edge && edge != startEdge); // 确保edge有效且未回到起点
        
        // 计算高斯曲率
        vertex->gaussianCurvature = (2 * M_PI - angleSum) / mixedAreas[i];
        
        // 计算平均曲率
        vertex->meanCurvature = meanCurvatureVec.length() / (2.0f * mixedAreas[i]);
        
        // 计算最大曲率
        vertex->maxCurvature = vertex->gaussianCurvature + vertex->meanCurvature;
    }
    
    // 归一化曲率值到 [0,1] 范围
    auto normalize = [](std::vector<HVertex*>& vertices, float HVertex::*curvature) {
        float minVal = std::numeric_limits<float>::max();
        float maxVal = std::numeric_limits<float>::lowest();
        
        for (auto v : vertices) {
            minVal = std::min(minVal, v->*curvature);
            maxVal = std::max(maxVal, v->*curvature);
        }
        
        float range = maxVal - minVal;
        if (range > EPSILON) {
            for (auto v : vertices) {
                v->*curvature = (v->*curvature - minVal) / range;
            }
        }
    };
    
    normalize(m_vertices, &HVertex::gaussianCurvature);
    normalize(m_vertices, &HVertex::meanCurvature);
    normalize(m_vertices, &HVertex::maxCurvature);
}