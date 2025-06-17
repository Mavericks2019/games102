#include "objmodelcanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFile>
#include <QTextStream>
#include <QtMath>
#include <QDebug>
#include <QFileInfo>

ObjModelCanvas::ObjModelCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    setMinimumSize(800, 600);
    setMouseTracking(true);
    rotationMatrix.setToIdentity();
    curveColor = Qt::darkGray;
}

bool ObjModelCanvas::loadObjFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << filePath;
        return false;
    }
    
    // 重置状态
    vertices.clear();
    faces.clear();
    modelLoaded = false;
    rotationMatrix.setToIdentity();
    scaleFactor = 1.0f;
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;
        
        QStringList parts = line.split(" ", QString::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        
        QString command = parts[0];
        
        if (command == "v") { // 顶点
            if (parts.size() >= 4) {
                float x = parts[1].toFloat();
                float y = parts[2].toFloat();
                float z = parts[3].toFloat();
                vertices.append(QVector3D(x, y, z));
            }
        }
        else if (command == "f") { // 面
            Face face;
            for (int i = 1; i < parts.size(); i++) {
                QStringList vertexData = parts[i].split("/");
                int vertexIndex = vertexData[0].toInt() - 1; // OBJ索引从1开始
                if (vertexIndex >= 0 && vertexIndex < vertices.size()) {
                    face.vertexIndices.append(vertexIndex);
                }
            }
            if (!face.vertexIndices.isEmpty()) {
                faces.append(face);
            }
        }
    }
    
    file.close();
    
    if (!vertices.isEmpty() && !faces.isEmpty()) {
        calculateBoundingBox();
        centerAndScaleModel();
        modelLoaded = true;
        
        // 提取文件名
        QFileInfo fileInfo(filePath);
        currentModelName = fileInfo.fileName();
        
        update();
        return true;
    }
    
    return false;
}

void ObjModelCanvas::resetView()
{
    rotationMatrix.setToIdentity();
    scaleFactor = 1.0f;
    if (modelLoaded) {
        centerAndScaleModel();
    }
    update();
}

void ObjModelCanvas::calculateBoundingBox()
{
    if (vertices.isEmpty()) return;
    
    minBounds = vertices[0];
    maxBounds = vertices[0];
    
    for (const QVector3D &vertex : vertices) {
        minBounds.setX(qMin(minBounds.x(), vertex.x()));
        minBounds.setY(qMin(minBounds.y(), vertex.y()));
        minBounds.setZ(qMin(minBounds.z(), vertex.z()));
        
        maxBounds.setX(qMax(maxBounds.x(), vertex.x()));
        maxBounds.setY(qMax(maxBounds.y(), vertex.y()));
        maxBounds.setZ(qMax(maxBounds.z(), vertex.z()));
    }
    
    modelCenter = (minBounds + maxBounds) * 0.5f;
}

void ObjModelCanvas::centerAndScaleModel()
{
    if (vertices.isEmpty()) return;
    
    // 计算模型尺寸
    QVector3D size = maxBounds - minBounds;
    float maxSize = qMax(size.x(), qMax(size.y(), size.z()));
    
    // 自动缩放以适应画布
    if (maxSize > 0) {
        float canvasSize = qMin(width(), height()) * 0.8f;
        scaleFactor = canvasSize / maxSize;
    }
}

QPointF ObjModelCanvas::projectToScreen(const QVector3D &point) const
{
    // 应用旋转
    QVector3D rotated = rotationMatrix * (point - modelCenter);
    
    // 应用缩放
    rotated *= scaleFactor;
    
    // 投影到2D (忽略Z坐标)
    float x = width() / 2.0f + rotated.x();
    float y = height() / 2.0f - rotated.y(); // Y轴翻转
    
    return QPointF(x, y);
}

void ObjModelCanvas::drawCurves(QPainter &painter)
{
    if (!modelLoaded) return;
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(curveColor, 1));
    
    // 绘制所有面
    for (const Face &face : faces) {
        if (face.vertexIndices.size() < 2) continue;
        
        // 绘制多边形边
        QPointF prevPoint = projectToScreen(vertices[face.vertexIndices.first()]);
        for (int i = 1; i < face.vertexIndices.size(); i++) {
            QPointF currentPoint = projectToScreen(vertices[face.vertexIndices[i]]);
            painter.drawLine(prevPoint, currentPoint);
            prevPoint = currentPoint;
        }
        
        // 闭合多边形
        if (face.vertexIndices.size() > 2) {
            QPointF firstPoint = projectToScreen(vertices[face.vertexIndices.first()]);
            painter.drawLine(prevPoint, firstPoint);
        }
    }
}

void ObjModelCanvas::drawPoints(QPainter &painter)
{
    // 该画板不需要绘制点
    Q_UNUSED(painter);
}

void ObjModelCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    
    QString info = "OBJ Model Viewer";
    if (modelLoaded) {
        info += " | " + currentModelName;
        info += QString(" | Vertices: %1 | Faces: %2").arg(vertices.size()).arg(faces.size());
        info += " | Scale: " + QString::number(scaleFactor, 'f', 2);
    } else {
        info += " | No model loaded";
    }
    
    painter.drawText(10, 20, info);
    
    // 添加操作提示
    if (modelLoaded) {
        painter.drawText(10, height() - 30, "Left drag: Rotate | Wheel: Zoom | Double click: Reset view");
    }
}

void ObjModelCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
    }
}

void ObjModelCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (!modelLoaded || !(event->buttons() & Qt::LeftButton)) 
        return;
    
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();
    
    // 旋转角度（灵敏度调节）
    float rotX = delta.y() * 0.5f;
    float rotY = delta.x() * 0.5f;
    
    // 创建旋转矩阵
    QMatrix4x4 newRotation;
    newRotation.rotate(rotY, 0.0f, 1.0f, 0.0f); // Y轴旋转
    newRotation.rotate(rotX, 1.0f, 0.0f, 0.0f); // X轴旋转
    
    // 应用新旋转
    rotationMatrix = newRotation * rotationMatrix;
    
    update();
}

void ObjModelCanvas::wheelEvent(QWheelEvent *event)
{
    if (!modelLoaded) return;
    
    // 缩放因子
    float zoomFactor = 1.0f + event->angleDelta().y() * 0.001f;
    scaleFactor = qBound(0.1f, scaleFactor * zoomFactor, 10.0f);
    
    update();
}