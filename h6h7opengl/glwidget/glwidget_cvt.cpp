#include "glwidget.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>
#include <algorithm>
#include <cmath>
#include <ctime> // 包含ctime用于srand
// CGAL 类型定义
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2<K> DelaunayTriangulation;
typedef CGAL::Delaunay_triangulation_adaptation_traits_2<DelaunayTriangulation> AT;
typedef CGAL::Delaunay_triangulation_caching_degeneracy_removal_policy_2<DelaunayTriangulation> AP;
typedef CGAL::Voronoi_diagram_2<DelaunayTriangulation, AT, AP> VoronoiDiagram;
typedef K::Point_2 Point_2;
typedef VoronoiDiagram::Locate_result Locate_result;
typedef VoronoiDiagram::Face_handle Face_handle;
typedef VoronoiDiagram::Ccb_halfedge_circulator Ccb_halfedge_circulator;


void GLWidget::generateRandomPoints(int count)
{
    canvasData.points.clear(); // 使用CanvasData存储点

    canvasData.points.push_back(Point(-1.0f, -1.0f));
    canvasData.points.push_back(Point(1.0f, -1.0f));
    canvasData.points.push_back(Point(-1.0f, 1.0f));
    canvasData.points.push_back(Point(1.0f, 1.0f));

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (int i = 0; i < count; i++) {
        float x = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
        float y = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
        canvasData.points.push_back(Point(x, y)); // 存入CanvasData
    }
    canvasData.dt = Delaunay(canvasData.points.begin(), canvasData.points.end());
    currentPointCount = count + 4;
    
    // 准备点数据
    std::vector<float> points;
    points.reserve(canvasData.points.size() * 2);
    for (const auto& point : canvasData.points) {
        points.push_back(point.x());
        points.push_back(point.y());
    }
    
    // 更新点缓冲区
    makeCurrent();
    
    pointVao.bind();
    pointVbo.bind();
    pointVbo.allocate(points.data(), static_cast<int>(points.size() * sizeof(float)));
    
    // 设置顶点属性
    pointProgram.bind();
    int posLoc = pointProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        pointProgram.enableAttributeArray(posLoc);
        pointProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    }
    
    pointVao.release();
    pointVbo.release();
    pointProgram.release();
    
    // 生成Voronoi图数据
    computeVoronoiDiagram();
    
    doneCurrent();
    update();
}

void GLWidget::drawRandomPoints()
{
    if (canvasData.points.empty()) return;
    
    // 禁用深度测试，确保点在最上层
    glDisable(GL_DEPTH_TEST);
    
    // 使用持久化的点绘制资源
    pointVao.bind();
    pointProgram.bind();
    
    // 设置投影矩阵（与背景相同）
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    pointProgram.setUniformValue("projection", projection);
    
    // 绘制点
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, currentPointCount);
    glDisable(GL_PROGRAM_POINT_SIZE);
    
    // 清理
    pointProgram.release();
    pointVao.release();
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

void GLWidget::computeDelaunayTriangulation() {
    // 实现Delaunay三角剖分
    // 这里可以添加具体实现
}

void GLWidget::drawVoronoiDiagram()
{
    if (voronoiCells.empty() || !showVoronoiDiagram) return; // 新增：检查是否显示Voronoi图

    // 获取窗口尺寸和宽高比
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;

    // 设置投影矩阵（与背景相同）
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }

    // 设置简单的着色器
    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
        "}");
    
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.0, 0.0, 1.0, 1.0); // 蓝色线条\n"
        "}");
    
    if (!program.link()) {
        qWarning() << "Voronoi shader link error:" << program.log();
        return;
    }

    program.bind();
    program.setUniformValue("projection", projection);

    // 为每个Voronoi单元创建并绘制多边形
    for (const auto& cell : voronoiCells) {
        std::vector<float> vertices;
        vertices.reserve(cell.size() * 2);
        
        for (const auto& point : cell) {
            vertices.push_back(point.x());
            vertices.push_back(point.y());
        }

        // 创建临时VBO和VAO
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
        
        vao.create();
        vao.bind();
        
        vbo.create();
        vbo.bind();
        vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
        
        // 设置顶点属性
        int posLoc = program.attributeLocation("aPos");
        if (posLoc != -1) {
            program.enableAttributeArray(posLoc);
            program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
        }
        
        // 绘制多边形边界
        glLineWidth(1.5f); // 增加线宽使其更明显
        glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(cell.size()));
        
        vao.release();
        vbo.release();
    }
    
    program.release();
}

std::vector<QVector2D> GLWidget::clipVoronoiCellToRectangle(const std::vector<QVector2D>& cell, 
                                                           float left, float right, 
                                                           float bottom, float top)
{
    std::vector<QVector2D> clippedCell;
    
    // 检查单元是否需要裁剪
    bool needsClipping = false;
    for (const auto& pt : cell) {
        if (pt.x() < left || pt.x() > right || pt.y() < bottom || pt.y() > top) {
            needsClipping = true;
            break;
        }
    }
    
    if (!needsClipping) {
        return cell; // 如果单元完全在矩形内，直接返回
    }
    
    // 检查每条边是否与边界相交
    for (size_t i = 0; i < cell.size(); i++) {
        const QVector2D& start = cell[i];
        const QVector2D& end = cell[(i + 1) % cell.size()];
        
        // 起点在矩形内
        if (start.x() >= left && start.x() <= right && 
            start.y() >= bottom && start.y() <= top) {
            clippedCell.push_back(start);
        }
        
        // 检查边是否与边界相交
        std::vector<QVector2D> intersections;
        
        // 检查与左边界的交点
        if ((start.x() < left && end.x() > left) || 
            (start.x() > left && end.x() < left)) {
            float t = (left - start.x()) / (end.x() - start.x());
            float y = start.y() + t * (end.y() - start.y());
            if (y >= bottom && y <= top) {
                intersections.push_back(QVector2D(left, y));
            }
        }
        
        // 检查与右边界的交点
        if ((start.x() < right && end.x() > right) || 
            (start.x() > right && end.x() < right)) {
            float t = (right - start.x()) / (end.x() - start.x());
            float y = start.y() + t * (end.y() - start.y());
            if (y >= bottom && y <= top) {
                intersections.push_back(QVector2D(right, y));
            }
        }
        
        // 检查与下边界的交点
        if ((start.y() < bottom && end.y() > bottom) || 
            (start.y() > bottom && end.y() < bottom)) {
            float t = (bottom - start.y()) / (end.y() - start.y());
            float x = start.x() + t * (end.x() - start.x());
            if (x >= left && x <= right) {
                intersections.push_back(QVector2D(x, bottom));
            }
        }
        
        // 检查与上边界的交点
        if ((start.y() < top && end.y() > top) || 
            (start.y() > top && end.y() < top)) {
            float t = (top - start.y()) / (end.y() - start.y());
            float x = start.x() + t * (end.x() - start.x());
            if (x >= left && x <= right) {
                intersections.push_back(QVector2D(x, top));
            }
        }
        
        // 添加交点（按距离起点的远近排序）
        if (!intersections.empty()) {
            std::sort(intersections.begin(), intersections.end(), 
                [&](const QVector2D& a, const QVector2D& b) {
                    return (a - start).lengthSquared() < 
                           (b - start).lengthSquared();
                });
            
            for (const auto& pt : intersections) {
                clippedCell.push_back(pt);
            }
        }
    }
    
    // 确保裁剪后的多边形是闭合的
    if (!clippedCell.empty() && clippedCell.front() != clippedCell.back()) {
        clippedCell.push_back(clippedCell.front());
    }
    
    return clippedCell;
}

void GLWidget::computeVoronoiDiagram()
{
    voronoiCells.clear();

    if (canvasData.points.empty()) return;

    // 直接使用已有的Delaunay三角剖分
    const DelaunayTriangulation& dt = canvasData.dt;

    // 创建Voronoi图
    VoronoiDiagram vd(dt);

    // 定义矩形边界
    const float left = -1.0f;
    const float right = 1.0f;
    const float bottom = -1.0f;
    const float top = 1.0f;

    // 遍历所有面（每个面对应一个采样点）
    for (auto fit = vd.faces_begin(); fit != vd.faces_end(); ++fit) {
        // 跳过无限面
        if (fit->is_unbounded()) continue;
        
        std::vector<QVector2D> cell;
        
        // 获取面的边界循环
        Ccb_halfedge_circulator ec_start = fit->ccb();
        Ccb_halfedge_circulator ec = ec_start;
        
        do {
            if (ec->has_target()) {
                Point_2 p = ec->target()->point();
                cell.push_back(QVector2D(p.x(), p.y()));
            }
        } while (++ec != ec_start);
        
        // 裁剪单元到矩形边界
        std::vector<QVector2D> clippedCell = clipVoronoiCellToRectangle(cell, left, right, bottom, top);
        voronoiCells.push_back(clippedCell);
    }

    update();
}

void GLWidget::drawDelaunayTriangles()
{
    if (canvasData.dt.number_of_faces() == 0 || !showDelaunay) 
        return;
    // 获取窗口尺寸和宽高比
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;

    // 设置投影矩阵
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }

    // 准备顶点数据 (仅使用原始点集)
    std::vector<float> vertices;
    for (const auto& p : canvasData.points) {
        vertices.push_back(p.x());
        vertices.push_back(p.y());
    }

    // 准备索引数据 - 使用GL_LINES模式绘制边线
    std::vector<unsigned int> indices;
    std::unordered_map<Point, unsigned int> pointToIndex;
    
    // 创建点坐标到索引的映射
    for (unsigned int i = 0; i < canvasData.points.size(); i++) {
        pointToIndex[canvasData.points[i]] = i;
    }
    
    // 定义四个角点
    Point corner1(-1.0f, -1.0f);
    Point corner2(1.0f, -1.0f);
    Point corner3(-1.0f, 1.0f);
    Point corner4(1.0f, 1.0f);
    
    // 改为遍历所有有限边
    for (auto eit = canvasData.dt.finite_edges_begin(); 
         eit != canvasData.dt.finite_edges_end(); ++eit) 
    {
        // 获取边的两个端点
        auto face = eit->first;
        int edgeIndex = eit->second;
        auto vh1 = face->vertex(face->cw(edgeIndex));
        auto vh2 = face->vertex(face->ccw(edgeIndex));
        
        Point p1(vh1->point().x(), vh1->point().y());
        Point p2(vh2->point().x(), vh2->point().y());
        
        // 检查是否与角点相连
        bool isCornerEdge = 
            (p1 == corner1 || p1 == corner2 || p1 == corner3 || p1 == corner4) ||
            (p2 == corner1 || p2 == corner2 || p2 == corner3 || p2 == corner4);
        
        // 添加边的两个端点索引（排除与角点相连的边）
        if (!isCornerEdge && 
            pointToIndex.find(p1) != pointToIndex.end() && 
            pointToIndex.find(p2) != pointToIndex.end()) 
        {
            indices.push_back(pointToIndex[p1]);
            indices.push_back(pointToIndex[p2]);
        }
    }

    // 设置着色器
    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
        "}");
    
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0, 0.5, 0.0, 1.0); // 橙色线条\n"
        "}");
    
    if (!program.link()) {
        qWarning() << "Delaunay shader link error:" << program.log();
        return;
    }

    program.bind();
    program.setUniformValue("projection", projection);

    // 创建临时VAO和VBO
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer ebo(QOpenGLBuffer::IndexBuffer);
    
    vao.create();
    vao.bind();
    
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    ebo.create();
    ebo.bind();
    ebo.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(unsigned int)));
    
    // 设置顶点属性
    int posLoc = program.attributeLocation("aPos");
    if (posLoc != -1) {
        program.enableAttributeArray(posLoc);
        program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    }
    
    // 绘制三角形边线
    glLineWidth(1.5f);
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
    
    vao.release();
    program.release();
}

void GLWidget::performLloydRelaxation() {
    if (voronoiCells.empty()) {
        computeVoronoiDiagram();
    }
    
    // 存储新点位置
    std::vector<Point> newPoints = canvasData.points;
    
    // 前4个点是矩形角点，不移动
    for (size_t i = 4; i < canvasData.points.size(); i++) {
        // 获取当前点的Voronoi单元
        const std::vector<QVector2D>& cell = voronoiCells[i-4];
        
        // 计算单元重心
        float centroidX = 0.0f;
        float centroidY = 0.0f;
        float area = 0.0f;
        
        // 使用鞋带公式计算重心
        int n = cell.size();
        for (int j = 0; j < n; j++) {
            const QVector2D& p1 = cell[j];
            const QVector2D& p2 = cell[(j+1) % n];
            
            float cross = p1.x() * p2.y() - p2.x() * p1.y();
            area += cross;
            centroidX += (p1.x() + p2.x()) * cross;
            centroidY += (p1.y() + p2.y()) * cross;
        }
        
        if (fabs(area) > 1e-7) {
            area *= 0.5f;
            centroidX /= (6.0f * area);
            centroidY /= (6.0f * area);
            
            // 更新点位置
            newPoints[i] = Point(centroidX, centroidY);
        }
    }
    
    // 更新点集
    canvasData.points = newPoints;
    
    // 重新计算Delaunay三角剖分
    canvasData.dt = Delaunay(canvasData.points.begin(), canvasData.points.end());
    
    // 准备点数据
    std::vector<float> points;
    points.reserve(canvasData.points.size() * 2);
    for (const auto& point : canvasData.points) {
        points.push_back(point.x());
        points.push_back(point.y());
    }
    
    // 更新点缓冲区
    makeCurrent();
    pointVao.bind();
    pointVbo.bind();
    pointVbo.allocate(points.data(), static_cast<int>(points.size() * sizeof(float)));
    pointVao.release();
    pointVbo.release();
    doneCurrent();
    
    // 重新计算Voronoi图
    computeVoronoiDiagram();
    
    update();
}

void GLWidget::drawCVTBackground()
{
    // 获取窗口尺寸
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;

    // 设置正交投影
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    
    // 创建模型矩阵
    QMatrix4x4 model;
    
    // 使用简单的着色器绘制白色背景
    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * model * vec4(aPos, 1.0);\n"
        "}");
    
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // 白色\n"
        "}");
    
    if (!program.link()) {
        qWarning() << "Background shader link error:" << program.log();
        return;
    }
    
    // 正方形顶点数据
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    
    // 索引数据
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    // 创建临时VAO和VBO
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer ebo(QOpenGLBuffer::IndexBuffer);
    
    vao.create();
    vao.bind();
    
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));
    
    ebo.create();
    ebo.bind();
    ebo.allocate(indices, sizeof(indices));
    
    // 设置顶点属性
    program.bind();
    int posLoc = program.attributeLocation("aPos");
    if (posLoc != -1) {
        program.enableAttributeArray(posLoc);
        program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    // 设置变换矩阵
    program.setUniformValue("model", model);
    program.setUniformValue("projection", projection);
    
    // 绘制正方形
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // 清理
    vao.release();
    program.release();
    glDisable(GL_DEPTH_TEST);
    
    // 在背景上绘制随机点
    if (!canvasData.points.empty() && showPoints) { // 修改：添加showPoints判断
        drawRandomPoints();    
    }

    if(showVoronoiDiagram) {
        drawVoronoiDiagram();
    }

    if(showDelaunay) {
        drawDelaunayTriangles();
    }
    
    glEnable(GL_DEPTH_TEST);
}