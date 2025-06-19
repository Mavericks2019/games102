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
    resize(1400, 800);
    
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
    controlPanel->setFixedWidth(300);
    
    // 点信息
    QGroupBox *pointGroup = new QGroupBox("Point Information");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    pointInfoLabel = new QLabel("Hover over a point to see coordinates");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(50);
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
    pointInfoLabel->setWordWrap(true);
    
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
    objInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
    objInfoLabel->setWordWrap(true);
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
    showFacesCheckbox = new QCheckBox("Show Faces (Fill Polygons)");
    showFacesCheckbox->setChecked(true); // 默认选中
    showFacesCheckbox->setStyleSheet("color: white;");
    connect(showFacesCheckbox, &QCheckBox::toggled, this, &MainWindow::toggleShowFaces);
    
    layout->addWidget(loadButton);
    layout->addWidget(resetButton);
    layout->addWidget(showFacesCheckbox);  // 添加复选框到布局
    
    // 添加使用说明
    QLabel *infoLabel = new QLabel(
        "<b>Instructions:</b><br>"
        "• After loading an OBJ model, it will be centered automatically<br>"
        "• Mouse drag: Rotate the model<br>"
        "• Mouse wheel: Zoom in/out<br>"
        "• Reset View: Restore the initial view"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    stackedControlLayout->addWidget(panel);
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