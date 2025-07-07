#include "glwidget.h"
#include <map>

void GLWidget::performLoopSubdivision() {
    if (!modelLoaded) return;
    
    // 清空现有细分网格
    loopSubdividedMesh.vertices.clear();
    loopSubdividedMesh.normals.clear();
    loopSubdividedMesh.indices.clear();
    
    // 创建原始网格的顶点映射
    std::map<Mesh::VertexHandle, int> vertexMap;
    int index = 0;
    for (auto vh : openMesh.vertices()) {
        const auto& p = openMesh.point(vh);
        loopSubdividedMesh.vertices.push_back(p[0]);
        loopSubdividedMesh.vertices.push_back(p[1]);
        loopSubdividedMesh.vertices.push_back(p[2]);
        
        const auto& n = openMesh.normal(vh);
        loopSubdividedMesh.normals.push_back(n[0]);
        loopSubdividedMesh.normals.push_back(n[1]);
        loopSubdividedMesh.normals.push_back(n[2]);
        
        vertexMap[vh] = index++;
    }
    
    // 创建边到新顶点的映射
    std::map<std::pair<Mesh::VertexHandle, Mesh::VertexHandle>, int> edgeVertexMap;
    
    // 第一遍：为每条边创建新顶点
    for (auto eit = openMesh.edges_begin(); eit != openMesh.edges_end(); ++eit) {
        auto heh = openMesh.halfedge_handle(*eit, 0);
        auto v0 = openMesh.from_vertex_handle(heh);
        auto v1 = openMesh.to_vertex_handle(heh);
        
        // 获取相邻的两个顶点
        auto v2 = openMesh.to_vertex_handle(openMesh.next_halfedge_handle(heh));
        auto heh_opp = openMesh.opposite_halfedge_handle(heh);
        auto v3 = openMesh.to_vertex_handle(openMesh.next_halfedge_handle(heh_opp));
        
        // Loop细分规则：新顶点 = 3/8*(A+B) + 1/8*(C+D)
        Mesh::Point newPos = 
            0.375f * (openMesh.point(v0) + openMesh.point(v1)) +
            0.125f * (openMesh.point(v2) + openMesh.point(v3));
        
        loopSubdividedMesh.vertices.push_back(newPos[0]);
        loopSubdividedMesh.vertices.push_back(newPos[1]);
        loopSubdividedMesh.vertices.push_back(newPos[2]);
        
        // 计算新顶点的法线（平均法线）
        Mesh::Normal newNormal = 
            0.375f * (openMesh.normal(v0) + openMesh.normal(v1)) +
            0.125f * (openMesh.normal(v2) + openMesh.normal(v3));
        newNormal.normalize();
        
        loopSubdividedMesh.normals.push_back(newNormal[0]);
        loopSubdividedMesh.normals.push_back(newNormal[1]);
        loopSubdividedMesh.normals.push_back(newNormal[2]);
        
        // 存储新顶点索引
        int newIndex = loopSubdividedMesh.vertices.size() / 3 - 1;
        edgeVertexMap[std::make_pair(v0, v1)] = newIndex;
        edgeVertexMap[std::make_pair(v1, v0)] = newIndex;
    }
    
    // 第二遍：创建新面
    for (auto fit = openMesh.faces_begin(); fit != openMesh.faces_end(); ++fit) {
        // 获取面的三个顶点
        auto fv_it = openMesh.fv_ccwbegin(*fit);
        auto v0 = *fv_it; ++fv_it;
        auto v1 = *fv_it; ++fv_it;
        auto v2 = *fv_it;
        
        // 获取三条边上的新顶点
        int e0 = edgeVertexMap[std::make_pair(v0, v1)];
        int e1 = edgeVertexMap[std::make_pair(v1, v2)];
        int e2 = edgeVertexMap[std::make_pair(v2, v0)];
        
        // 原始顶点索引
        int v0_idx = vertexMap[v0];
        int v1_idx = vertexMap[v1];
        int v2_idx = vertexMap[v2];
        
        // 创建四个新三角形
        loopSubdividedMesh.indices.insert(loopSubdividedMesh.indices.end(), {v0_idx, e0, e2});
        loopSubdividedMesh.indices.insert(loopSubdividedMesh.indices.end(), {e0, v1_idx, e1});
        loopSubdividedMesh.indices.insert(loopSubdividedMesh.indices.end(), {e2, e1, v2_idx});
        loopSubdividedMesh.indices.insert(loopSubdividedMesh.indices.end(), {e0, e1, e2});
    }
    
    // 更新渲染模式
    currentRenderMode = LoopSubdivision;
    update();
}