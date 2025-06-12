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
#include <QRadioButton>
#include <QButtonGroup>

QHBoxLayout* MainWindow::createSliderLayout(QSlider* slider, QLabel* valueLabel) {
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(slider, 4);
    layout->addWidget(valueLabel, 1);
    return layout;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Interpolation and Fitting Visualization");
    resize(1400, 800);
    
    // Initialize curve colors
    curveColors["Polynomial Interpolation"] = Qt::blue;
    curveColors["Gaussian Interpolation"] = Qt::darkGreen;
    curveColors["Least Squares"] = Qt::magenta;
    curveColors["Ridge Regression"] = Qt::darkCyan;
    
    // Create main widget and layout
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // ========== LEFT CONTROL PANEL ==========
    QGroupBox *leftControlGroup = new QGroupBox("Parameterization Methods");
    leftControlGroup->setFixedWidth(250);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftControlGroup);
    
    // Parameterization methods
    paramGroup = new QButtonGroup(this);
    
    uniformParam = new QRadioButton("Uniform Parameterization");
    chordalParam = new QRadioButton("Chordal Parameterization");
    centripetalParam = new QRadioButton("Centripetal Parameterization");
    foleyParam = new QRadioButton("Foley-Nielsen Parameterization");
    
    paramGroup->addButton(uniformParam, 0);
    paramGroup->addButton(chordalParam, 1);
    paramGroup->addButton(centripetalParam, 2);
    paramGroup->addButton(foleyParam, 3);
    
    uniformParam->setChecked(true);
    
    leftLayout->addWidget(uniformParam);
    leftLayout->addWidget(chordalParam);
    leftLayout->addWidget(centripetalParam);
    leftLayout->addWidget(foleyParam);
    
    leftLayout->addStretch();
    
    // Connect parameterization signals
    connect(paramGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), 
            this, &MainWindow::parameterizationMethodChanged);
    
    mainLayout->addWidget(leftControlGroup);
    
    // ========== CANVAS ==========
    canvas = new CanvasWidget(this);
    mainLayout->addWidget(canvas, 1);
    
    // ========== RIGHT CONTROL PANEL ==========
    QGroupBox *rightControlGroup = new QGroupBox("Fitting Methods");
    rightControlGroup->setFixedWidth(300);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightControlGroup);
    rightLayout->setAlignment(Qt::AlignTop);
    
    // Point information
    QGroupBox *pointGroup = new QGroupBox("Point Information");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    pointInfoLabel = new QLabel("Hover over a point to see coordinates");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(50);
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px;");
    pointInfoLabel->setWordWrap(true);
    
    pointLayout->addWidget(pointInfoLabel);
    
    rightLayout->addWidget(pointGroup);
    
    // Legend
    QGroupBox *legendGroup = new QGroupBox("Legend");
    QVBoxLayout *legendLayout = new QVBoxLayout(legendGroup);
    
    legendLabel = new QLabel();
    legendLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    legendLabel->setMinimumSize(200, 150);
    legendLayout->addWidget(legendLabel);
    
    rightLayout->addWidget(legendGroup);
    
    // Fitting methods
    QGroupBox *methodGroup = new QGroupBox("Fitting Algorithms");
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
    
    rightLayout->addWidget(methodGroup);
    
    // Parameters
    QGroupBox *paramGroup = new QGroupBox("Parameters");
    QFormLayout *paramLayout = new QFormLayout(paramGroup);
    
    // Polynomial degree
    degreeSlider = new QSlider(Qt::Horizontal);
    degreeSlider->setRange(1, 20);
    degreeSlider->setValue(3);
    connect(degreeSlider, &QSlider::valueChanged, canvas, &CanvasWidget::setPolyDegree);
    connect(degreeSlider, &QSlider::valueChanged, this, &MainWindow::updateDegreeValue);
    
    degreeValueLabel = new QLabel("3");
    degreeValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    degreeValueLabel->setMinimumWidth(30);
    
    paramLayout->addRow("Poly Degree:", createSliderLayout(degreeSlider, degreeValueLabel));
    
    // Gaussian Sigma
    sigmaSlider = new QSlider(Qt::Horizontal);
    sigmaSlider->setRange(1, 100);
    sigmaSlider->setValue(10);
    connect(sigmaSlider, &QSlider::valueChanged, canvas, &CanvasWidget::setGaussianSigma);
    connect(sigmaSlider, &QSlider::valueChanged, this, &MainWindow::updateSigmaValue);
    
    sigmaValueLabel = new QLabel("10.0");
    sigmaValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sigmaValueLabel->setMinimumWidth(30);
    
    paramLayout->addRow("Gaussian Sigma:", createSliderLayout(sigmaSlider, sigmaValueLabel));
    
    // Ridge Lambda
    lambdaSlider = new QSlider(Qt::Horizontal);
    lambdaSlider->setRange(1, 100);
    lambdaSlider->setValue(10);
    connect(lambdaSlider, &QSlider::valueChanged, canvas, &CanvasWidget::setRidgeLambda);
    connect(lambdaSlider, &QSlider::valueChanged, this, &MainWindow::updateLambdaValue);
    
    lambdaValueLabel = new QLabel("0.10");
    lambdaValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lambdaValueLabel->setMinimumWidth(30);
    
    paramLayout->addRow("Ridge Lambda:", createSliderLayout(lambdaSlider, lambdaValueLabel));
    
    rightLayout->addWidget(paramGroup);
    
    // Clear button
    QPushButton *clearButton = new QPushButton("Clear Points");
    clearButton->setStyleSheet("background-color: #505050; color: white;");
    connect(clearButton, &QPushButton::clicked, canvas, &CanvasWidget::clearPoints);
    rightLayout->addWidget(clearButton);
    
    rightLayout->addStretch();
    
    mainLayout->addWidget(rightControlGroup);
    
    setCentralWidget(centralWidget);
    
    // Connect canvas signals
    connect(canvas, &CanvasWidget::pointHovered, this, &MainWindow::updatePointInfo);
    connect(canvas, &CanvasWidget::noPointHovered, this, &MainWindow::clearPointInfo);
    connect(canvas, &CanvasWidget::pointDeleted, this, &MainWindow::showDeleteMessage);
    
    // Initial updates
    updateLegend();
    updateDegreeValue(degreeSlider->value());
    updateSigmaValue(sigmaSlider->value());
    updateLambdaValue(lambdaSlider->value());
    
    // Delete message timer
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
    // Convert to mathematical coordinates
    double mathX = point.x();
    double mathY = canvas->height() - point.y();
    
    // Compact single-line format
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
    
    // Restore after 2 seconds
    deleteMessageTimer->start(2000);
}

void MainWindow::parameterizationMethodChanged(int id)
{
    canvas->setParameterizationMethod(static_cast<CanvasWidget::ParameterizationMethod>(id));
}