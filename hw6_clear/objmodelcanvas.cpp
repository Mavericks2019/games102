#include "objmodelcanvas.h"
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include <QDebug>
#include <QDir>

using namespace Eigen;

ObjModelCanvas::ObjModelCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    curveColor = Qt::darkGray;
    allowPointCreation = false; // 禁用点创建
    cameraPosition = Eigen::Vector3f(0, 0, 1.5f); // 更靠近屏幕
}

void ObjModelCanvas::drawGrid(QPainter &painter)
{
    // 在OBJ模型模式下不绘制网格
}

void ObjModelCanvas::drawPoints(QPainter &painter)
{
    // 在OBJ模型模式下不绘制点
}

void ObjModelCanvas::drawCurves(QPainter &painter)
{
    if (model.vertices.empty() || model.faces.empty()) return;
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(curveColor, 1));
    
    Eigen::Matrix4f mvp = getModelViewProjection();
    
    // 绘制所有面
    for (const Face& face : model.faces) {
        QPolygonF polygon;
        for (int vertexIndex : face.vertexIndices) {
            if (vertexIndex < model.vertices.size()) {
                Eigen::Vector3f vertex = model.vertices[vertexIndex];
                QPointF screenPoint = projectVertex(vertex);
                polygon << screenPoint;
            }
        }
        painter.drawPolygon(polygon);
    }
}

void ObjModelCanvas::drawHoverIndicator(QPainter &painter)
{
    // 在OBJ模型模式下不绘制悬停指示器
}

void ObjModelCanvas::loadObjFile(const QString &filePath)
{
    model.vertices.clear();
    model.faces.clear();
    parseObjFile(filePath);
    fitObjectToView();
    update();
}

void ObjModelCanvas::calculateBoundingBox() {
    if (model.vertices.empty()) {
        m_minBound = Eigen::Vector3f(0, 0, 0);
        m_maxBound = Eigen::Vector3f(0, 0, 0);
        m_center = Eigen::Vector3f(0, 0, 0);
        m_boundingRadius = 0.0f;
        return;
    }

    // 初始化最小最大值
    m_minBound = model.vertices[0];
    m_maxBound = model.vertices[0];

    // 计算包围盒
    for (const Eigen::Vector3f& vertex : model.vertices) {
        m_minBound = m_minBound.cwiseMin(vertex);
        m_maxBound = m_maxBound.cwiseMax(vertex);
    }

    // 计算中心点和包围球半径
    m_center = (m_minBound + m_maxBound) * 0.5f;
    m_boundingRadius = (m_maxBound - m_minBound).norm() * 0.5f;
}

void ObjModelCanvas::fitObjectToView() {
    calculateBoundingBox();
    
    if (model.vertices.empty()) return;
    
    // 1. 将包围盒扩大1.5倍
    const float scaleFactor = 1.5f;
    Eigen::Vector3f size = (m_maxBound - m_minBound) * scaleFactor;
    m_minBound = m_center - size * 0.5f;
    m_maxBound = m_center + size * 0.5f;
    m_boundingRadius *= scaleFactor;

    // 2. 调整物体位置使包围盒中心位于原点
    Eigen::Matrix4f translation = Eigen::Matrix4f::Identity();
    translation.block<3,1>(0,3) = -m_center;
    for (Eigen::Vector3f& vertex : model.vertices) {
        Eigen::Vector4f homogenous(vertex.x(), vertex.y(), vertex.z(), 1.0f);
        homogenous = translation * homogenous;
        vertex = homogenous.head<3>();
    }
    m_center = Eigen::Vector3f(0, 0, 0);

    // 3. 调整相机位置和方向
    adjustCamera();
}

void ObjModelCanvas::adjustCamera() {
    // 计算包围盒尺寸
    Eigen::Vector3f size = m_maxBound - m_minBound;
    
    // 确定最窄方向（找最小尺寸的轴）
    int minAxis = 0;
    float minSize = size.x();
    if (size.y() < minSize) {
        minSize = size.y();
        minAxis = 1;
    }
    if (size.z() < minSize) {
        minAxis = 2;
    }

    // 设置相机位置（沿最窄方向）
    float distance = m_boundingRadius * 1.5f; // 3倍半径距离
    Eigen::Vector3f eyePosition;
    
    switch (minAxis) {
    case 0: // X轴最窄
        eyePosition = Eigen::Vector3f(distance, 0, 0);
        m_upVector = Eigen::Vector3f(0, 1, 0); // Y轴向上
        break;
    case 1: // Y轴最窄
        eyePosition = Eigen::Vector3f(0, distance, 0);
        m_upVector = Eigen::Vector3f(0, 0, 1); // Z轴向上
        break;
    case 2: // Z轴最窄
    default:
        eyePosition = Eigen::Vector3f(0, 0, distance);
        m_upVector = Eigen::Vector3f(0, 1, 0); // Y轴向上
        break;
    }

    // 设置相机参数
    cameraPosition = eyePosition;
    cameraTarget = m_center;

    // 自动计算合适的FOV
    float maxDimension = std::max({size.x(), size.y(), size.z()});
    float fov = 45.0f * (maxDimension / m_boundingRadius);
    fov = std::max(30.0f, std::min(fov, 90.0f)); // 限制在30-90度之间
    this->fov = fov;

    // 重置旋转和缩放
    rotationX = 0.0f;
    rotationY = 0.0f;
    zoom = 1.0f;
}

void ObjModelCanvas::clearPoints()
{
    model.vertices.clear();
    model.faces.clear();
    update();
}

// 重置视图时也调用自适应调整
void ObjModelCanvas::resetView() {
    fitObjectToView();
    update();
}

void ObjModelCanvas::parseObjFile(const QString &filePath)
{
    model.vertices.clear();
    model.faces.clear();
    
    // 确保路径是绝对路径
    QString absolutePath = filePath;
    if (!QDir::isAbsolutePath(filePath)) {
        absolutePath = QDir::current().absoluteFilePath(filePath);
    }
    
    if (!QFile::exists(absolutePath)) {
        qWarning() << "OBJ file does not exist:" << absolutePath;
        return;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open OBJ file:" << filePath;
        return;
    }
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;
        
        QStringList tokens = line.split(" ", QString::SkipEmptyParts);
        if (tokens.isEmpty()) continue;
        
        QString identifier = tokens[0];
        
        if (identifier == "v") {
            if (tokens.size() >= 4) {
                Vector3f vertex;
                vertex[0] = tokens[1].toFloat();
                vertex[1] = tokens[2].toFloat();
                vertex[2] = tokens[3].toFloat();
                model.vertices.push_back(vertex);
            }
        } else if (identifier == "f") {
            Face face;
            for (int i = 1; i < tokens.size(); i++) {
                QStringList indices = tokens[i].split("/");
                if (!indices.isEmpty()) {
                    int vertexIndex = indices[0].toInt() - 1;
                    if (vertexIndex >= 0 && vertexIndex < model.vertices.size()) {
                        face.vertexIndices.push_back(vertexIndex);
                    }
                }
            }
            if (!face.vertexIndices.empty()) {
                model.faces.push_back(face);
            }
        }
    }
    
    file.close();
    
    // 计算模型包围盒
    if (!model.vertices.empty()) {
        Eigen::Vector3f minPoint = model.vertices[0];
        Eigen::Vector3f maxPoint = model.vertices[0];
        
        for (const auto& vertex : model.vertices) {
            minPoint = minPoint.cwiseMin(vertex);
            maxPoint = maxPoint.cwiseMax(vertex);
        }
        
        modelCenter = (minPoint + maxPoint) * 0.5f;
        Eigen::Vector3f size = maxPoint - minPoint;
        float maxSize = std::max({size.x(), size.y(), size.z()});
        
        // 将模型中心移动到原点
        for (auto& vertex : model.vertices) {
            vertex -= modelCenter;
        }
        
        // 根据模型大小调整相机位置
        adjustCameraPosition(maxSize);
    }
    fitObjectToView();
    qDebug() << "Loaded OBJ model with" << model.vertices.size() << "vertices and" << model.faces.size() << "faces";
    update();
}

void ObjModelCanvas::adjustCameraPosition(float modelSize)
{
    // 根据模型大小自适应调整相机位置
    if (modelSize > 0.001f) {
        // 计算合适的相机距离：模型最大尺寸的2.5倍
        float distance = modelSize * 2.5f;
        
        // 确保相机不会太近或太远
        distance = std::max(0.5f, std::min(distance, 20.0f));
        
        cameraPosition = Eigen::Vector3f(0, 0, distance);
    } else {
        // 默认距离
        cameraPosition = Eigen::Vector3f(0, 0, 5.0f);
    }
    
    // 设置更宽的视野
    fov = 60.0f;
    
    // 重置缩放
    zoom = 1.0f;
}

Eigen::Matrix4f ObjModelCanvas::getModelViewProjection() const
{
    // 模型矩阵 - 旋转和缩放
    Eigen::Matrix4f modelMatrix = Eigen::Matrix4f::Identity();
    
    // 围绕Y轴旋转
    Eigen::Matrix4f rotationYMatrix = Eigen::Matrix4f::Identity();
    rotationYMatrix(0, 0) = cos(rotationY);
    rotationYMatrix(0, 2) = sin(rotationY);
    rotationYMatrix(2, 0) = -sin(rotationY);
    rotationYMatrix(2, 2) = cos(rotationY);
    
    // 围绕X轴旋转
    Eigen::Matrix4f rotationXMatrix = Eigen::Matrix4f::Identity();
    rotationXMatrix(1, 1) = cos(rotationX);
    rotationXMatrix(1, 2) = -sin(rotationX);
    rotationXMatrix(2, 1) = sin(rotationX);
    rotationXMatrix(2, 2) = cos(rotationX);
    
    modelMatrix = rotationXMatrix * rotationYMatrix;
    modelMatrix.block<3, 3>(0, 0) *= zoom;
    
    // 视图矩阵 - 添加向上向量
    Eigen::Vector3f eye = cameraPosition;
    Eigen::Vector3f center(0, 0, 0);
    Eigen::Vector3f up = m_upVector;
    
    Eigen::Vector3f f = (center - eye).normalized();
    Eigen::Vector3f s = f.cross(up).normalized();
    Eigen::Vector3f u = s.cross(f);
    
    Eigen::Matrix4f viewMatrix = Eigen::Matrix4f::Identity();
    viewMatrix(0, 0) = s.x();
    viewMatrix(0, 1) = s.y();
    viewMatrix(0, 2) = s.z();
    viewMatrix(1, 0) = u.x();
    viewMatrix(1, 1) = u.y();
    viewMatrix(1, 2) = u.z();
    viewMatrix(2, 0) = -f.x();
    viewMatrix(2, 1) = -f.y();
    viewMatrix(2, 2) = -f.z();
    viewMatrix(0, 3) = -s.dot(eye);
    viewMatrix(1, 3) = -u.dot(eye);
    viewMatrix(2, 3) = f.dot(eye);
    
    // 投影矩阵 - 使用透视投影
    float aspect = static_cast<float>(width()) / height();
    float near = 0.1f;
    float far = 100.0f;
    float fovRad = fov * M_PI / 180.0f;
    float tanHalfFov = tan(fovRad / 2.0f);
    
    Eigen::Matrix4f projectionMatrix = Eigen::Matrix4f::Zero();
    projectionMatrix(0, 0) = 1.0f / (aspect * tanHalfFov);
    projectionMatrix(1, 1) = 1.0f / tanHalfFov;
    projectionMatrix(2, 2) = -(far + near) / (far - near);
    projectionMatrix(2, 3) = -2.0f * far * near / (far - near);
    projectionMatrix(3, 2) = -1.0f;
    
    return projectionMatrix * viewMatrix * modelMatrix;
}

QPointF ObjModelCanvas::projectVertex(const Eigen::Vector3f& vertex) const
{
    Eigen::Matrix4f mvp = getModelViewProjection();
    Eigen::Vector4f homogenousVertex(vertex[0], vertex[1], vertex[2], 1.0f);
    Eigen::Vector4f transformed = mvp * homogenousVertex;
    
    // 透视除法
    if (transformed[3] != 0.0f) {
        transformed /= transformed[3];
    }
    
    // 转换为屏幕坐标
    float x = (transformed[0] + 1.0f) * 0.5f * width();
    float y = (1.0f - transformed[1]) * 0.5f * height();
    
    return QPointF(x, y);
}

void ObjModelCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    QString info = QString("OBJ Model - Vertices: %1, Faces: %2")
                  .arg(model.vertices.size())
                  .arg(model.faces.size());
    painter.drawText(10, 20, info);
}

// objmodelcanvas.cpp
void ObjModelCanvas::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    float scaleFactor = 1.0f + delta * 0.1f;
    
    // 限制缩放范围在0.1到10倍之间
    float newZoom = zoom * scaleFactor;
    newZoom = std::max(0.1f, std::min(newZoom, 10.0f));
    
    zoom = newZoom;
    update();
}

void ObjModelCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
    }
}

void ObjModelCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - lastMousePos;
        rotationY += delta.x() * 0.01f;
        rotationX += delta.y() * 0.01f;
        lastMousePos = event->pos();
        update();
    }
}