#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QResizeEvent>
#include <QVBoxLayout>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    GLWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {}

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 设置背景为黑色
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
        // 获取窗口尺寸
        int width = this->width();
        int height = this->height();
        int minSize = qMin(width, height); // 取宽高中的最小值
        
        // 设置正交投影，保持坐标比例
        if (width > height) {
            float aspect = (float)width / (float)height;
            glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
        } else {
            float aspect = (float)height / (float)width;
            glOrtho(-1.0, 1.0, -aspect, aspect, -1.0, 1.0);
        }
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // 计算正方形边长（归一化坐标）
        float squareSize = 0.5f; // 边长占最小尺寸的一半（视觉上）
        
        // 绘制红色正方形
        glColor3f(1.0f, 0.0f, 0.0f); // 红色
        glBegin(GL_QUADS);
            glVertex2f(-squareSize, -squareSize);
            glVertex2f( squareSize, -squareSize);
            glVertex2f( squareSize,  squareSize);
            glVertex2f(-squareSize,  squareSize);
        glEnd();
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    QWidget window;
    window.setWindowTitle("OpenGL Square in Qt");
    window.resize(600, 600);
    
    GLWidget *glWidget = new GLWidget(&window);
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addWidget(glWidget);
    window.setLayout(layout);
    
    window.show();
    return app.exec();
}