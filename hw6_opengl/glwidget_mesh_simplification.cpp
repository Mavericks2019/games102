#include "glwidget.h"
#include <queue>
#include <map>

void GLWidget::performMeshSimplification(float ratio) {
    if (!modelLoaded) return;
    
    // 清空简化网格
    simplifiedMesh.vertices.clear();
    simplifiedMesh.normals.clear();
    simplifiedMesh.indices.clear();
    
    // 计算要移除的边数量
    int targetEdges = openMesh.n_edges() * ratio;
    
    // 使用二次误差度量进行简化
    struct EdgeCost {
        Mesh::EdgeHandle eh;
        double cost;
        Mesh::Point newPos;
        
        bool operator<(const EdgeCost& other) const {
            return cost > other.cost; // 最小堆
        }
    };
    
    std::priority_queue<EdgeCost> edgeQueue;
    
    // 初始化边成本
    for (auto eit = openMesh.edges_begin(); eit != openMesh.edges_end(); ++eit) {
        // 跳过边界边
        if (openMesh.is_boundary(*eit)) continue;
        
        auto heh = openMesh.halfedge_handle(*eit, 0);
        auto v0 = openMesh.from_vertex_handle(heh);
        auto v1 = openMesh.to_vertex_handle(heh);
        
        // 计算新顶点位置（中点）
        Mesh::Point newPos = 0.5 * (openMesh.point(v0) + openMesh.point(v1));
        
        // 计算二次误差
        double cost = (openMesh.point(v0) - newPos).sqrnorm() + 
                     (openMesh.point(v1) - newPos).sqrnorm();
        
        edgeQueue.push({*eit, cost, newPos});
    }
    
    // 执行简化
    int edgesRemoved = 0;
    while (!edgeQueue.empty() && edgesRemoved < targetEdges) {
        EdgeCost ec = edgeQueue.top();
        edgeQueue.pop();
        
        // 检查边是否仍然存在
        if (!openMesh.status(ec.eh).deleted()) {
            // 获取边对应的半边
            auto heh = openMesh.halfedge_handle(ec.eh, 0);
            auto v0 = openMesh.from_vertex_handle(heh);
            auto v1 = openMesh.to_vertex_handle(heh);
            
            // 检查是否可以安全折叠
            if (!openMesh.is_collapse_ok(heh)) continue;
            
            // 设置目标顶点位置为中点
            openMesh.set_point(v1, ec.newPos);
            
            // 执行边折叠操作 (移除v0，保留v1)
            openMesh.collapse(heh);
            
            edgesRemoved++;
        }
    }
    
    // 垃圾收集
    openMesh.garbage_collection();
    
    // 更新法线
    openMesh.update_normals();
    
    // 准备简化后的网格数据
    for (auto vh : openMesh.vertices()) {
        const auto& p = openMesh.point(vh);
        simplifiedMesh.vertices.push_back(p[0]);
        simplifiedMesh.vertices.push_back(p[1]);
        simplifiedMesh.vertices.push_back(p[2]);
        
        const auto& n = openMesh.normal(vh);
        simplifiedMesh.normals.push_back(n[0]);
        simplifiedMesh.normals.push_back(n[1]);
        simplifiedMesh.normals.push_back(n[2]);
    }
    
    for (auto fh : openMesh.faces()) {
        for (auto fv_it = openMesh.fv_ccwbegin(fh); fv_it != openMesh.fv_ccwend(fh); ++fv_it) {
            simplifiedMesh.indices.push_back(fv_it->idx());
        }
    }
    
    // 更新渲染模式
    currentRenderMode = MeshSimplification;
    update();
}