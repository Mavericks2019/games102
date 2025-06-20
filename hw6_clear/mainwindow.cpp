#include "mainwindow.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QFileDialog>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("OBJ Model Viewer");
    resize(1920, 1080);
    
    // 初始化曲线颜色
    curveColors["OBJ Model"] = Qt::darkGray;
    
    // 创建主部件和布局
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // ========== 左侧画布区域 ==========
    tabWidget = new QTabWidget(this);
    
    objModelCanvas = new ObjModelCanvas;
    tabWidget->addTab(objModelCanvas, "OBJ Model");
    
    mainLayout->addWidget(tabWidget, 5);
    
    // ========== 右侧控制面板 ==========
    controlPanel = new QWidget;
    controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(400);
    
    // 点信息
    QGroupBox *pointGroup = new QGroupBox("Point Information");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    pointInfoLabel = new QLabel("Hover over a point to see coordinates");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(50);
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");    pointInfoLabel->setWordWrap(true);
    
    pointLayout->addWidget(pointInfoLabel);
    controlLayout->addWidget(pointGroup);
    
    // 特定控制面板使用堆叠布局
    stackedControlLayout = new QStackedLayout;
    controlLayout->addLayout(stackedControlLayout);
    
    // 为OBJ模型创建控制面板
    setupObjControls();
    
    // 清除按钮
    QPushButton *clearButton = new QPushButton("Clear Points");
    clearButton->setStyleSheet("background-color: #505050; color: white;");
    connect(clearButton, &QPushButton::clicked, this, [this](){
        objModelCanvas->clearPoints();
    });
    controlLayout->addWidget(clearButton);
    
    mainLayout->addWidget(controlPanel);
    
    setCentralWidget(centralWidget);
    
    // 初始设置
    stackedControlLayout->setCurrentIndex(0);
    objModelCanvas->setCurveColor(curveColors["OBJ Model"]);
    
    // 连接信号
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateCanvasView);
    connect(objModelCanvas, &BaseCanvasWidget::pointHovered, this, &MainWindow::updatePointInfo);
    connect(objModelCanvas, &BaseCanvasWidget::noPointHovered, this, &MainWindow::clearPointInfo);
    connect(objModelCanvas, &BaseCanvasWidget::pointDeleted, this, &MainWindow::showDeleteMessage);
    
    // 删除消息计时器
    deleteMessageTimer = new QTimer(this);
    deleteMessageTimer->setSingleShot(true);
    connect(deleteMessageTimer, &QTimer::timeout, this, &MainWindow::clearPointInfo);
}

void MainWindow::setupObjControls() {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);

    // 创建OBJ信息标签
    objInfoLabel = new QLabel("No OBJ model loaded");
    objInfoLabel->setAlignment(Qt::AlignCenter);
    objInfoLabel->setFixedHeight(50);
    objInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");    objInfoLabel->setWordWrap(true);
    layout->addWidget(objInfoLabel);

    // 添加OBJ加载按钮
    QPushButton *loadButton = new QPushButton("Load OBJ File");
    loadButton->setStyleSheet("background-color: #505050; color: white;");
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadObjModel);
    
    // 添加重置视图按钮
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet("background-color: #505050; color: white;");
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::resetObjView);
    
    // 添加显示面的复选框
    QCheckBox *showFacesCheckbox = new QCheckBox("Show Faces (Fill Polygons)");
    showFacesCheckbox->setChecked(false); // 默认选中
    showFacesCheckbox->setStyleSheet("color: white;");
    connect(showFacesCheckbox, &QCheckBox::toggled, this, &MainWindow::toggleShowFaces);
    
    // 绘制模式选择
    QLabel *drawModeLabel = new QLabel("Draw Mode:");
    drawModeLabel->setStyleSheet("color: white;");
    drawModeComboBox = new QComboBox();
    drawModeComboBox->addItem("Triangles", 0); // Triangles 对应 0
    drawModeComboBox->addItem("Pixels", 1);    // Pixels 对应 1
    connect(drawModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::setDrawMode);
    
    // 新增：背景颜色按钮
    bgColorButton = new QPushButton("Change Background Color");
    bgColorButton->setStyleSheet("background-color: #505050; color: white;");
    connect(bgColorButton, &QPushButton::clicked, this, &MainWindow::changeBackgroundColor);
    
    // 添加光照控制
    QGroupBox *lightGroup = new QGroupBox("Lighting Controls");
    lightGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QFormLayout *lightLayout = new QFormLayout(lightGroup);
    
    // 环境光强度滑块
    ambientSlider = new QSlider(Qt::Horizontal);
    ambientSlider->setRange(0, 100);
    ambientSlider->setValue(30); // 30% 环境光
    connect(ambientSlider, &QSlider::valueChanged, this, &MainWindow::updateAmbientIntensity);
    lightLayout->addRow("Ambient:", ambientSlider);
    
    // 漫反射强度滑块
    diffuseSlider = new QSlider(Qt::Horizontal);
    diffuseSlider->setRange(0, 100);
    diffuseSlider->setValue(70); // 70% 漫反射
    connect(diffuseSlider, &QSlider::valueChanged, this, &MainWindow::updateDiffuseIntensity);
    lightLayout->addRow("Diffuse:", diffuseSlider);
    
    // 镜面反射强度滑块
    specularSlider = new QSlider(Qt::Horizontal);
    specularSlider->setRange(0, 100);
    specularSlider->setValue(40); // 40% 镜面反射
    connect(specularSlider, &QSlider::valueChanged, this, &MainWindow::updateSpecularIntensity);
    lightLayout->addRow("Specular:", specularSlider);
    
    // 高光指数滑块
    shininessSlider = new QSlider(Qt::Horizontal);
    shininessSlider->setRange(1, 256);
    shininessSlider->setValue(32); // 默认高光指数
    connect(shininessSlider, &QSlider::valueChanged, this, &MainWindow::updateShininess);
    lightLayout->addRow("Shininess:", shininessSlider);
    
    layout->addWidget(loadButton);
    layout->addWidget(resetButton);
    layout->addWidget(showFacesCheckbox);
    
    // 添加绘制模式控件
    QHBoxLayout *drawModeLayout = new QHBoxLayout();
    drawModeLayout->addWidget(drawModeLabel);
    drawModeLayout->addWidget(drawModeComboBox);
    layout->addLayout(drawModeLayout);
    
    // 添加背景颜色按钮
    layout->addWidget(bgColorButton);
    
    layout->addWidget(lightGroup);
    
    // 添加使用说明
    QLabel *infoLabel = new QLabel(
        "<b>Instructions:</b><br>"
        "• After loading an OBJ model, it will be centered automatically<br>"
        "• Mouse drag: Rotate the model<br>"
        "• Mouse wheel: Zoom in/out<br>"
        "• Reset View: Restore the initial view<br>"
        "• Lighting controls: Adjust the appearance<br>"
        "• Draw Mode: Triangles (faster) or Pixels (slower but more accurate)<br>"
        "• Change Background: Click the button to set canvas background color"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    stackedControlLayout->addWidget(panel);
}


// 设置绘制模式
// 设置绘制模式
void MainWindow::setDrawMode(int index) {
    int modeValue = drawModeComboBox->itemData(index).toInt();
    
    // 将整数转换为枚举值
    ObjModelCanvas::DrawMode mode;
    if (modeValue == 0) {
        mode = ObjModelCanvas::Triangles;
    } else if (modeValue == 1) {
        mode = ObjModelCanvas::Pixels;
    } else {
        mode = ObjModelCanvas::Triangles;
    }
    
    objModelCanvas->setDrawMode(mode);
}

void MainWindow::changeBackgroundColor() {
    QColor color = QColorDialog::getColor(objModelCanvas->backgroundColor, this, "Select Background Color");
    if (color.isValid()) {
        objModelCanvas->setBackgroundColor(color);
        
        // 更新按钮文本颜色提示
        QString style = QString("background-color: %1; color: %2;")
            .arg(color.name())
            .arg(color.lightness() > 128 ? "black" : "white");
        bgColorButton->setStyleSheet(style);
    }
}


void MainWindow::updateAmbientIntensity(int value) {
    objModelCanvas->ambientIntensity = value / 100.0f;
    objModelCanvas->update();
}

// 更新漫反射强度
void MainWindow::updateDiffuseIntensity(int value) {
    objModelCanvas->diffuseIntensity = value / 100.0f;
    objModelCanvas->update();
}

// 更新镜面反射强度
void MainWindow::updateSpecularIntensity(int value) {
    objModelCanvas->specularIntensity = value / 100.0f;
    objModelCanvas->update();
}

// 新增：更新高光指数
void MainWindow::updateShininess(int value) {
    objModelCanvas->shininess = static_cast<float>(value);
    objModelCanvas->updateFaceColors();  // 更新所有面的颜色
    objModelCanvas->update();
}

// 新增：切换显示面的槽函数
void MainWindow::toggleShowFaces(bool show)
{
    objModelCanvas->setShowFaces(show);
}

void MainWindow::loadObjModel()
{
    // 获取当前工作目录
    QString currentDir = QDir::currentPath();
    
    // 尝试在当前目录下查找OBJ文件
    QDir dir(currentDir);
    QStringList objFiles = dir.entryList(QStringList() << "*.obj", QDir::Files);
    
    // 如果有OBJ文件，使用第一个作为默认选择
    QString defaultFile;
    if (!objFiles.isEmpty()) {
        defaultFile = dir.absoluteFilePath(objFiles.first());
    }
    
    QString filePath = QFileDialog::getOpenFileName(this, 
        "Open OBJ File", 
        defaultFile.isEmpty() ? currentDir : defaultFile,  // 默认选择文件或目录
        "OBJ Files (*.obj);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        objModelCanvas->loadObjFile(filePath);
    }
}

void MainWindow::updateCanvasView(int index)
{
    // 设置曲线颜色
    objModelCanvas->setCurveColor(curveColors["OBJ Model"]);
    
    // 连接当前画布的信号
    connect(objModelCanvas, &BaseCanvasWidget::pointHovered, 
            this, &MainWindow::updatePointInfo, Qt::UniqueConnection);
    connect(objModelCanvas, &BaseCanvasWidget::noPointHovered, 
            this, &MainWindow::clearPointInfo, Qt::UniqueConnection);
    connect(objModelCanvas, &BaseCanvasWidget::pointDeleted, 
            this, &MainWindow::showDeleteMessage, Qt::UniqueConnection);
}

void MainWindow::updatePointInfo(const QPointF& point)
{
    // 转换为数学坐标
    double mathY = objModelCanvas->height() - point.y();
    
    // 紧凑单行格式
    QString info = QString("Screen: (%1, %2) \nMath: (%3, %4)")
                  .arg(point.x(), 0, 'f', 1)
                  .arg(point.y(), 0, 'f', 1)
                  .arg(point.x(), 0, 'f', 1)
                  .arg(mathY, 0, 'f', 1);
    
    pointInfoLabel->setText(info);
    pointInfoLabel->setStyleSheet("background-color: #2A4A6A; color: white; border-radius: 5px; padding: 5px;");
}

void MainWindow::clearPointInfo()
{
    pointInfoLabel->setText("Hover over a point to see coordinates");
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
}

void MainWindow::showDeleteMessage()
{
    pointInfoLabel->setText("Point deleted");
    pointInfoLabel->setStyleSheet("background-color: #6A2A2A; color: white; border-radius: 5px; padding: 5px;");
    
    // 2秒后恢复
    deleteMessageTimer->start(2000);
}

void MainWindow::resetObjView()
{
    objModelCanvas->resetView();
}