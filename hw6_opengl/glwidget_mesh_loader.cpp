#include "glwidget.h"
#include <QFile>
#include <QDebug>
#include <OpenMesh/Core/IO/MeshIO.hh>

void GLWidget::loadOBJ(const QString &path)
{
    openMesh.clear();
    faces.clear();
    edges.clear();
    modelLoaded = false;
    
    OpenMesh::IO::Options opt = OpenMesh::IO::Options::Default;
    
    // 使用OpenMesh加载OBJ文件
    if (!OpenMesh::IO::read_mesh(openMesh, path.toStdString(), opt)) {
        qWarning() << "Failed to load mesh:" << path;
        return;
    }
    
    // 请求法线属性
    openMesh.request_vertex_normals();
    openMesh.request_face_normals();
    
    // 计算边界框
    Mesh::Point min, max;
    if (openMesh.n_vertices() > 0) {
        min = max = openMesh.point(*openMesh.vertices_begin());
        for (auto vh : openMesh.vertices()) {
            min.minimize(openMesh.point(vh));
            max.maximize(openMesh.point(vh));
        }
    }
    
    // 计算中心点和缩放因子
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    float scaleFactor = 2.0f / maxSize;
    
    // 应用中心化和缩放
    for (auto vh : openMesh.vertices()) {
        Mesh::Point p = openMesh.point(vh);
        p = (p - center) * scaleFactor;
        openMesh.set_point(vh, p);
    }
    
    // 更新法线
    openMesh.update_normals();
    
    // 计算曲率
    calculateCurvatures();
    
    // 准备面索引数据 - 确保每个面提取3个顶点
    for (auto fh : openMesh.faces()) {
        // 获取面的顶点迭代器
        auto fv_it = openMesh.fv_ccwbegin(fh);
        auto fv_end = openMesh.fv_ccwend(fh);
        
        // 获取面顶点的数量
        int vertexCount = openMesh.valence(fh);
        
        if (vertexCount < 3) {
            qWarning() << "Face with less than 3 vertices, skipping";
            continue;
        }
        
        // 对于三角形面，直接添加三个顶点
        if (vertexCount == 3) {
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx());
        }
        // 对于多边形面，进行三角剖分（扇形）
        else {
            // 第一个顶点作为中心点
            unsigned int centerIdx = (*fv_it).idx();
            ++fv_it;
            
            // 第二个顶点作为起始点
            unsigned int prevIdx = (*fv_it).idx();
            ++fv_it;
            
            // 遍历剩余顶点，形成三角形扇
            for (int i = 2; i < vertexCount; i++) {
                unsigned int currentIdx = (*fv_it).idx();
                
                // 添加一个三角形：中心点、前一个点、当前点
                faces.push_back(centerIdx);
                faces.push_back(prevIdx);
                faces.push_back(currentIdx);
                
                // 更新前一个点为当前点，为下一个三角形做准备
                prevIdx = currentIdx;
                
                // 移动到下一个顶点
                ++fv_it;
            }
        }
    }
    
    // 准备边索引数据 - 使用半边确保方向一致
    std::set<std::pair<unsigned int, unsigned int>> uniqueEdges;
    for (auto heh : openMesh.halfedges()) {
        if (openMesh.is_boundary(heh) || heh.idx() < openMesh.opposite_halfedge_handle(heh).idx()) {
            unsigned int from = openMesh.from_vertex_handle(heh).idx();
            unsigned int to = openMesh.to_vertex_handle(heh).idx();
            
            // 确保小索引在前
            if (from > to) std::swap(from, to);
            
            uniqueEdges.insert({from, to});
        }
    }
    
    for (const auto& edge : uniqueEdges) {
        edges.push_back(edge.first);
        edges.push_back(edge.second);
    }
    
    qDebug() << "Loaded OBJ file:" << path;
    qDebug() << "Vertices:" << openMesh.n_vertices() << "Faces:" << openMesh.n_faces();
    qDebug() << "Edges:" << edges.size() / 2;
    qDebug() << "Model center:" << center[0] << "," << center[1] << "," << center[2];
    qDebug() << "Model size:" << maxSize;
    
    modelLoaded = true;
    
    // 更新OpenGL缓冲区
    makeCurrent();
    initializeShaders();  // 重新初始化着色器和缓冲区
    doneCurrent();
    
    // 重置视图参数
    rotationX = rotationY = 0;
    zoom = 1.0f;
    
    update(); // 触发重绘
}