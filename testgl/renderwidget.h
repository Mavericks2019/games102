#ifndef RENDERWIDGET_H
#define RENDERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QBasicTimer>

class RenderWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    RenderWidget(QWidget *parent = nullptr);
    ~RenderWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void timerEvent(QTimerEvent *e) override;

private:
    void initShaders();
    void initScene();
    void createQuad();
    void renderQuad();

    QOpenGLShaderProgram computeShader;
    QOpenGLShaderProgram quadShader;
    
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
    GLuint textureID = 0;
    
    QBasicTimer timer;
    int frameCount = 0;
    float aspectRatio = 1.0f;
};

#endif // RENDERWIDGET_H