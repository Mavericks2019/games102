#include "glwidget.h"
#include <queue>
#include <map>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>

void GLWidget::performMeshSimplification(float ratio) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 清空简化网格
    simplifiedMesh.vertices.clear();
    simplifiedMesh.normals.clear();
    simplifiedMesh.indices.clear();
    
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
        return;
    }
    
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