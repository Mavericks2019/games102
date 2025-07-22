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
    canvasData.points.reserve(count + 4);

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
    currentPointCount = count;
    
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

void GLWidget::computeVoronoiDiagram()
{
    voronoiCells.clear();
    delaunayVertices.clear();
    delaunayIndices.clear();

    if (canvasData.points.empty()) return;

    // 添加矩形的四个角点
    boundaryPoints = {
        Point(-1.0f, -1.0f),
        Point(1.0f, -1.0f),
        Point(1.0f, 1.0f),
        Point(-1.0f, 1.0f)
    };
    
    std::vector<Point_2> points;
    points.reserve(canvasData.points.size() + boundaryPoints.size());
    
    // 创建点坐标到索引的映射
    pointIndexMap.clear();
    
    // 添加随机点
    for (int i = 0; i < canvasData.points.size(); i++) {
        const auto& p = canvasData.points[i];
        Point_2 cgalPoint(p.x(), p.y()); // 修复：显式创建Point_2对象
        points.push_back(cgalPoint);
        pointIndexMap[cgalPoint] = i;
    }
    
    // 添加边界点
    for (int i = 0; i < boundaryPoints.size(); i++) {
        const auto& p = boundaryPoints[i];
        Point_2 cgalPoint(p.x(), p.y()); // 修复：显式创建Point_2对象
        points.push_back(cgalPoint);
        pointIndexMap[cgalPoint] = canvasData.points.size() + i;
    }

    // 创建Delaunay三角剖分
    DelaunayTriangulation dt;
    dt.insert(points.begin(), points.end());

    // 存储Delaunay三角形
    for (auto fit = dt.finite_faces_begin(); fit != dt.finite_faces_end(); ++fit) {
        for (int i = 0; i < 3; i++) {
            auto vh = fit->vertex(i);
            if (vh == nullptr || dt.is_infinite(vh)) continue;
            
            Point_2 p = vh->point();
            auto it = pointIndexMap.find(p);
            if (it != pointIndexMap.end()) {
                delaunayIndices.push_back(it->second);
            }
        }
    }
    // 创建Voronoi图
    VoronoiDiagram vd(dt);

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
        
        voronoiCells.push_back(cell);
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
    
    // 遍历所有有限面（三角形）
    for (auto fit = canvasData.dt.finite_faces_begin(); 
         fit != canvasData.dt.finite_faces_end(); ++fit) 
    {
        // 获取三角形的三个顶点
        for (int i = 0; i < 3; i++) {
            auto vh = fit->vertex(i);
            Point p(vh->point().x(), vh->point().y());
            
            if (pointToIndex.find(p) != pointToIndex.end()) {
                unsigned int idx = pointToIndex[p];
                indices.push_back(idx);
            }
        }
        
        // 添加闭合三角形的额外索引（连接最后一个顶点和第一个顶点）
        auto vh0 = fit->vertex(0);
        Point p0(vh0->point().x(), vh0->point().y());
        if (pointToIndex.find(p0) != pointToIndex.end()) {
            indices.push_back(pointToIndex[p0]);
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
    // 实现Lloyd松弛算法
    // 这里可以添加具体实现
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
    if (!canvasData.points.empty()) {
        // 绘制Voronoi图
        drawVoronoiDiagram();
        // 绘制Delaunay三角网格
        drawDelaunayTriangles();
        drawRandomPoints();    
    }
    
    glEnable(GL_DEPTH_TEST);
}