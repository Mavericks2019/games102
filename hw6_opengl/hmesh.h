#ifndef HMESH_H
#define HMESH_H

#include <vector>
#include <map>
#include <QVector3D>
#include <cmath>

struct HVertex;
struct HEdge;
struct HFace;

// 半边数据结构中的顶点
struct HVertex {
    QVector3D position;      // 顶点位置
    HEdge* edge;             // 从该顶点出发的一条半边
    float gaussianCurvature; // 高斯曲率
    float meanCurvature;     // 平均曲率
    float maxCurvature;      // 最大曲率
    
    HVertex() : edge(nullptr), gaussianCurvature(0.0f), 
                meanCurvature(0.0f), maxCurvature(0.0f) {}
};

// 半边数据结构中的半边
struct HEdge {
    HVertex* vertex;   // 半边指向的顶点
    HFace* face;       // 半边所属的面
    HEdge* twin;       // 对偶的半边
    HEdge* next;       // 同一面中的下一条半边
    
    HEdge() : vertex(nullptr), face(nullptr), 
              twin(nullptr), next(nullptr) {}
};

// 半边数据结构中的面
struct HFace {
    HEdge* edge;  // 属于该面的一条半边
    
    HFace() : edge(nullptr) {}
};

// 半边网格数据结构
class HMesh {
public:
    HMesh() = default;
    ~HMesh();
    
    // 从顶点和面数据构建半边结构
    void build(const std::vector<float>& vertices, 
               const std::vector<unsigned int>& faces);
    
    // 计算网格曲率
    void calculateCurvatures();
    
    // 获取顶点曲率数据
    const std::vector<HVertex*>& getVertices() const { return m_vertices; }
    
private:
    // 计算顶点的混合面积
    float calculateMixedArea(HVertex* vertex);
    
    // 计算两个向量之间的角度
    float angleBetween(const QVector3D& v1, const QVector3D& v2);
    
    // 计算余切值
    float cotangent(const QVector3D& v1, const QVector3D& v2);
    
private:
    std::vector<HVertex*> m_vertices;  // 所有顶点
    std::vector<HEdge*>   m_edges;     // 所有半边
    std::vector<HFace*>   m_faces;     // 所有面
};

#endif // HMESH_H