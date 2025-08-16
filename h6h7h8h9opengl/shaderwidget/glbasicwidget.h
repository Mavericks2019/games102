#ifndef GLBASICWIDGET_H
#define GLBASICWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_3_Core>
#include <QTimer>
#include <QFile>
#include <chrono>
#include <QElapsedTimer>
#include <QPainter>
#include <QDebug>

class GLBasicWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core {
    Q_OBJECT
public:
    explicit GLBasicWidget(QWidget* parent = nullptr);
    ~GLBasicWidget();

    // 新增公共方法
    void resetTime();
    void reloadShaders();

signals:
    // 新增信号
    void resolutionChanged(int width, int height);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QOpenGLShaderProgram* program = nullptr;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QTimer* timer = nullptr;
    
    std::chrono::high_resolution_clock::time_point startTime;
    
    bool loadShader(QOpenGLShader::ShaderType type, const QString& filePath);
    
    // 新增变量
    float elapsedTime = 0.0f;
    int currentWidth = 0;
    int currentHeight = 0;
};
#endif // GLBASICWIDGET_H