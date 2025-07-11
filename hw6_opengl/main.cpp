#include "glwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QStyleFactory>
#include <QPalette>
#include <QGroupBox>
#include <QFormLayout>
#include <QStackedLayout>
#include <QTabWidget>
#include <QFileInfo>
#include <QSlider>
#include <QColorDialog>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置Fusion样式
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 设置深色主题
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(palette);
    
    // 设置字体
    QFont defaultFont("Arial", 12);
    app.setFont(defaultFont);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.resize(1920, 1080);
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(&mainWindow);
    
    // 创建OpenGL窗口
    GLWidget *glWidget = new GLWidget;
    
    // 创建TabWidget
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->addTab(glWidget, "OBJ Model");
    mainLayout->addWidget(tabWidget, 5);
    
    // 创建右侧控制面板
    QWidget *controlPanel = new QWidget;
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(400);
    
    // 点信息组
    QGroupBox *pointGroup = new QGroupBox("Model Information");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    QLabel *pointInfoLabel = new QLabel("No model loaded");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(50);
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    pointInfoLabel->setWordWrap(true);
    
    pointLayout->addWidget(pointInfoLabel);
    controlLayout->addWidget(pointGroup);
    
    // 创建堆叠布局用于切换控制面板
    QStackedLayout *stackedControlLayout = new QStackedLayout;
    controlLayout->addLayout(stackedControlLayout);
    
    // ==== 新增：颜色设置组 ====
    QGroupBox *colorGroup = new QGroupBox("Color Settings");
    QVBoxLayout *colorLayout = new QVBoxLayout(colorGroup);
    colorLayout->setSpacing(10);
    
    // 背景颜色按钮
    QPushButton *bgColorButton = new QPushButton("Change Background Color");
    bgColorButton->setStyleSheet(
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
    QObject::connect(bgColorButton, &QPushButton::clicked, [glWidget, &mainWindow]() {
        QColor color = QColorDialog::getColor(Qt::black, &mainWindow, "Select Background Color");
        if (color.isValid()) {
            glWidget->setBackgroundColor(color);
        }
    });
    colorLayout->addWidget(bgColorButton);
    
    // 线框颜色按钮
    QPushButton *lineColorButton = new QPushButton("Change Wireframe Color");
    lineColorButton->setStyleSheet(
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
    QObject::connect(lineColorButton, &QPushButton::clicked, [glWidget, &mainWindow]() {
        QColor color = QColorDialog::getColor(Qt::red, &mainWindow, "Select Wireframe Color");
        if (color.isValid()) {
            glWidget->setWireframeColor(QVector4D(
                color.redF(), 
                color.greenF(), 
                color.blueF(), 
                1.0f
            ));
        }
    });
    colorLayout->addWidget(lineColorButton);
    
    // 新增：表面颜色按钮
    QPushButton *surfaceColorButton = new QPushButton("Change Surface Color");
    surfaceColorButton->setStyleSheet(
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
    QObject::connect(surfaceColorButton, &QPushButton::clicked, [glWidget, &mainWindow]() {
        QColor color = QColorDialog::getColor(QColor(179, 179, 204), &mainWindow, "Select Surface Color");
        if (color.isValid()) {
            glWidget->setSurfaceColor(QVector3D(
                color.redF(), 
                color.greenF(), 
                color.blueF()
            ));
        }
    });
    colorLayout->addWidget(surfaceColorButton);
    
    // 新增：关闭高光复选框
    QCheckBox *disableSpecularCheckbox = new QCheckBox("Disable Specular Highlight");
    disableSpecularCheckbox->setStyleSheet("color: white;");
    QObject::connect(disableSpecularCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setSpecularEnabled(state != Qt::Checked);
    });
    colorLayout->addWidget(disableSpecularCheckbox);
    
    colorLayout->addStretch();
    controlLayout->addWidget(colorGroup);
    // ==== 结束颜色设置组 ====
    
    // 在控制面板添加新功能
    QGroupBox *subdivGroup = new QGroupBox("Loop Subdivision");
    QVBoxLayout *subdivLayout = new QVBoxLayout(subdivGroup);
    
    // 细分级别控制
    QSpinBox *subdivLevelSpin = new QSpinBox;
    subdivLevelSpin->setRange(1, 5);
    subdivLevelSpin->setValue(1);
    QObject::connect(subdivLevelSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                     glWidget, &GLWidget::setSubdivisionLevel);
    
    // 应用细分按钮
    QPushButton *applySubdivBtn = new QPushButton("Apply Subdivision");
    applySubdivBtn->setStyleSheet(
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
    QObject::connect(applySubdivBtn, &QPushButton::clicked, [glWidget]() {
        glWidget->performLoopSubdivision();
    });
    
    subdivLayout->addWidget(new QLabel("Subdivision Level:"));
    subdivLayout->addWidget(subdivLevelSpin);
    subdivLayout->addWidget(applySubdivBtn);
    
    // 网格简化组
    QGroupBox *simplifyGroup = new QGroupBox("Mesh Simplification");
    QVBoxLayout *simplifyLayout = new QVBoxLayout(simplifyGroup);
    
    // 简化比例控制
    QSlider *simplifySlider = new QSlider(Qt::Horizontal);
    simplifySlider->setRange(10, 90);
    simplifySlider->setValue(50);
    QLabel *simplifyLabel = new QLabel("Simplification: 50%");
    
    QObject::connect(simplifySlider, &QSlider::valueChanged, 
                     [simplifyLabel, glWidget](int value) {
        float ratio = value / 100.0f;
        simplifyLabel->setText(QString("Simplification: %1%").arg(value));
        glWidget->setSimplificationRatio(ratio);
    });
    
    // 应用简化按钮
    QPushButton *applySimplifyBtn = new QPushButton("Apply Simplification");
    applySimplifyBtn->setStyleSheet(
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
    QObject::connect(applySimplifyBtn, &QPushButton::clicked, [glWidget]() {
        glWidget->performMeshSimplification(glWidget->simplificationRatio);
    });
    
    simplifyLayout->addWidget(simplifyLabel);
    simplifyLayout->addWidget(simplifySlider);
    simplifyLayout->addWidget(applySimplifyBtn);
    
    // 添加到控制面板
    controlLayout->addWidget(subdivGroup);
    controlLayout->addWidget(simplifyGroup);
    
    // 添加OBJ控制面板
    QWidget *objControlPanel = new QWidget;
    QVBoxLayout *objControlLayout = new QVBoxLayout(objControlPanel);
    
    // 添加OBJ加载按钮
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
    QObject::connect(loadButton, &QPushButton::clicked, [glWidget, pointInfoLabel, &mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            &mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            glWidget->loadOBJ(filePath);
            pointInfoLabel->setText("Model loaded: " + QFileInfo(filePath).fileName());
            mainWindow.setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName());
        }
    });
    objControlLayout->addWidget(loadButton);
    
    // 创建渲染模式选择组
    QGroupBox *renderModeGroup = new QGroupBox("Rendering Mode");
    QVBoxLayout *renderModeLayout = new QVBoxLayout(renderModeGroup);
    
    // 删除Wireframe单选按钮
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
    QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
    QRadioButton *maxRadio = new QRadioButton("Max Curvature");
    
    solidRadio->setChecked(true);  // 默认选择实体模式
    
    // 添加到布局
    renderModeLayout->addWidget(solidRadio);
    renderModeLayout->addWidget(gaussianRadio);
    renderModeLayout->addWidget(meanRadio);
    renderModeLayout->addWidget(maxRadio);
    
    // 连接信号
    QObject::connect(solidRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::BlinnPhong);
    });
    
    QObject::connect(gaussianRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::GaussianCurvature);
    });
    
    QObject::connect(meanRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::MeanCurvature);
    });
    
    QObject::connect(maxRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::MaxCurvature);
    });
    
    // 添加到控制面板
    objControlLayout->addWidget(renderModeGroup);
    
    // 添加线框叠加选项
    QCheckBox *wireframeOverlayCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeOverlayCheckbox->setStyleSheet("color: white;");
    QObject::connect(wireframeOverlayCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setShowWireframeOverlay(state == Qt::Checked);
    });
    objControlLayout->addWidget(wireframeOverlayCheckbox);
    
    // 新增：添加隐藏面选项
    QCheckBox *hideFacesCheckbox = new QCheckBox("Hide Faces");
    hideFacesCheckbox->setStyleSheet("color: white;");
    QObject::connect(hideFacesCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setHideFaces(state == Qt::Checked);
    });
    objControlLayout->addWidget(hideFacesCheckbox);
    
    // 添加重置视图按钮
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet(
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
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget]() {
        glWidget->resetView();
    });
    objControlLayout->addWidget(resetButton);
    
    // 添加迭代方法选择组
    QGroupBox *methodGroup = new QGroupBox("Iteration Method");
    methodGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    
    QRadioButton *uniformRadio = new QRadioButton("Uniform Laplacian");
    QRadioButton *cotangentRadio = new QRadioButton("Cotangent Weights");
    QRadioButton *cotangentAreaRadio = new QRadioButton("Cotangent with Area (Laplace-Beltrami)"); 
    
    // 默认选择余切权重
    uniformRadio->setChecked(true);
    
    methodLayout->addWidget(uniformRadio);
    methodLayout->addWidget(cotangentRadio);
    methodLayout->addWidget(cotangentAreaRadio); // 新增
    
    // 连接信号
    QObject::connect(uniformRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setIterationMethod(GLWidget::UniformLaplacian);
    });
    QObject::connect(cotangentRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setIterationMethod(GLWidget::CotangentWeights);
    });
    QObject::connect(cotangentAreaRadio, &QRadioButton::clicked, [glWidget]() { // 新增
        glWidget->setIterationMethod(GLWidget::CotangentWithArea);
    });
    
    // 添加到控制面板
    objControlLayout->addWidget(methodGroup);
    
    // 添加极小曲面迭代控制组
    QGroupBox *minimalSurfaceGroup = new QGroupBox("Minimal Surface Iteration");
    minimalSurfaceGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QFormLayout *minimalLayout = new QFormLayout(minimalSurfaceGroup);
    
    // 迭代次数
    QSpinBox *iterationsSpinBox = new QSpinBox;
    iterationsSpinBox->setRange(1, 1000);
    iterationsSpinBox->setValue(10);
    minimalLayout->addRow("Iterations:", iterationsSpinBox);
    
    // 步长参数
    QDoubleSpinBox *lambdaSpinBox = new QDoubleSpinBox;
    lambdaSpinBox->setRange(0.0001, 0.5);
    lambdaSpinBox->setValue(0.1);
    lambdaSpinBox->setSingleStep(0.01);
    lambdaSpinBox->setDecimals(4);
    minimalLayout->addRow("Step Size (λ):", lambdaSpinBox);
    
    // 应用迭代按钮
    QPushButton *applyIterationButton = new QPushButton("Apply Iteration");
    applyIterationButton->setStyleSheet(
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
    QObject::connect(applyIterationButton, &QPushButton::clicked, [glWidget, iterationsSpinBox, lambdaSpinBox]() {
        glWidget->performMinimalSurfaceIteration(
            iterationsSpinBox->value(),
            lambdaSpinBox->value()
        );
    });
    minimalLayout->addRow(applyIterationButton);
    
    objControlLayout->addWidget(minimalSurfaceGroup);
    
    objControlLayout->addStretch();
    
    // 将OBJ控制面板添加到堆叠布局
    stackedControlLayout->addWidget(objControlPanel);
    
    // 将控制面板添加到主布局
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}