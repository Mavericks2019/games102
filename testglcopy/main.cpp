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
    
    QRadioButton *wireframeRadio = new QRadioButton("Wireframe");
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
    QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
    QRadioButton *maxRadio = new QRadioButton("Max Curvature");
    
    wireframeRadio->setChecked(true);
    
    // 添加到布局
    renderModeLayout->addWidget(wireframeRadio);
    renderModeLayout->addWidget(solidRadio);
    renderModeLayout->addWidget(gaussianRadio);
    renderModeLayout->addWidget(meanRadio);
    renderModeLayout->addWidget(maxRadio);
    
    // 连接信号
    QObject::connect(wireframeRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::Wireframe);
    });
    
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
    
    // 添加使用说明
    QLabel *infoLabel = new QLabel(
        "<b>Instructions:</b><br>"
        "• Load an OBJ model using the button above<br>"
        "• Mouse drag: Rotate the model<br>"
        "• Mouse wheel: Zoom in/out<br>"
        "• Arrow keys: Fine-tune rotation<br>"
        "• '+'/'-' keys: Zoom in/out<br>"
        "• 'R' key: Reset view<br>"
        "• Switch between wireframe and solid rendering"
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