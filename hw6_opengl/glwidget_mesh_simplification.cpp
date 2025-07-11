#include "glwidget.h"
#include <queue>
#include <map>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>

#include "glwidget.h"
#include <queue>
#include <map>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>

void GLWidget::performMeshSimplification(float ratio) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 备份原始网格（用于可能的撤销操作）
    Mesh originalMesh = openMesh;
    
    try {
        // 创建简化器实例
        OpenMesh::Decimater::DecimaterT<Mesh> decimater(openMesh);
        
        // 添加QEM模块
        OpenMesh::Decimater::ModQuadricT<Mesh>::Handle quadricModule;
        if (!decimater.add(quadricModule)) {
            qWarning() << "Failed to add Quadric module to decimater";
            return;
        }
        
        // 初始化简化器
        if (!decimater.initialize()) {
            qWarning() << "Failed to initialize decimater";
            return;
        }
        
        // 计算目标顶点数
        size_t originalVertices = openMesh.n_vertices();
        size_t targetVertices = static_cast<size_t>(originalVertices * (1.0f - ratio));
        if (targetVertices < 4) {
            qWarning() << "Target vertices too low:" << targetVertices;
            targetVertices = 4; // 保持最小网格
        }
        
        // 执行简化
        decimater.decimate_to(targetVertices);
        
        // 垃圾收集
        openMesh.garbage_collection();
        
        // 更新法线
        openMesh.update_normals();
        
        qDebug() << "Mesh simplification completed:"
                 << "Original vertices:" << originalVertices
                 << "Target vertices:" << targetVertices
                 << "Actual vertices:" << openMesh.n_vertices();
    } catch (const std::exception& e) {
        qCritical() << "Mesh simplification failed:" << e.what();
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
    
    // 重新计算曲率（适配曲率可视化）
    calculateCurvatures();
    
    // 更新OpenGL缓冲区（适配所有渲染模式）
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    // 不改变当前渲染模式，保持一致性
    update();
}