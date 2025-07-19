#include "glwidget.h"
#include <QFile>
#include <QDebug>
#include <OpenMesh/Core/IO/MeshIO.hh>

// 清除当前网格数据
void GLWidget::clearMeshData()
{
    openMesh.clear();
    faces.clear();
    edges.clear();
    modelLoaded = false;
}

// 加载OBJ文件到OpenMesh
bool GLWidget::loadOBJToOpenMesh(const QString &path)
{
    OpenMesh::IO::Options opt = OpenMesh::IO::Options::Default;
    return OpenMesh::IO::read_mesh(openMesh, path.toStdString(), opt);
}

// 计算网格的边界框
void GLWidget::computeBoundingBox(Mesh::Point& min, Mesh::Point& max)
{
    if (openMesh.n_vertices() > 0) {
        min = max = openMesh.point(*openMesh.vertices_begin());
        for (auto vh : openMesh.vertices()) {
            min.minimize(openMesh.point(vh));
            max.maximize(openMesh.point(vh));
        }
    }
}

// 中心化并缩放网格
void GLWidget::centerAndScaleMesh(const Mesh::Point& center, float maxSize)
{
    float scaleFactor = 2.0f / maxSize;
    for (auto vh : openMesh.vertices()) {
        Mesh::Point p = openMesh.point(vh);
        p = (p - center) * scaleFactor;
        openMesh.set_point(vh, p);
    }
}

// 准备面索引数据（包括三角剖分）
void GLWidget::prepareFaceIndices()
{
    for (auto fh : openMesh.faces()) {
        auto fv_it = openMesh.fv_ccwbegin(fh);
        auto fv_end = openMesh.fv_ccwend(fh);
        int vertexCount = openMesh.valence(fh);
        
        if (vertexCount < 3) {
            qWarning() << "Face with less than 3 vertices, skipping";
            continue;
        }
        
        if (vertexCount == 3) {
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx());
        }
        else {
            unsigned int centerIdx = (*fv_it).idx();
            ++fv_it;
            unsigned int prevIdx = (*fv_it).idx();
            ++fv_it;
            
            for (int i = 2; i < vertexCount; i++) {
                unsigned int currentIdx = (*fv_it).idx();
                faces.push_back(centerIdx);
                faces.push_back(prevIdx);
                faces.push_back(currentIdx);
                prevIdx = currentIdx;
                ++fv_it;
            }
        }
    }
}

// 准备边索引数据
void GLWidget::prepareEdgeIndices()
{
    std::set<std::pair<unsigned int, unsigned int>> uniqueEdges;
    for (auto heh : openMesh.halfedges()) {
        if (openMesh.is_boundary(heh) || heh.idx() < openMesh.opposite_halfedge_handle(heh).idx()) {
            unsigned int from = openMesh.from_vertex_handle(heh).idx();
            unsigned int to = openMesh.to_vertex_handle(heh).idx();
            
            if (from > to) std::swap(from, to);
            uniqueEdges.insert({from, to});
        }
    }
    
    for (const auto& edge : uniqueEdges) {
        edges.push_back(edge.first);
        edges.push_back(edge.second);
    }
}

// 保存原始网格状态
void GLWidget::saveOriginalMesh()
{
    originalMesh = openMesh;
    hasOriginalMesh = true;
    subdivisionLevel = 0;
}

// 主加载函数
void GLWidget::loadOBJ(const QString &path)
{
    // 1. 清除旧数据
    clearMeshData();
    // 2. 加载OBJ文件
    if (!loadOBJToOpenMesh(path)) {
        qWarning() << "Failed to load mesh:" << path;
        return;
    }
    
    // 3. 计算边界框
    Mesh::Point min, max;
    computeBoundingBox(min, max);
    
    // 4. 中心化和缩放
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    centerAndScaleMesh(center, maxSize);
    
    // 5. 更新网格属性
    openMesh.request_vertex_normals();
    openMesh.request_face_normals();
    openMesh.update_normals();
    
    // 6. 准备渲染数据
    prepareFaceIndices();
    prepareEdgeIndices();
    
    // 7. 计算曲率
    calculateCurvatures();
    
    // 8. 保存状态
    saveOriginalMesh();
    modelLoaded = true;
    
    // 9. 更新UI和渲染
    qDebug() << "Loaded OBJ file:" << path
             << "\nVertices:" << openMesh.n_vertices()
             << "Faces:" << openMesh.n_faces()
             << "\nEdges:" << edges.size() / 2
             << "\nModel center:" << center[0] << "," << center[1] << "," << center[2]
             << "\nModel size:" << maxSize;
    
    makeCurrent();
    initializeShaders();
    doneCurrent();
    
    rotationX = rotationY = 0;
    zoom = 1.0f;
    update();
}

void GLWidget::resetMeshOperation()
{
    if (!hasOriginalMesh) return;
    
    // 恢复原始网格
    openMesh = originalMesh;
    
    // 更新网格数据
    meshOperationValue = 0;
    subdivisionLevel = 0;    
    performMeshSimplification(0);
    updateBuffersFromOpenMesh();

    update();
}

void GLWidget::applyMeshOperation(int sliderValue)
{
    if (!hasOriginalMesh) return;
    
    // 保存新值
    meshOperationValue = sliderValue;
    
    // 恢复原始网格
    openMesh = originalMesh;
    
    float ratio = sliderValue / 100.0f;
    performMeshSimplification(ratio);
    
    // 更新网格数据
    updateBuffersFromOpenMesh();
    update();
}