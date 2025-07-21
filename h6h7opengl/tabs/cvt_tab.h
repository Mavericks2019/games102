#include "../glwidget/glwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>

// 创建CVT选项卡
QWidget* createCVTTab(GLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    // 创建专用视图
    GLWidget *cvtView = new GLWidget;
    layout->addWidget(cvtView);
    
    // 保存视图指针
    tab->setProperty("cvtGLWidget", QVariant::fromValue(cvtView));
    
    return tab;
}

// 创建CVT控制面板
QWidget* createCVTControlPanel(GLWidget* glWidget, QWidget* cvtTab) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 获取视图
    GLWidget *cvtView = cvtTab->property("cvtGLWidget").value<GLWidget*>();
    
    // 创建按钮
    QPushButton *randomButton = new QPushButton("Random generation");
    QPushButton *delaunayButton = new QPushButton("Delaunay");
    QPushButton *voronoiButton = new QPushButton("Voronoi");
    QPushButton *lloydButton = new QPushButton("Do Lloyd");
    
    // 设置按钮样式
    QString buttonStyle = 
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "   margin-bottom: 10px;"
        "}"
        "QPushButton:hover { background-color: #606060; }";
    
    randomButton->setStyleSheet(buttonStyle);
    delaunayButton->setStyleSheet(buttonStyle);
    voronoiButton->setStyleSheet(buttonStyle);
    lloydButton->setStyleSheet(buttonStyle);
    
    // 连接按钮信号
    QObject::connect(randomButton, &QPushButton::clicked, [cvtView]() {
        cvtView->generateRandomPoints();
    });
    
    QObject::connect(delaunayButton, &QPushButton::clicked, [cvtView]() {
        cvtView->computeDelaunayTriangulation();
    });
    
    QObject::connect(voronoiButton, &QPushButton::clicked, [cvtView]() {
        cvtView->computeVoronoiDiagram();
    });
    
    QObject::connect(lloydButton, &QPushButton::clicked, [cvtView]() {
        cvtView->performLloydRelaxation();
    });
    
    // 添加按钮到布局
    layout->addWidget(randomButton);
    layout->addWidget(delaunayButton);
    layout->addWidget(voronoiButton);
    layout->addWidget(lloydButton);
    
    layout->addStretch();
    return panel;
}