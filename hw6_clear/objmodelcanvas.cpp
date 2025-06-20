#include "objmodelcanvas.h"
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include <QDebug>
#include <QDir>
#include <algorithm>  // 用于std::sort
#include <mutex>
#include <omp.h>
#include <thread>

using namespace Eigen;
using namespace cv;

static bool insideTriangle(float x, float y, const Eigen::Vector4f* v) {
    Eigen::Vector3f points[3];
    for (int i = 0; i < 3; i++) {
        points[i] = Eigen::Vector3f(v[i].x(), v[i].y(), 1.0f);
    }
    
    Eigen::Vector3f p(x, y, 1.0f);
    
    Eigen::Vector3f edge0 = points[1] - points[0];
    Eigen::Vector3f edge1 = points[2] - points[1];
    Eigen::Vector3f edge2 = points[0] - points[2];
    
    Eigen::Vector3f normal0 = edge0.cross(points[0] - p);
    Eigen::Vector3f normal1 = edge1.cross(points[1] - p);
    Eigen::Vector3f normal2 = edge2.cross(points[2] - p);
    
    return (normal0.dot(normal1)) > 0 && (normal1.dot(normal2)) > 0;
}

// 计算重心坐标 (修改为使用Eigen::Vector4f)
static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Eigen::Vector4f* v) {
    float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y());
    float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y());
    float c3 = (x * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * y + v[0].x() * v[1].y() - v[1].x() * v[0].y());
    
    float denom = c1 + c2 + c3;
    if (denom == 0) return {0, 0, 0};
    
    return {c1 / denom, c2 / denom, c3 / denom};
}


ObjModelCanvas::ObjModelCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    curveColor = Qt::darkGray;
    allowPointCreation = false; // 禁用点创建
    cameraPosition = Eigen::Vector3f(0, 0, 1.5f); // 更靠近屏幕
    showFaces = false; // 默认显示面
    
    // 初始化光照参数
    lightPosition = Eigen::Vector3f(1.0f, 1.0f, 1.0f); // 光源位置
    lightColor = Eigen::Vector3f(1.0f, 1.0f, 1.0f);    // 白光
    ambientColor = Eigen::Vector3f(0.2f, 0.2f, 0.3f);   // 蓝色环境光
    
    // 初始化缓存矩阵
    cachedViewMatrix = Eigen::Matrix4f::Identity();
    cachedModelMatrix = Eigen::Matrix4f::Identity();
    
    // 初始化像素缓冲区
    pixelBuffer = Mat::zeros(600, 800, CV_8UC3);
    
    // 初始化深度缓冲区
    depth_buf.resize(600 * 800, std::numeric_limits<float>::max());
}

// 计算所有面的法向量
void ObjModelCanvas::calculateFaceNormals() {
    for (Face& face : model.faces) {
        if (face.vertexIndices.size() < 3) continue;
        
        // 获取面的三个顶点
        const Eigen::Vector3f& v0 = model.vertices[face.vertexIndices[0]];
        const Eigen::Vector3f& v1 = model.vertices[face.vertexIndices[1]];
        const Eigen::Vector3f& v2 = model.vertices[face.vertexIndices[2]];
        
        // 计算两个边向量
        Eigen::Vector3f edge1 = v1 - v0;
        Eigen::Vector3f edge2 = v2 - v0;
        
        // 叉积得到法向量并归一化
        face.normal = edge1.cross(edge2).normalized();
    }
}

// 获取模型矩阵（不包含视图和投影）
Eigen::Matrix4f ObjModelCanvas::getModelMatrix() const {
    if (modelMatrixDirty) {
        // 模型矩阵 - 旋转和缩放
        cachedModelMatrix = Eigen::Matrix4f::Identity();
        
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
        
        cachedModelMatrix = rotationXMatrix * rotationYMatrix;
        cachedModelMatrix.block<3, 3>(0, 0) *= zoom;
        
        modelMatrixDirty = false;
    }
    return cachedModelMatrix;
}

// 获取视图矩阵
Eigen::Matrix4f ObjModelCanvas::getViewMatrix() const {
    if (viewMatrixDirty) {
        // 视图矩阵 - 添加向上向量
        Eigen::Vector3f eye = cameraPosition;
        Eigen::Vector3f center = cameraTarget;
        Eigen::Vector3f up = m_upVector;
        
        Eigen::Vector3f f = (center - eye).normalized();
        Eigen::Vector3f s = f.cross(up).normalized();
        Eigen::Vector3f u = s.cross(f);
        
        cachedViewMatrix = Eigen::Matrix4f::Identity();
        cachedViewMatrix(0, 0) = s.x();
        cachedViewMatrix(0, 1) = s.y();
        cachedViewMatrix(0, 2) = s.z();
        cachedViewMatrix(1, 0) = u.x();
        cachedViewMatrix(1, 1) = u.y();
        cachedViewMatrix(1, 2) = u.z();
        cachedViewMatrix(2, 0) = -f.x();
        cachedViewMatrix(2, 1) = -f.y();
        cachedViewMatrix(2, 2) = -f.z();
        cachedViewMatrix(0, 3) = -s.dot(eye);
        cachedViewMatrix(1, 3) = -u.dot(eye);
        cachedViewMatrix(2, 3) = f.dot(eye);
        
        viewMatrixDirty = false;
    }
    return cachedViewMatrix;
}

// 根据法向量计算面颜色
QColor ObjModelCanvas::calculateFaceColor(const Eigen::Vector3f& normal) const {
    // 获取模型矩阵和视图矩阵
    Eigen::Matrix4f modelMatrix = getModelMatrix();
    Eigen::Matrix4f viewMatrix = getViewMatrix();
    
    // 转换法向量到视图空间
    Eigen::Matrix3f modelViewMatrix3x3 = (viewMatrix * modelMatrix).block<3,3>(0,0);
    Eigen::Vector3f viewNormal = (modelViewMatrix3x3 * normal).normalized();
    
    // 光源方向（归一化）
    Eigen::Vector3f lightDir = (lightPosition - cameraPosition).normalized();
    
    // 视线方向（从物体指向相机）
    Eigen::Vector3f viewDir = (-cameraPosition).normalized();
    
    // 计算漫反射分量
    float diffuse = std::max(0.0f, viewNormal.dot(lightDir));
    
    // 计算半角向量（布林-冯模型）
    Eigen::Vector3f halfDir = (lightDir + viewDir).normalized();
    
    // 计算镜面反射分量
    float specular = std::pow(std::max(0.0f, viewNormal.dot(halfDir)), shininess);
    
    // 组合光照分量
    Eigen::Vector3f ambientComp = ambientColor * ambientIntensity;
    Eigen::Vector3f diffuseComp = lightColor * diffuse * diffuseIntensity;
    Eigen::Vector3f specularComp = lightColor * specular * specularIntensity;
    
    // 组合最终颜色
    Eigen::Vector3f finalColor = ambientComp + diffuseComp + specularComp;
    
    // 确保颜色值在0-1范围内
    finalColor = finalColor.cwiseMax(Eigen::Vector3f::Zero()).cwiseMin(Eigen::Vector3f::Ones());
    
    // 转换为QColor
    return QColor(
        static_cast<int>(finalColor.x() * 255),
        static_cast<int>(finalColor.y() * 255),
        static_cast<int>(finalColor.z() * 255)
    );
}

// 更新所有面的颜色
void ObjModelCanvas::updateFaceColors() {
    for (Face& face : model.faces) {
        face.color = calculateFaceColor(face.normal);
    }
}

// 更新所有面的深度（到相机的距离）
void ObjModelCanvas::updateFaceDepths() {
    Eigen::Matrix4f viewMatrix = getViewMatrix();
    Eigen::Matrix4f modelMatrix = getModelMatrix();
    Eigen::Matrix4f modelViewMatrix = viewMatrix * modelMatrix;
    
    for (Face& face : model.faces) {
        // 计算面中心（世界坐标）
        Eigen::Vector3f center(0,0,0);
        for (int vertexIndex : face.vertexIndices) {
            if (vertexIndex < model.vertices.size()) {
                center += model.vertices[vertexIndex];
            }
        }
        center /= face.vertexIndices.size();
        
        // 将中心点变换到视图空间
        Eigen::Vector4f centerView = modelViewMatrix * Eigen::Vector4f(center.x(), center.y(), center.z(), 1.0f);
        face.viewCenter = centerView.head<3>();
        
        // 计算到相机的距离（深度）
        face.depth = centerView.z();
    }
}

// 按深度排序面（从远到近）
void ObjModelCanvas::sortFacesByDepth() {
    std::sort(model.faces.begin(), model.faces.end(), [](const Face& a, const Face& b) {
        return a.depth > b.depth; // 深度值大的（更负）先绘制（更远）
    });
}

void ObjModelCanvas::drawCurves(QPainter &painter)
{
    if (model.vertices.empty() || model.faces.empty()) return;
    
    // 根据绘制模式选择不同的绘制方法
    switch (drawMode) {
        case DrawMode::Triangles:
            drawTriangles(painter);
            break;
        case DrawMode::Pixels:
            drawPixels(painter);
            break;
    }
}

// 原有的三角形绘制方法
void ObjModelCanvas::drawTriangles(QPainter &painter)
{
    painter.setRenderHint(QPainter::Antialiasing);
    const Eigen::Matrix4f mvp = getModelViewProjection();

    // 1. 预计算所有顶点的投影坐标（线程安全）
    std::vector<QPointF> projectedVertices(model.vertices.size());
    #pragma omp parallel for
    for (int i = 0; i < model.vertices.size(); ++i) {
        projectedVertices[i] = projectVertex(model.vertices[i]);
    }

    // 2. 分割三角形列表到多个线程
    const int numThreads = std::thread::hardware_concurrency();
    const int trianglesPerThread = model.faces.size() / numThreads + 1;

    // 存储每个线程的离屏绘图表面
    std::vector<QImage> threadImages(numThreads, QImage(size(), QImage::Format_ARGB32));
    std::vector<std::thread> threads;

    for (int threadId = 0; threadId < numThreads; ++threadId) {
        threads.emplace_back([&, threadId]() {
            // 初始化当前线程的离屏图像
            QImage &img = threadImages[threadId];
            img.fill(Qt::transparent);
            QPainter threadPainter(&img);
            threadPainter.setRenderHint(QPainter::Antialiasing);

            // 计算当前线程处理的三角形范围
            const int startIdx = threadId * trianglesPerThread;
            const int endIdx = std::min(startIdx + trianglesPerThread, static_cast<int>(model.faces.size()));

            // 绘制分配给当前线程的三角形
            for (int faceIdx = startIdx; faceIdx < endIdx; ++faceIdx) {
                const Face& face = model.faces[faceIdx];
                QPolygonF polygon;
                for (int vertexIndex : face.vertexIndices) {
                    if (vertexIndex < projectedVertices.size()) {
                        polygon << projectedVertices[vertexIndex];
                    }
                }

                // 设置绘图属性
                threadPainter.setPen(QPen(curveColor, 1));
                if (showFaces) {
                    threadPainter.setBrush(QBrush(face.color));
                } else {
                    threadPainter.setBrush(Qt::NoBrush);
                }

                // 在离屏图像上绘制
                threadPainter.drawPolygon(polygon);
            }
        });
    }

    // 3. 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 4. 合并所有离屏图像到主画布
    for (const QImage& img : threadImages) {
        painter.drawImage(0, 0, img);
    }
}

// 新增：像素绘制方法
void ObjModelCanvas::drawPixels(QPainter &painter)
{
    // 确保像素缓冲区大小与画布匹配
    if (pixelBuffer.cols != width() || pixelBuffer.rows != height()) {
        pixelBuffer = Mat::zeros(height(), width(), CV_8UC3);
        depth_buf.resize(width() * height(), std::numeric_limits<float>::max());
    }
    
    // 清除像素缓冲区
    pixelBuffer.setTo(Scalar(0, 0, 0));
    
    // 重置深度缓冲区
    std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::max());
    
    // 计算模型视图投影矩阵
    Eigen::Matrix4f mvp = getModelViewProjection();
    
    // 遍历所有面
    for (const Face& face : model.faces) {
        // 获取面的三个顶点
        Eigen::Vector4f v[3];
        for (int i = 0; i < 3; i++) {
            int vertexIndex = face.vertexIndices[i];
            Eigen::Vector4f homogenous(model.vertices[vertexIndex].x(),
                                     model.vertices[vertexIndex].y(),
                                     model.vertices[vertexIndex].z(),
                                     1.0f);
            v[i] = mvp * homogenous;
            
            // 透视除法
            if (v[i].w() != 0.0f) {
                v[i].x() /= v[i].w();
                v[i].y() /= v[i].w();
                v[i].z() /= v[i].w();
            }
            
            // 视口变换
            v[i].x() = (v[i].x() + 1.0f) * 0.5f * width();
            v[i].y() = (1.0f - v[i].y()) * 0.5f * height();
        }
        
        // 计算面的包围盒
        float minX = std::min({v[0].x(), v[1].x(), v[2].x()});
        float maxX = std::max({v[0].x(), v[1].x(), v[2].x()});
        float minY = std::min({v[0].y(), v[1].y(), v[2].y()});
        float maxY = std::max({v[0].y(), v[1].y(), v[2].y()});
        
        // 限制在画布范围内
        int startX = std::max(0, static_cast<int>(std::floor(minX)));
        int endX = std::min(width() - 1, static_cast<int>(std::ceil(maxX)));
        int startY = std::max(0, static_cast<int>(std::floor(minY)));
        int endY = std::min(height() - 1, static_cast<int>(std::ceil(maxY)));
        
        // 遍历包围盒内的所有像素
        for (int y = startY; y <= endY; y++) {
            for (int x = startX; x <= endX; x++) {
                // 检查像素是否在三角形内
                if (insideTriangle(x + 0.5f, y + 0.5f, v)) {
                    // 计算重心坐标
                    auto [alpha, beta, gamma] = computeBarycentric2D(x + 0.5f, y + 0.5f, v);
                    
                    // 插值深度值
                    float z = alpha * v[0].z() + beta * v[1].z() + gamma * v[2].z();
                    
                    // 深度测试
                    int index = y * width() + x;
                    if (z < depth_buf[index]) {
                        depth_buf[index] = z;
                        
                        // 设置像素颜色
                        Vec3b color;
                        color[0] = static_cast<uchar>(face.color.blue());   // B
                        color[1] = static_cast<uchar>(face.color.green());  // G
                        color[2] = static_cast<uchar>(face.color.red());    // R
                        pixelBuffer.at<Vec3b>(y, x) = color;
                    }
                }
            }
        }
    }
    
    // 将OpenCV图像转换为QImage
    QImage img(pixelBuffer.data, pixelBuffer.cols, pixelBuffer.rows, 
              pixelBuffer.step, QImage::Format_RGB888);
    
    // 绘制到画布
    painter.drawImage(0, 0, img.rgbSwapped());
}
// 判断点是否在三角形内 (修改为使用Eigen::Vector4f)

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
    
    // 计算法向量
    calculateFaceNormals();
    
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
    
    // 设置光源位置为包围盒中心上方
    lightPosition = Eigen::Vector3f(0, m_maxBound.y() + m_boundingRadius * 0.5f, 0);
    
    fitObjectToView();
    
    // 更新面的颜色和深度
    updateFaceColors();
    updateFaceDepths();
    sortFacesByDepth();
    
    qDebug() << "Loaded OBJ model with" << model.vertices.size() << "vertices and" << model.faces.size() << "faces";
    update();
}

void ObjModelCanvas::drawGrid(QPainter &painter)
{
    // 在OBJ模型模式下不绘制网格
}

void ObjModelCanvas::drawPoints(QPainter &painter)
{
    // 在OBJ模型模式下不绘制点
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
    
    // 4. 设置光源位置为包围盒中心上方
    lightPosition = Eigen::Vector3f(0, m_maxBound.y() + m_boundingRadius * 0.5f, 0);
    
    // 标记矩阵需要更新
    modelMatrixDirty = true;
    viewMatrixDirty = true;
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

void ObjModelCanvas::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // 投影矩阵依赖于窗口大小，所以需要更新
    viewMatrixDirty = true;
    
    // 重置像素缓冲区和深度缓冲区大小
    pixelBuffer = Mat::zeros(height(), width(), CV_8UC3);
    depth_buf.resize(width() * height(), std::numeric_limits<float>::max());
    
    update();
}

void ObjModelCanvas::wheelEvent(QWheelEvent *event) {
    float delta = event->angleDelta().y() / 120.0f;
    float scaleFactor = 1.0f + delta * 0.1f;
    
    // 限制缩放范围在0.1到10倍之间
    float newZoom = zoom * scaleFactor;
    newZoom = std::max(0.1f, std::min(newZoom, 10.0f));
    
    zoom = newZoom;
    
    // 标记模型矩阵需要更新
    modelMatrixDirty = true;
    update();
}

void ObjModelCanvas::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
    }
}

void ObjModelCanvas::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - lastMousePos;
        rotationY += delta.x() * 0.01f;
        rotationX += delta.y() * 0.01f;
        lastMousePos = event->pos();
        
        // 标记模型矩阵需要更新
        modelMatrixDirty = true;
        update();
    }
}