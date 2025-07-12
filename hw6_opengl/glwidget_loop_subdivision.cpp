#include "glwidget.h"
#include <queue>
#include <map>
#include <vector>
#include <unordered_map>
#include <OpenMesh/Core/Geometry/VectorT.hh>
#include <OpenMesh/Tools/Subdivider/Uniform/LoopT.hh>

// Loop细分权重常量
const float LOOP_BETA = 3.0f / (8.0f * 3.0f); // 3/(8*valence)
const float LOOP_ALPHA = 3.0f / 8.0f;
typedef OpenMesh::Subdivider::Uniform::LoopT<Mesh> LoopSubdivider;

void GLWidget::performLoopSubdivisionOrigin() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 备份原始网格（用于多级细分）
    Mesh originalMesh = openMesh;
    
    try {
        // 执行多次细分
        for (int level = 0; level < subdivisionLevel; level++) {
            // 存储新顶点位置
            std::vector<Mesh::Point> newVertices;
            std::vector<Mesh::VertexHandle> newVertexHandles;
            
            // 1. 计算新顶点位置（边点）
            std::map<std::pair<Mesh::VertexHandle, Mesh::VertexHandle>, Mesh::VertexHandle> edgeVertices;
            
            // 遍历所有边
            for (auto eh : openMesh.edges()) {
                if (!openMesh.is_boundary(eh)) {
                    // 内部边
                    auto heh = openMesh.halfedge_handle(eh, 0);
                    auto v0 = openMesh.from_vertex_handle(heh);
                    auto v1 = openMesh.to_vertex_handle(heh);
                    
                    // 获取对面顶点
                    auto heh_opp = openMesh.opposite_halfedge_handle(heh);
                    auto v2 = openMesh.to_vertex_handle(openMesh.next_halfedge_handle(heh));
                    auto v3 = openMesh.to_vertex_handle(openMesh.next_halfedge_handle(heh_opp));
                    
                    // 计算新顶点位置 (3/8 * (A+B) + 1/8 * (C+D))
                    Mesh::Point newPos = 
                        3.0f/8.0f * (openMesh.point(v0)) + 
                        3.0f/8.0f * (openMesh.point(v1)) + 
                        1.0f/8.0f * (openMesh.point(v2)) + 
                        1.0f/8.0f * (openMesh.point(v3));
                    
                    // 添加新顶点
                    auto newVh = openMesh.add_vertex(newPos);
                    newVertices.push_back(newPos);
                    newVertexHandles.push_back(newVh);
                    
                    // 存储边到新顶点的映射
                    edgeVertices[{v0, v1}] = newVh;
                    edgeVertices[{v1, v0}] = newVh;
                } else {
                    // 边界边
                    auto heh = openMesh.halfedge_handle(eh, 0);
                    auto v0 = openMesh.from_vertex_handle(heh);
                    auto v1 = openMesh.to_vertex_handle(heh);
                    
                    // 计算边界中点
                    Mesh::Point newPos = 0.5f * (openMesh.point(v0) + 0.5f * (openMesh.point(v1)));
                    
                    // 添加新顶点
                    auto newVh = openMesh.add_vertex(newPos);
                    newVertices.push_back(newPos);
                    newVertexHandles.push_back(newVh);
                    
                    // 存储边到新顶点的映射
                    edgeVertices[{v0, v1}] = newVh;
                    edgeVertices[{v1, v0}] = newVh;
                }
            }
            
            // 2. 更新旧顶点位置
            std::vector<Mesh::Point> updatedPositions(openMesh.n_vertices());
            for (auto vh : openMesh.vertices()) {
                if (!openMesh.is_boundary(vh)) {
                    // 内部顶点
                    int n = openMesh.valence(vh);
                    float u = (n == 3) ? 3.0f/16.0f : 3.0f/(8.0f * n);
                    
                    Mesh::Point sum(0.0f, 0.0f, 0.0f);
                    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
                        sum += openMesh.point(*vv_it);
                    }
                    
                    updatedPositions[vh.idx()] = 
                        (1.0f - n * u) * openMesh.point(vh) + 
                        u * sum;
                } else {
                    // 边界顶点
                    std::vector<Mesh::VertexHandle> boundaryNeighbors;
                    for (auto voh_it = openMesh.voh_begin(vh); voh_it != openMesh.voh_end(vh); ++voh_it) {
                        if (openMesh.is_boundary(openMesh.edge_handle(*voh_it))) {
                            boundaryNeighbors.push_back(openMesh.to_vertex_handle(*voh_it));
                        }
                    }
                    
                    if (boundaryNeighbors.size() == 2) {
                        updatedPositions[vh.idx()] = 
                            0.75f * openMesh.point(vh) + 
                            0.125f * openMesh.point(boundaryNeighbors[0]) + 
                            0.125f * openMesh.point(boundaryNeighbors[1]);
                    } else {
                        updatedPositions[vh.idx()] = openMesh.point(vh);
                    }
                }
            }
            
            // 应用旧顶点更新
            for (auto vh : openMesh.vertices()) {
                openMesh.set_point(vh, updatedPositions[vh.idx()]);
            }
            
            // 3. 重新连接面
            std::vector<Mesh::FaceHandle> facesToDelete;
            for (auto fh : openMesh.faces()) {
                facesToDelete.push_back(fh);
                
                // 获取面的三个顶点
                auto fv_it = openMesh.fv_ccwbegin(fh);
                Mesh::VertexHandle v0 = *fv_it; ++fv_it;
                Mesh::VertexHandle v1 = *fv_it; ++fv_it;
                Mesh::VertexHandle v2 = *fv_it;
                
                // 获取三条边上的新顶点
                Mesh::VertexHandle e0 = edgeVertices[{v0, v1}];
                Mesh::VertexHandle e1 = edgeVertices[{v1, v2}];
                Mesh::VertexHandle e2 = edgeVertices[{v2, v0}];
                
                // 创建四个新三角形
                std::vector<Mesh::VertexHandle> face1 = {v0, e0, e2};
                std::vector<Mesh::VertexHandle> face2 = {e0, v1, e1};
                std::vector<Mesh::VertexHandle> face3 = {e2, e1, v2};
                std::vector<Mesh::VertexHandle> face4 = {e0, e1, e2};
                
                openMesh.add_face(face1);
                openMesh.add_face(face2);
                openMesh.add_face(face3);
                openMesh.add_face(face4);
            }
            
            // 删除旧面
            for (auto fh : facesToDelete) {
                openMesh.delete_face(fh, false);
            }
            
            // 垃圾收集
            openMesh.garbage_collection();
        }
        
        qDebug() << "Loop subdivision completed: Levels =" << subdivisionLevel
                 << "New vertices:" << openMesh.n_vertices()
                 << "New faces:" << openMesh.n_faces();
    } catch (const std::exception& e) {
        qCritical() << "Loop subdivision failed:" << e.what();
        openMesh = originalMesh; // 恢复原始网格
        return;
    }
    
    // 更新边索引（用于线框渲染）
    edges.clear();
    for (auto eh : openMesh.edges()) {
        auto heh = openMesh.halfedge_handle(eh, 0);
        auto v0 = openMesh.from_vertex_handle(heh);
        auto v1 = openMesh.to_vertex_handle(heh);
        edges.push_back(v0.idx());
        edges.push_back(v1.idx());
    }
    
    // 更新面索引（用于三角面渲染）
    faces.clear();
    for (auto fh : openMesh.faces()) {
        for (auto fv_it = openMesh.fv_ccwbegin(fh); fv_it != openMesh.fv_ccwend(fh); ++fv_it) {
            faces.push_back(fv_it->idx());
        }
    }
    
    // 更新法线
    openMesh.update_normals();
    
    // 重新计算曲率
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    // 切换到细分渲染模式
    currentRenderMode = LoopSubdivision;
    update();
}

void GLWidget::performLoopSubdivision() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    try {
        // 创建Loop细分器
        LoopSubdivider loopSubdivider;
        
        // 附加网格到细分器
        loopSubdivider.attach(openMesh);
        
        // 执行细分（支持多级细分）
        if (!loopSubdivider(subdivisionLevel)) {
            qCritical() << "Loop subdivision failed";
            return;
        }
        
        // 从细分器分离网格
        loopSubdivider.detach();
        
        // qDebug() << "Loop subdivision completed: Levels =" << subdivisionLevel
        //          << "New vertices:" << openMesh.n_vertices()
        //          << "New faces:" << openMesh.n_faces();
    } catch (const std::exception& e) {
        qCritical() << "Loop subdivision failed:" << e.what();
        return;
    }
    
    // 更新边索引（用于线框渲染）
    edges.clear();
    for (auto eh : openMesh.edges()) {
        auto heh = openMesh.halfedge_handle(eh, 0);
        auto v0 = openMesh.from_vertex_handle(heh);
        auto v1 = openMesh.to_vertex_handle(heh);
        edges.push_back(v0.idx());
        edges.push_back(v1.idx());
    }
    
    // 更新面索引（用于三角面渲染）
    faces.clear();
    for (auto fh : openMesh.faces()) {
        for (auto fv_it = openMesh.fv_ccwbegin(fh); fv_it != openMesh.fv_ccwend(fh); ++fv_it) {
            faces.push_back(fv_it->idx());
        }
    }
    
    // 更新法线
    openMesh.update_normals();
    
    // 重新计算曲率
    calculateCurvatures();
}