#ifndef ADJACENCYGRAPH_H
#define ADJACENCYGRAPH_H

#include <vector>
#include <cstddef> // for size_t

struct VertexAdjacency {
    std::vector<size_t> neighbors;        // 邻接顶点索引
    std::vector<size_t> adjacentFaces;    // 邻接三角形索引
};

class AdjacencyGraph {
public:
    AdjacencyGraph() = default;
    
    // 构建邻接图
    void build(const std::vector<float>& vertices, 
               const std::vector<unsigned int>& faces);
    
    // 获取邻接信息
    const std::vector<VertexAdjacency>& getAdjacency() const { return adjacency; }
    size_t vertexCount() const { return adjacency.size(); }
    
private:
    std::vector<VertexAdjacency> adjacency;
};

#endif // ADJACENCYGRAPH_H