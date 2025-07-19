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

void GLWidget::performLoopSubdivision()
{
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 检查是否达到最大细分级别
    if (subdivisionLevel >= 3) {
        qWarning() << "Maximum subdivision level (3) reached";
        return;
    }
    
    try {
        // 创建Loop细分器
        LoopSubdivider loopSubdivider;
        
        // 附加网格到细分器
        loopSubdivider.attach(openMesh);
        
        // 执行一次细分
        if (!loopSubdivider(1)) {  // 每次只细分一级
            qCritical() << "Loop subdivision failed";
            return;
        }
        
        // 从细分器分离网格
        loopSubdivider.detach();
        
        // 增加细分级别
        subdivisionLevel++;
        
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
        
        qDebug() << "Loop subdivision applied. Current level:" << subdivisionLevel;
    } catch (const std::exception& e) {
        qCritical() << "Loop subdivision failed:" << e.what();
        return;
    }
    
    update();
}