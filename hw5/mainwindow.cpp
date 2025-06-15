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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      polyInterpCheck(nullptr),
      gaussInterpCheck(nullptr),
      leastSquaresCheck(nullptr),
      ridgeRegCheck(nullptr),
      sigmaSlider(nullptr),
      lambdaSlider(nullptr),
      sigmaValueLabel(nullptr),
      lambdaValueLabel(nullptr)
{
    setWindowTitle("Curve Fitting Visualization");
    resize(1400, 800);
    
    // 初始化曲线颜色
    curveColors["Parametric"] = Qt::blue;
    curveColors["Spline"] = Qt::darkGreen;
    curveColors["Bezier"] = Qt::magenta;
    curveColors["B-Spline"] = Qt::darkCyan;
    curveColors["Chaikin"] = Qt::darkYellow; // 添加新的颜色
    
    // 创建主部件和布局
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // ========== 左侧画布区域 ==========
    tabWidget = new QTabWidget(this);
    
    parametricCanvas = new ParametricCurveCanvas;
    splineCanvas = new CubicSplineCanvas;
    bezierCanvas = new BezierCurveCanvas;
    bSplineCanvas = new BSplineCanvas;
    polygonCanvas = new PolygonCanvas;

    tabWidget->addTab(parametricCanvas, "Parametric Curve");
    tabWidget->addTab(splineCanvas, "Cubic Spline");
    tabWidget->addTab(bezierCanvas, "Bezier Curve");
    tabWidget->addTab(bSplineCanvas, "B-Spline");
    tabWidget->addTab(polygonCanvas, "Chaikin Subdivision"); 
    
    mainLayout->addWidget(tabWidget, 5);
    
    // ========== 右侧控制面板 ==========
    controlPanel = new QWidget;
    controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(300);
    
    // 通用控制
    QGroupBox *generalGroup = new QGroupBox("General Controls");
    QVBoxLayout *generalLayout = new QVBoxLayout(generalGroup);
    
    showCurveCheck = new QCheckBox("Show Curve");
    showCurveCheck->setChecked(true);
    connect(showCurveCheck, &QCheckBox::toggled, this, &MainWindow::toggleCurveVisibility);
    
    showControlPointsCheck = new QCheckBox("Show Control Points");
    showControlPointsCheck->setChecked(true);
    connect(showControlPointsCheck, &QCheckBox::toggled, this, &MainWindow::toggleControlPointsVisibility);
    
    showControlPolygonCheck = new QCheckBox("Show Control Polygon");
    showControlPolygonCheck->setChecked(true);
    connect(showControlPolygonCheck, &QCheckBox::toggled, this, &MainWindow::toggleControlPolygonVisibility);
    
    generalLayout->addWidget(showCurveCheck);
    generalLayout->addWidget(showControlPointsCheck);
    generalLayout->addWidget(showControlPolygonCheck);
    controlLayout->addWidget(generalGroup);
    
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
    
    // 为每种曲线类型创建控制面板
    setupParametricControls();
    setupSplineControls();
    setupBezierControls();
    setupBSplineControls();
    setupChaikinControls();
    
    // 清除按钮
    QPushButton *clearButton = new QPushButton("Clear Points");
    clearButton->setStyleSheet("background-color: #505050; color: white;");
    connect(clearButton, &QPushButton::clicked, this, [this](){
        switch(tabWidget->currentIndex()) {
            case 0: parametricCanvas->clearPoints(); break;
            case 1: splineCanvas->clearPoints(); break;
            case 2: bezierCanvas->clearPoints(); break;
            case 3: bSplineCanvas->clearPoints(); break;
            case 4: polygonCanvas->clearPoints(); break;
        }
    });
    controlLayout->addWidget(clearButton);
    
    mainLayout->addWidget(controlPanel);
    
    setCentralWidget(centralWidget);
    
    // 初始设置
    stackedControlLayout->setCurrentIndex(0);
    parametricCanvas->setCurveColor(curveColors["Parametric"]);
    
    // 连接信号
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateCanvasView);
    connect(parametricCanvas, &BaseCanvasWidget::pointHovered, this, &MainWindow::updatePointInfo);
    connect(parametricCanvas, &BaseCanvasWidget::noPointHovered, this, &MainWindow::clearPointInfo);
    connect(parametricCanvas, &BaseCanvasWidget::pointDeleted, this, &MainWindow::showDeleteMessage);
    
    // 删除消息计时器
    deleteMessageTimer = new QTimer(this);
    deleteMessageTimer->setSingleShot(true);
    connect(deleteMessageTimer, &QTimer::timeout, this, &MainWindow::clearPointInfo);
}

void MainWindow::setupParametricControls()
{
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 参数化方法
    QGroupBox *paramGroup = new QGroupBox("Parameterization Methods");
    QVBoxLayout *paramLayout = new QVBoxLayout(paramGroup);
    
    QButtonGroup *paramButtonGroup = new QButtonGroup(this);
    QRadioButton *uniformParam = new QRadioButton("Uniform");
    QRadioButton *chordalParam = new QRadioButton("Chordal");
    QRadioButton *centripetalParam = new QRadioButton("Centripetal");
    QRadioButton *foleyParam = new QRadioButton("Foley-Nielsen");
    
    paramButtonGroup->addButton(uniformParam, 0);
    paramButtonGroup->addButton(chordalParam, 1);
    paramButtonGroup->addButton(centripetalParam, 2);
    paramButtonGroup->addButton(foleyParam, 3);
    
    uniformParam->setChecked(true);
    
    paramLayout->addWidget(uniformParam);
    paramLayout->addWidget(chordalParam);
    paramLayout->addWidget(centripetalParam);
    paramLayout->addWidget(foleyParam);
    
    layout->addWidget(paramGroup);
    
    connect(paramButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), 
            this, &MainWindow::parameterizationMethodChanged);
    
    // 拟合方法
    QGroupBox *methodGroup = new QGroupBox("Fitting Algorithms");
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    
    polyInterpCheck = new QCheckBox("Polynomial Interpolation");
    connect(polyInterpCheck, &QCheckBox::toggled, parametricCanvas, &ParametricCurveCanvas::togglePolyInterpolation);
    
    gaussInterpCheck = new QCheckBox("Gaussian Interpolation");
    connect(gaussInterpCheck, &QCheckBox::toggled, parametricCanvas, &ParametricCurveCanvas::toggleGaussianInterpolation);
    
    leastSquaresCheck = new QCheckBox("Least Squares Fitting");
    connect(leastSquaresCheck, &QCheckBox::toggled, parametricCanvas, &ParametricCurveCanvas::toggleLeastSquares);
    
    ridgeRegCheck = new QCheckBox("Ridge Regression");
    connect(ridgeRegCheck, &QCheckBox::toggled, parametricCanvas, &ParametricCurveCanvas::toggleRidgeRegression);
    
    methodLayout->addWidget(polyInterpCheck);
    methodLayout->addWidget(gaussInterpCheck);
    methodLayout->addWidget(leastSquaresCheck);
    methodLayout->addWidget(ridgeRegCheck);
    
    layout->addWidget(methodGroup);
    
    // 参数
    QGroupBox *paramControlGroup = new QGroupBox("Parameters");
    QFormLayout *paramControlLayout = new QFormLayout(paramControlGroup);
    
    // 多项式阶数
    degreeSlider = new QSlider(Qt::Horizontal);
    degreeSlider->setRange(1, 20);
    degreeSlider->setValue(3);
    connect(degreeSlider, &QSlider::valueChanged, parametricCanvas, &ParametricCurveCanvas::setPolyDegree);
    connect(degreeSlider, &QSlider::valueChanged, this, &MainWindow::updateDegreeValue);
    
    degreeValueLabel = new QLabel("3");
    degreeValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    degreeValueLabel->setMinimumWidth(30);
    
    paramControlLayout->addRow("Poly Degree:", createSliderLayout(degreeSlider, degreeValueLabel));
    
    // 高斯Sigma
    sigmaSlider = new QSlider(Qt::Horizontal);
    sigmaSlider->setRange(1, 100);
    sigmaSlider->setValue(10);
    connect(sigmaSlider, &QSlider::valueChanged, parametricCanvas, &ParametricCurveCanvas::setGaussianSigma);
    connect(sigmaSlider, &QSlider::valueChanged, this, &MainWindow::updateSigmaValue);
    
    sigmaValueLabel = new QLabel("10.0");
    sigmaValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sigmaValueLabel->setMinimumWidth(30);
    
    paramControlLayout->addRow("Gaussian Sigma:", createSliderLayout(sigmaSlider, sigmaValueLabel));
    
    // Ridge Lambda
    lambdaSlider = new QSlider(Qt::Horizontal);
    lambdaSlider->setRange(1, 100);
    lambdaSlider->setValue(10);
    connect(lambdaSlider, &QSlider::valueChanged, parametricCanvas, &ParametricCurveCanvas::setRidgeLambda);
    connect(lambdaSlider, &QSlider::valueChanged, this, &MainWindow::updateLambdaValue);
    
    lambdaValueLabel = new QLabel("0.10");
    lambdaValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lambdaValueLabel->setMinimumWidth(30);
    
    paramControlLayout->addRow("Ridge Lambda:", createSliderLayout(lambdaSlider, lambdaValueLabel));
    
    layout->addWidget(paramControlGroup);
    
    layout->addStretch();
    
    stackedControlLayout->addWidget(panel);
}

void MainWindow::setupChaikinControls()
{
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    QLabel *infoLabel = new QLabel("Draw a polygon and curves will be automatically generated based on the initial points.");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    // 曲线类型选择
    QGroupBox *curveGroup = new QGroupBox("Curve Type");
    QVBoxLayout *curveLayout = new QVBoxLayout(curveGroup);
    
    QButtonGroup *curveButtonGroup = new QButtonGroup(this);
    QRadioButton *noneButton = new QRadioButton("None");
    QRadioButton *quadraticButton = new QRadioButton("Quadratic Uniform B-Spline");
    QRadioButton *cubicButton = new QRadioButton("Cubic Uniform B-Spline");
    
    curveButtonGroup->addButton(noneButton, 0);
    curveButtonGroup->addButton(quadraticButton, 1);
    curveButtonGroup->addButton(cubicButton, 2);
    
    noneButton->setChecked(true);
    
    curveLayout->addWidget(noneButton);
    curveLayout->addWidget(quadraticButton);
    curveLayout->addWidget(cubicButton);
    
    layout->addWidget(curveGroup);
    
    // 连接信号 - 选择曲线类型后立即更新
    connect(curveButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), 
            [this](int id) {
                PolygonCanvas::CurveType type = static_cast<PolygonCanvas::CurveType>(id);
                polygonCanvas->setCurveType(type);
            });
    // Chaikin细分按钮
    QPushButton *chaikinButton = new QPushButton("Chaikin Subdivision");
    connect(chaikinButton, &QPushButton::clicked, polygonCanvas, &PolygonCanvas::performChaikinSubdivision);
    
    // 添加还原按钮
    QPushButton *restoreButton = new QPushButton("Restore Original");
    connect(restoreButton, &QPushButton::clicked, polygonCanvas, &PolygonCanvas::restoreOriginalPolygon);
    
    // 其他细分方法按钮
    QPushButton *dooSabinButton = new QPushButton("Doo-Sabin (Not Implemented)");
    QPushButton *catmullClarkButton = new QPushButton("Catmull-Clark (Not Implemented)");
    QPushButton *loopButton = new QPushButton("Loop (Not Implemented)");
    
    layout->addWidget(chaikinButton);
    layout->addWidget(restoreButton);
    layout->addWidget(dooSabinButton);
    layout->addWidget(catmullClarkButton);
    layout->addWidget(loopButton);
    
    layout->addStretch();
    
    stackedControlLayout->addWidget(panel);
}

void MainWindow::setupSplineControls()
{
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加三次样条特定的控件
    QLabel *infoLabel = new QLabel("Click on a point to show derivatives. Drag control points to adjust.");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    // 边界条件选择
    QGroupBox *boundaryGroup = new QGroupBox("Boundary Conditions");
    QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryGroup);
    
    QButtonGroup *boundaryButtonGroup = new QButtonGroup(this);
    QRadioButton *naturalBoundary = new QRadioButton("Natural (C2)");
    QRadioButton *clampedBoundary = new QRadioButton("Clamped");
    QRadioButton *notAKnotBoundary = new QRadioButton("Not-a-Knot");
    
    boundaryButtonGroup->addButton(naturalBoundary, 0);
    boundaryButtonGroup->addButton(clampedBoundary, 1);
    boundaryButtonGroup->addButton(notAKnotBoundary, 2);
    
    naturalBoundary->setChecked(true);
    
    boundaryLayout->addWidget(naturalBoundary);
    boundaryLayout->addWidget(clampedBoundary);
    boundaryLayout->addWidget(notAKnotBoundary);
    
    layout->addWidget(boundaryGroup);
    
    // 张力参数
    QGroupBox *tensionGroup = new QGroupBox("Tension");
    QFormLayout *tensionLayout = new QFormLayout(tensionGroup);
    
    QSlider *tensionSlider = new QSlider(Qt::Horizontal);
    tensionSlider->setRange(0, 100);
    tensionSlider->setValue(50);
    
    QLabel *tensionValueLabel = new QLabel("0.5");
    tensionValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    tensionValueLabel->setMinimumWidth(30);
    
    tensionLayout->addRow("Tension:", createSliderLayout(tensionSlider, tensionValueLabel));
    layout->addWidget(tensionGroup);
    
    layout->addStretch();
    
    stackedControlLayout->addWidget(panel);
}

void MainWindow::setupBezierControls()
{
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    QLabel *infoLabel = new QLabel("Drag control points to adjust the Bezier curve.");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    // 阶数提升按钮
    QPushButton *elevateButton = new QPushButton("Elevate Degree");
    // 暂时移除未实现的函数
    // connect(elevateButton, &QPushButton::clicked, bezierCanvas, &BezierCurveCanvas::elevateDegree);
    layout->addWidget(elevateButton);
    
    // 分割曲线按钮
    QPushButton *splitButton = new QPushButton("Split Curve");
    // 暂时移除未实现的函数
    // connect(splitButton, &QPushButton::clicked, bezierCanvas, &BezierCurveCanvas::splitCurve);
    layout->addWidget(splitButton);
    
    layout->addStretch();
    
    stackedControlLayout->addWidget(panel);
}

void MainWindow::setupBSplineControls()
{
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    QLabel *infoLabel = new QLabel("B-Spline Controls");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    // 阶数控制
    QGroupBox *degreeGroup = new QGroupBox("Degree");
    QVBoxLayout *degreeLayout = new QVBoxLayout(degreeGroup);
    
    QSlider *degreeSlider = new QSlider(Qt::Horizontal);
    degreeSlider->setRange(1, 5);
    degreeSlider->setValue(3);
    connect(degreeSlider, &QSlider::valueChanged, bSplineCanvas, &BSplineCanvas::setDegree);
    
    QLabel *degreeLabel = new QLabel("3");
    connect(degreeSlider, &QSlider::valueChanged, [degreeLabel](int value) {
        degreeLabel->setText(QString::number(value));
    });
    
    degreeLayout->addLayout(createSliderLayout(degreeSlider, degreeLabel));
    layout->addWidget(degreeGroup);
    
    // 节点向量类型
    QGroupBox *knotGroup = new QGroupBox("Knot Vector");
    QVBoxLayout *knotLayout = new QVBoxLayout(knotGroup);
    
    QButtonGroup *knotButtonGroup = new QButtonGroup(this);
    QRadioButton *uniformKnots = new QRadioButton("Uniform");
    QRadioButton *openUniformKnots = new QRadioButton("Open Uniform");
    QRadioButton *customKnots = new QRadioButton("Custom");
    
    knotButtonGroup->addButton(uniformKnots, 0);
    knotButtonGroup->addButton(openUniformKnots, 1);
    knotButtonGroup->addButton(customKnots, 2);
    
    uniformKnots->setChecked(true);
    
    knotLayout->addWidget(uniformKnots);
    knotLayout->addWidget(openUniformKnots);
    knotLayout->addWidget(customKnots);
    
    layout->addWidget(knotGroup);
    
    layout->addStretch();
    
    stackedControlLayout->addWidget(panel);
}

void MainWindow::updateCanvasView(int index)
{
    // 更新堆叠布局的当前索引
    stackedControlLayout->setCurrentIndex(index);
    
    // 设置曲线颜色
    switch(index) {
        case 0: 
            parametricCanvas->setCurveColor(curveColors["Parametric"]);
            break;
        case 1: 
            splineCanvas->setCurveColor(curveColors["Spline"]);
            break;
        case 2: 
            bezierCanvas->setCurveColor(curveColors["Bezier"]);
            break;
        case 3: 
            bSplineCanvas->setCurveColor(curveColors["B-Spline"]);
            break;
        case 4: // Chaikin面板
            polygonCanvas->setCurveColor(curveColors["Chaikin"]);
        break;
    }
    
    // 连接当前画布的信号
    BaseCanvasWidget *currentCanvas = qobject_cast<BaseCanvasWidget*>(tabWidget->widget(index));
    if (currentCanvas) {
        connect(currentCanvas, &BaseCanvasWidget::pointHovered, 
                this, &MainWindow::updatePointInfo, Qt::UniqueConnection);
        connect(currentCanvas, &BaseCanvasWidget::noPointHovered, 
                this, &MainWindow::clearPointInfo, Qt::UniqueConnection);
        connect(currentCanvas, &BaseCanvasWidget::pointDeleted, 
                this, &MainWindow::showDeleteMessage, Qt::UniqueConnection);
    }
    
    // 更新通用控制状态
    showCurveCheck->setChecked(true);
    showControlPointsCheck->setChecked(true);
    showControlPolygonCheck->setChecked(true);
}

QHBoxLayout* MainWindow::createSliderLayout(QSlider* slider, QLabel* valueLabel)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(slider, 4);
    layout->addWidget(valueLabel, 1);
    return layout;
}

void MainWindow::updatePointInfo(const QPointF& point)
{
    // 转换为数学坐标
    BaseCanvasWidget *currentCanvas = qobject_cast<BaseCanvasWidget*>(tabWidget->currentWidget());
    double mathY = currentCanvas ? currentCanvas->height() - point.y() : 0;
    
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

void MainWindow::toggleCurveVisibility(bool visible)
{
    switch(tabWidget->currentIndex()) {
        case 0: parametricCanvas->toggleCurveVisibility(visible); break;
        case 1: splineCanvas->toggleCurveVisibility(visible); break;
        case 2: bezierCanvas->toggleCurveVisibility(visible); break;
        case 3: bSplineCanvas->toggleCurveVisibility(visible); break;
    }
}

void MainWindow::toggleControlPointsVisibility(bool visible)
{
    switch(tabWidget->currentIndex()) {
        case 0: 
            // 参数曲线没有额外的控制点
            break;
        case 1: 
            splineCanvas->toggleControlPointsVisibility(visible); 
            break;
        case 2: 
            // 贝塞尔曲线的控制点就是普通点
            break;
        case 3: 
            // B样条的控制点就是普通点
            break;
    }
}

void MainWindow::toggleControlPolygonVisibility(bool visible)
{
    switch(tabWidget->currentIndex()) {
        case 0: 
            // 参数曲线没有控制多边形
            break;
        case 1: 
            // 三次样条没有控制多边形
            break;
        case 2: 
            bezierCanvas->toggleControlPolygon(visible); 
            break;
        case 3: 
            bSplineCanvas->toggleControlPolygon(visible); 
            break;
    }
}

void MainWindow::updateDegreeValue(int value)
{
    if (degreeValueLabel) {
        degreeValueLabel->setText(QString::number(value));
    }
}

void MainWindow::updateSigmaValue(int value)
{
    if (sigmaValueLabel) {
        sigmaValueLabel->setText(QString::number(value) + ".0");
    }
}

void MainWindow::updateLambdaValue(int value)
{
    if (lambdaValueLabel) {
        lambdaValueLabel->setText("0." + QString("%1").arg(value, 2, 10, QChar('0')));
    }
}

void MainWindow::parameterizationMethodChanged(int id)
{
    parametricCanvas->setParameterizationMethod(static_cast<ParametricCurveCanvas::ParameterizationMethod>(id));
}

void MainWindow::setBSplineDegree(int degree)
{
    bSplineCanvas->setDegree(degree);
}