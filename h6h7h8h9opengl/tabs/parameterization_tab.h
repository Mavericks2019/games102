#include "../glwidget/glwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSplitter>
#include <QFileDialog>
#include <QLabel>
#include <QFileInfo>

// 声明UIUtils命名空间中的函数
namespace UIUtils {
    GLWidget* getCurrentGLWidget(GLWidget* mainGLWidget, QTabWidget* tabWidget);
}

// 创建参数化视图选项卡
QWidget* createParameterizationTab(GLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    // 使用分割器创建左右视图
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // 创建专用视图
    GLWidget *leftView = new GLWidget;
    GLWidget *rightView = new GLWidget;
    
    // 设置右侧视图为参数化视图（禁止旋转）
    rightView->isParameterizationView = false;
    
    // 添加视图
    splitter->addWidget(leftView);
    splitter->addWidget(rightView);
    splitter->setSizes(QList<int>() << 500 << 500);
    
    layout->addWidget(splitter);
    
    // 保存视图指针
    tab->setProperty("leftGLWidget", QVariant::fromValue(leftView));
    tab->setProperty("rightGLWidget", QVariant::fromValue(rightView));
    
    return tab;
}

// 创建显示选项组（双视图）
QGroupBox* createDisplayOptionsGroupForDualView(GLWidget* leftView, GLWidget* rightView) {
    QGroupBox *group = new QGroupBox("Display Options");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeCheckbox->setStyleSheet("color: white;");
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [leftView, rightView](int state) {
        leftView->setShowWireframeOverlay(state == Qt::Checked);
        rightView->setShowWireframeOverlay(state == Qt::Checked);
    });
    
    QCheckBox *faceCheckbox = new QCheckBox("Hide Faces");
    faceCheckbox->setStyleSheet("color: white;");
    QObject::connect(faceCheckbox, &QCheckBox::stateChanged, [leftView, rightView](int state) {
        leftView->setHideFaces(state == Qt::Checked);
        rightView->setHideFaces(state == Qt::Checked);
    });
    
    layout->addWidget(wireframeCheckbox);
    layout->addWidget(faceCheckbox);
    return group;
}

// 创建渲染模式组（双视图）
QGroupBox* createRenderingModeGroupForDualView(GLWidget* leftView, GLWidget* rightView) {
    QGroupBox *group = new QGroupBox("Rendering Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
    QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
    QRadioButton *maxRadio = new QRadioButton("Max Curvature");
    QRadioButton *textureRadio = new QRadioButton("Texture Mapping");
    
    solidRadio->setChecked(true);
    
    layout->addWidget(solidRadio);
    layout->addWidget(gaussianRadio);
    layout->addWidget(meanRadio);
    layout->addWidget(maxRadio);
    layout->addWidget(textureRadio);
    
    // 连接信号：同时设置左右视图
    auto connectMode = [leftView, rightView](QRadioButton* radio, GLWidget::RenderMode mode) {
        QObject::connect(radio, &QRadioButton::clicked, [leftView, rightView, mode]() {
            leftView->setRenderMode(mode);
            rightView->setRenderMode(mode);
        });
    };
    
    connectMode(solidRadio, GLWidget::BlinnPhong);
    connectMode(gaussianRadio, GLWidget::GaussianCurvature);
    connectMode(meanRadio, GLWidget::MeanCurvature);
    connectMode(maxRadio, GLWidget::MaxCurvature);
    connectMode(textureRadio, GLWidget::TextureMapping);
    
    return group;
}

// 创建参数化控制面板
QWidget* createParameterizationControlPanel(GLWidget* glWidget, QWidget* paramTab) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 获取视图
    GLWidget *leftView = paramTab->property("leftGLWidget").value<GLWidget*>();
    GLWidget *rightView = paramTab->property("rightGLWidget").value<GLWidget*>();
    
    // 添加加载按钮
    QPushButton *loadButton = new QPushButton("Load OBJ File");
    loadButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(loadButton, &QPushButton::clicked, [rightView, leftView]() {
        QString filePath = QFileDialog::getOpenFileName(
            nullptr, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            rightView->loadOBJ(filePath);
            leftView->loadOBJ(filePath);
        }
    });
    layout->addWidget(loadButton);
    
    // 添加渲染模式组（双视图）
    layout->addWidget(createRenderingModeGroupForDualView(leftView, rightView));
    
    // 添加显示选项组（双视图）
    layout->addWidget(createDisplayOptionsGroupForDualView(leftView, rightView));
    
    // 添加边界选项
    QGroupBox *boundaryGroup = new QGroupBox("Boundary Type");
    boundaryGroup->setStyleSheet("QGroupBox { color: white; }");
    QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryGroup);
    
    QRadioButton *rectRadio = new QRadioButton("Rectangle");
    QRadioButton *circleRadio = new QRadioButton("Circle");
    rectRadio->setChecked(true);

    // 连接边界选项信号
    QObject::connect(rectRadio, &QRadioButton::clicked, [rightView]() {
        rightView->setBoundaryType(GLWidget::Rectangle);
    });
    QObject::connect(circleRadio, &QRadioButton::clicked, [rightView]() {
        rightView->setBoundaryType(GLWidget::Circle);
    });
    
    boundaryLayout->addWidget(rectRadio);
    boundaryLayout->addWidget(circleRadio);
    layout->addWidget(boundaryGroup);
    
    // 添加参数化按钮
    QPushButton *paramButton = new QPushButton("Perform Parameterization");
    paramButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );

    // 连接参数化按钮信号
    QObject::connect(paramButton, &QPushButton::clicked, [glWidget, paramTab]() {
        GLWidget* leftView = paramTab->property("leftGLWidget").value<GLWidget*>();
        GLWidget* rightView = paramTab->property("rightGLWidget").value<GLWidget*>();
        
        // 在右视图执行参数化
        rightView->performParameterization();
        
        // 将右视图的参数化纹理坐标传递给左视图
        leftView->setParameterizationTexCoords(rightView->paramTexCoords);
    });
    
    layout->addWidget(paramButton);
    
    // 添加一个拉伸因子，但确保它不会使控件过度延伸
    layout->addStretch(1);

    return panel;
}