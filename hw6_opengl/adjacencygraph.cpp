#include "adjacencygraph.h"
#include <algorithm>

void AdjacencyGraph::build(const std::vector<float>& vertices, 
                           const std::vector<unsigned int>& faces) {
    size_t vertexCount = vertices.size() / 3;
    adjacency.clear();
    adjacency.resize(vertexCount);
    
    // 遍历所有面，建立邻接关系
    for (size_t i = 0; i < faces.size(); i += 3) {
        unsigned int idx1 = faces[i];
        unsigned int idx2 = faces[i+1];
        unsigned int idx3 = faces[i+2];
        size_t faceIndex = i / 3;
        
        // 添加邻接关系
        adjacency[idx1].neighbors.push_back(idx2);
        adjacency[idx1].neighbors.push_back(idx3);
        adjacency[idx1].adjacentFaces.push_back(faceIndex);
        
        adjacency[idx2].neighbors.push_back(idx1);
        adjacency[idx2].neighbors.push_back(idx3);
        adjacency[idx2].adjacentFaces.push_back(faceIndex);
        
        adjacency[idx3].neighbors.push_back(idx1);
        adjacency[idx3].neighbors.push_back(idx2);
        adjacency[idx3].adjacentFaces.push_back(faceIndex);
    }
    
    // 去除重复的邻接顶点
    for (auto& adj : adjacency) {
        std::sort(adj.neighbors.begin(), adj.neighbors.end());
        auto last = std::unique(adj.neighbors.begin(), adj.neighbors.end());
        adj.neighbors.erase(last, adj.neighbors.end());
    }
}