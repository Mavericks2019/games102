#include "mainwindow.h"
#include "canvaswidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QFrame>
#include <QFormLayout>
#include <QTimer>

QHBoxLayout* MainWindow::createSliderLayout(QSlider* slider, QLabel* valueLabel) {
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(slider, 4);
    layout->addWidget(valueLabel, 1);
    return layout;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Interpolation and Fitting Tool");
    resize(1200, 800);
    
    // 初始化曲线颜色
    curveColors["Polynomial Interpolation"] = Qt::blue;
    curveColors["Gaussian Interpolation"] = Qt::darkGreen;
    curveColors["Least Squares"] = Qt::magenta;
    curveColors["Ridge Regression"] = Qt::darkCyan;
    
    // 创建主部件和布局
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // 创建画布
    canvas = new CanvasWidget(this);
    mainLayout->addWidget(canvas, 1);
    
    // 创建控制面板
    QGroupBox *controlGroup = new QGroupBox("Control Panel");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    controlLayout->setAlignment(Qt::AlignTop);
    
    // 添加点信息标签 - 简化版本
    QGroupBox *pointGroup = new QGroupBox("Point Information");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    pointInfoLabel = new QLabel("Hover over a point to see coordinates");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(40); // 固定高度
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
    pointInfoLabel->setWordWrap(true);
    
    pointLayout->addWidget(pointInfoLabel);
    
    controlLayout->addWidget(pointGroup);
    
    // 添加图例标签
    QGroupBox *legendGroup = new QGroupBox("Legend");
    QVBoxLayout *legendLayout = new QVBoxLayout(legendGroup);
    
    legendLabel = new QLabel();
    legendLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    legendLabel->setMinimumSize(200, 150);
    legendLayout->addWidget(legendLabel);
    
    controlLayout->addWidget(legendGroup);
    
    // 添加控制按钮
    QGroupBox *methodGroup = new QGroupBox("Methods");
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    
    polyInterpCheck = new QCheckBox("Polynomial Interpolation");
    connect(polyInterpCheck, &QCheckBox::toggled, canvas, &CanvasWidget::togglePolyInterpolation);
    connect(polyInterpCheck, &QCheckBox::toggled, this, &MainWindow::updateLegend);
    methodLayout->addWidget(polyInterpCheck);
    
    gaussInterpCheck = new QCheckBox("Gaussian Interpolation");
    connect(gaussInterpCheck, &QCheckBox::toggled, canvas, &CanvasWidget::toggleGaussianInterpolation);
    connect(gaussInterpCheck, &QCheckBox::toggled, this, &MainWindow::updateLegend);
    methodLayout->addWidget(gaussInterpCheck);
    
    leastSquaresCheck = new QCheckBox("Least Squares Fitting");
    connect(leastSquaresCheck, &QCheckBox::toggled, canvas, &CanvasWidget::toggleLeastSquares);
    connect(leastSquaresCheck, &QCheckBox::toggled, this, &MainWindow::updateLegend);
    methodLayout->addWidget(leastSquaresCheck);
    
    ridgeRegCheck = new QCheckBox("Ridge Regression");
    connect(ridgeRegCheck, &QCheckBox::toggled, canvas, &CanvasWidget::toggleRidgeRegression);
    connect(ridgeRegCheck, &QCheckBox::toggled, this, &MainWindow::updateLegend);
    methodLayout->addWidget(ridgeRegCheck);
    
    controlLayout->addWidget(methodGroup);
    
    // 添加参数控制
    QGroupBox *paramGroup = new QGroupBox("Parameters");
    QFormLayout *paramLayout = new QFormLayout(paramGroup);
    
    // 多项式次数
    degreeSlider = new QSlider(Qt::Horizontal);
    degreeSlider->setRange(1, 10);
    degreeSlider->setValue(3);
    connect(degreeSlider, &QSlider::valueChanged, canvas, &CanvasWidget::setPolyDegree);
    connect(degreeSlider, &QSlider::valueChanged, this, &MainWindow::updateDegreeValue);
    
    degreeValueLabel = new QLabel("3");
    degreeValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    degreeValueLabel->setMinimumWidth(30);
    
    paramLayout->addRow("Poly Degree:", createSliderLayout(degreeSlider, degreeValueLabel));
    
    // 高斯Sigma
    sigmaSlider = new QSlider(Qt::Horizontal);
    sigmaSlider->setRange(1, 100);
    sigmaSlider->setValue(10);
    connect(sigmaSlider, &QSlider::valueChanged, canvas, &CanvasWidget::setGaussianSigma);
    connect(sigmaSlider, &QSlider::valueChanged, this, &MainWindow::updateSigmaValue);
    
    sigmaValueLabel = new QLabel("10.0");
    sigmaValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sigmaValueLabel->setMinimumWidth(30);
    
    paramLayout->addRow("Gaussian Sigma:", createSliderLayout(sigmaSlider, sigmaValueLabel));
    
    // 岭回归Lambda
    lambdaSlider = new QSlider(Qt::Horizontal);
    lambdaSlider->setRange(1, 100);
    lambdaSlider->setValue(10);
    connect(lambdaSlider, &QSlider::valueChanged, canvas, &CanvasWidget::setRidgeLambda);
    connect(lambdaSlider, &QSlider::valueChanged, this, &MainWindow::updateLambdaValue);
    
    lambdaValueLabel = new QLabel("0.10");
    lambdaValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lambdaValueLabel->setMinimumWidth(30);
    
    paramLayout->addRow("Ridge Lambda:", createSliderLayout(lambdaSlider, lambdaValueLabel));
    
    controlLayout->addWidget(paramGroup);
    
    // 添加清空按钮
    QPushButton *clearButton = new QPushButton("Clear Points");
    clearButton->setStyleSheet("background-color: #505050; color: white;");
    connect(clearButton, &QPushButton::clicked, canvas, &CanvasWidget::clearPoints);
    controlLayout->addWidget(clearButton);
    
    controlLayout->addStretch();
    
    mainLayout->addWidget(controlGroup);
    
    setCentralWidget(centralWidget);
    
    // 连接画布信号
    connect(canvas, &CanvasWidget::pointHovered, this, &MainWindow::updatePointInfo);
    connect(canvas, &CanvasWidget::noPointHovered, this, &MainWindow::clearPointInfo);
    connect(canvas, &CanvasWidget::pointDeleted, this, &MainWindow::showDeleteMessage);
    
    // 初始更新图例和参数值
    updateLegend();
    updateDegreeValue(degreeSlider->value());
    updateSigmaValue(sigmaSlider->value());
    updateLambdaValue(lambdaSlider->value());
    
    // 创建删除消息定时器
    deleteMessageTimer = new QTimer(this);
    deleteMessageTimer->setSingleShot(true);
    connect(deleteMessageTimer, &QTimer::timeout, this, [this]() {
        clearPointInfo();
    });
}

void MainWindow::updateLegend()
{
    QString html = "<html><body style='font-family:Arial; font-size:10pt; color:white;'>";
    html += "<h3 style='color:white;'>Active Curves</h3>";
    
    if (polyInterpCheck->isChecked()) {
        QString color = curveColors["Polynomial Interpolation"].name();
        html += QString("<div><span style='color:%1; font-weight:bold;'>■ </span>Polynomial Interpolation</div>").arg(color);
    }
    
    if (gaussInterpCheck->isChecked()) {
        QString color = curveColors["Gaussian Interpolation"].name();
        html += QString("<div><span style='color:%1; font-weight:bold;'>■ </span>Gaussian Interpolation</div>").arg(color);
    }
    
    if (leastSquaresCheck->isChecked()) {
        QString color = curveColors["Least Squares"].name();
        html += QString("<div><span style='color:%1; font-weight:bold;'>■ </span>Least Squares</div>").arg(color);
    }
    
    if (ridgeRegCheck->isChecked()) {
        QString color = curveColors["Ridge Regression"].name();
        html += QString("<div><span style='color:%1; font-weight:bold;'>■ </span>Ridge Regression</div>").arg(color);
    }
    
    if (!polyInterpCheck->isChecked() && 
        !gaussInterpCheck->isChecked() && 
        !leastSquaresCheck->isChecked() && 
        !ridgeRegCheck->isChecked()) {
        html += "<div><i>No active curves</i></div>";
    }
    
    html += "</body></html>";
    legendLabel->setText(html);
}

void MainWindow::updatePointInfo(const QPointF& point)
{
    // 将点坐标转换为数学坐标系
    double mathX = point.x();
    double mathY = canvas->height() - point.y();
    
    // 使用紧凑的单行格式
    QString info = QString("Screen: (%1, %2) \n Math: (%3, %4)")
                  .arg(point.x(), 0, 'f', 1)
                  .arg(point.y(), 0, 'f', 1)
                  .arg(mathX, 0, 'f', 1)
                  .arg(mathY, 0, 'f', 1);
    
    pointInfoLabel->setText(info);
    pointInfoLabel->setStyleSheet("background-color: #2A4A6A; color: white; border-radius: 5px; padding: 5px;");
}

void MainWindow::clearPointInfo()
{
    pointInfoLabel->setText("Hover over a point to see coordinates");
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
}

void MainWindow::updateDegreeValue(int value)
{
    degreeValueLabel->setText(QString::number(value));
}

void MainWindow::updateSigmaValue(int value)
{
    sigmaValueLabel->setText(QString::number(value) + ".0");
}

void MainWindow::updateLambdaValue(int value)
{
    lambdaValueLabel->setText("0." + QString("%1").arg(value, 2, 10, QChar('0')));
}

void MainWindow::showDeleteMessage()
{
    pointInfoLabel->setText("Point deleted");
    pointInfoLabel->setStyleSheet("background-color: #6A2A2A; color: white; border-radius: 5px; padding: 5px;");
    
    // 2秒后恢复原始状态
    deleteMessageTimer->start(2000);
}