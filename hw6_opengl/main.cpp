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
    
    // 添加线框颜色选择按钮
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
    objControlLayout->addWidget(lineColorButton);
    
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
    lambdaSpinBox->setRange(0.0001, 0.1);
    lambdaSpinBox->setValue(0.01);
    lambdaSpinBox->setSingleStep(0.001);
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
    
    // 添加使用说明
    QLabel *infoLabel = new QLabel(
        "<b>Instructions:</b><br>"
        "• Load an OBJ model using the button above<br>"
        "• Mouse drag: Rotate the model<br>"
        "• Mouse wheel: Zoom in/out<br>"
        "• Arrow keys: Fine-tune rotation<br>"
        "• '+'/'-' keys: Zoom in/out<br>"
        "• 'R' key: Reset view<br>"
        "• Switch between solid rendering and curvature visualizations<br>"
        "• Enable wireframe overlay to see model structure<br>"
        "• Use 'Hide Faces' to show only wireframe"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    objControlLayout->addWidget(infoLabel);
    
    // 添加光照控制组
    QGroupBox *lightGroup = new QGroupBox("Lighting Controls");
    lightGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QFormLayout *lightLayout = new QFormLayout(lightGroup);
    
    // 环境光强度滑块
    QSlider *ambientSlider = new QSlider(Qt::Horizontal);
    ambientSlider->setRange(0, 100);
    ambientSlider->setValue(30);
    lightLayout->addRow("Ambient:", ambientSlider);
    
    // 漫反射强度滑块
    QSlider *diffuseSlider = new QSlider(Qt::Horizontal);
    diffuseSlider->setRange(0, 100);
    diffuseSlider->setValue(70);
    lightLayout->addRow("Diffuse:", diffuseSlider);
    
    // 镜面反射强度滑块
    QSlider *specularSlider = new QSlider(Qt::Horizontal);
    specularSlider->setRange(0, 100);
    specularSlider->setValue(40);
    lightLayout->addRow("Specular:", specularSlider);
    
    // 高光指数滑块
    QSlider *shininessSlider = new QSlider(Qt::Horizontal);
    shininessSlider->setRange(1, 256);
    shininessSlider->setValue(32);
    lightLayout->addRow("Shininess:", shininessSlider);
    
    objControlLayout->addWidget(lightGroup);
    
    // 添加背景颜色按钮
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
    objControlLayout->addWidget(bgColorButton);
    
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