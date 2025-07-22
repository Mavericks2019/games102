#include "../glwidget/glwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>

// 创建CVT选项卡
QWidget* createCVTTab(GLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    // 创建专用视图
    GLWidget *cvtView = new GLWidget;
    layout->addWidget(cvtView);
    
    // 保存视图指针
    tab->setProperty("cvtGLWidget", QVariant::fromValue(cvtView));
    
    // 连接标签切换信号
    QObject::connect(tab, &QWidget::show, [cvtView]() {
        cvtView->setCVTView(true);
        cvtView->resetViewForParameterization(); // 重置视图
        cvtView->update();
    });
    
    return tab;
}

// 创建CVT控制面板
QWidget* createCVTControlPanel(GLWidget* glWidget, QWidget* cvtTab) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 获取视图
    GLWidget *cvtView = cvtTab->property("cvtGLWidget").value<GLWidget*>();
    
    // 创建点数设置控件
    QGroupBox *pointGroup = new QGroupBox("Point Settings");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    // 点数输入框
    QHBoxLayout *countLayout = new QHBoxLayout();
    QLabel *countLabel = new QLabel("Points Count:");
    countLabel->setStyleSheet("color: white;");
    QLineEdit *countInput = new QLineEdit();
    countInput->setText("100");
    countInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #3A3A3A;"
        "   color: white;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "}"
    );
    countLayout->addWidget(countLabel);
    countLayout->addWidget(countInput);
    
    pointLayout->addLayout(countLayout);
    
    // 创建按钮
    QPushButton *randomButton = new QPushButton("Random generation");
    
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
    
    // 连接按钮信号
    QObject::connect(randomButton, &QPushButton::clicked, [cvtView, countInput]() {
        bool ok;
        int count = countInput->text().toInt(&ok);
        if (ok && count > 0) {
            cvtView->generateRandomPoints(count);
        }
    });
    
    pointLayout->addWidget(randomButton);
    
    // 添加按钮到布局
    layout->addWidget(pointGroup);
    
    // 添加其他按钮
    QPushButton *delaunayButton = new QPushButton("Delaunay");
    QPushButton *voronoiButton = new QPushButton("Voronoi");
    QPushButton *lloydButton = new QPushButton("Do Lloyd");
    
    delaunayButton->setStyleSheet(buttonStyle);
    voronoiButton->setStyleSheet(buttonStyle);
    lloydButton->setStyleSheet(buttonStyle);
    
    QObject::connect(delaunayButton, &QPushButton::clicked, [cvtView]() {
        cvtView->computeDelaunayTriangulation();
    });
    
    QObject::connect(voronoiButton, &QPushButton::clicked, [cvtView]() {
        cvtView->computeVoronoiDiagram();
    });
    
    QObject::connect(lloydButton, &QPushButton::clicked, [cvtView]() {
        cvtView->performLloydRelaxation();
    });
    
    layout->addWidget(delaunayButton);
    layout->addWidget(voronoiButton);
    layout->addWidget(lloydButton);
    
    layout->addStretch();
    return panel;
}