// [文件]: curve_designer.cpp
#include "curve_designer.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QTimer>
#include <QButtonGroup>

CurveDesigner::CurveDesigner(QWidget *parent) : QWidget(parent) {
    // 初始化曲线颜色
    curveColors["Cubic Spline"] = QColor(255, 255, 100);      // 黄色
    curveColors["Original Spline"] = QColor(255, 100, 100);   // 红色
    curveColors["Bezier Curve"] = QColor(100, 255, 100);      // 绿色
    curveColors["Quadratic Spline"] = QColor(100, 100, 255);  // 蓝色
    setupUI();
}

void CurveDesigner::setupUI() {
    // 设置窗口属性
    setWindowTitle("Curve Design Tool");
    setMinimumSize(1200, 800);
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // ========== 左侧面板 - 曲线类型 ==========
    curveTypeGroup = new QGroupBox("Curve Type");
    curveTypeGroup->setFixedWidth(250);
    curveTypeGroup->setStyleSheet(
        "QGroupBox {"
        "  border: 1px solid #3A3939;"
        "  border-radius: 5px;"
        "  margin-top: 1ex;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top center;"
        "  padding: 0 5px;"
        "  background-color: #3A3939;"
        "  color: white;"
        "  font-weight: bold;"
        "}"
    );
    
    QVBoxLayout *curveTypeLayout = new QVBoxLayout(curveTypeGroup);
    
    originalSplineRadio = new QRadioButton("Original Spline");
    bezierCurveRadio = new QRadioButton("Bezier Curve");
    quadraticSplineRadio = new QRadioButton("Quadratic Spline");
    cubicSplineRadio = new QRadioButton("Cubic Spline");
    
    // 设置单选按钮样式 - 匹配提供的UI
    QString radioStyle = 
        "QRadioButton {"
        "   color: white;"
        "   padding: 10px 0;"
        "   font-size: 13px;"
        "}"
        "QRadioButton::indicator {"
        "   width: 18px;"
        "   height: 18px;"
        "}";
    
    originalSplineRadio->setStyleSheet(radioStyle);
    bezierCurveRadio->setStyleSheet(radioStyle);
    quadraticSplineRadio->setStyleSheet(radioStyle);
    cubicSplineRadio->setStyleSheet(radioStyle);
    
    // 默认选择原始样条
    cubicSplineRadio->setChecked(true);
    
    curveTypeLayout->addWidget(cubicSplineRadio);
    curveTypeLayout->addWidget(originalSplineRadio);
    curveTypeLayout->addWidget(bezierCurveRadio);
    curveTypeLayout->addWidget(quadraticSplineRadio);
    curveTypeLayout->addStretch();
    
    mainLayout->addWidget(curveTypeGroup);
    
    // ========== 中间画布 ==========
    canvas = new Canvas();
    mainLayout->addWidget(canvas, 1);
    
    // ========== 右侧面板 ==========
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);
    
    // 点信息显示 - 匹配提供的UI
    pointInfoGroup = new QGroupBox("Point Information");
    pointInfoGroup->setStyleSheet(curveTypeGroup->styleSheet());
    QVBoxLayout *pointInfoLayout = new QVBoxLayout(pointInfoGroup);
    
    pointInfoLabel = new QLabel("Hover over a point to see coordinates");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(60);
    pointInfoLabel->setStyleSheet(
        "QLabel {"
        "   background-color:rgb(240, 233, 233);"
        "   color: white;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "   font-size: 13px;"
        "}"
    );
    pointInfoLabel->setWordWrap(true);
    
    pointInfoLayout->addWidget(pointInfoLabel);
    rightLayout->addWidget(pointInfoGroup);
    
    // 图例 - 匹配提供的UI
    legendGroup = new QGroupBox("Legend");
    legendGroup->setStyleSheet(curveTypeGroup->styleSheet());
    QVBoxLayout *legendLayout = new QVBoxLayout(legendGroup);
    
    legendLabel = new QLabel();
    legendLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    legendLabel->setMinimumSize(200, 150);
    legendLabel->setStyleSheet(
        "QLabel {"
        "   color: white;"
        "   font-size: 13px;"
        "}"
    );
    legendLayout->addWidget(legendLabel);
    
    rightLayout->addWidget(legendGroup);
    
    // 控制选项 - 匹配提供的UI
    controlGroup = new QGroupBox("Controls");
    controlGroup->setStyleSheet(curveTypeGroup->styleSheet());
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    showCurveCheck = new QCheckBox("Show Curve");
    showCurveCheck->setStyleSheet(
        "QCheckBox {"
        "   color: white;"
        "   padding: 8px 0;"
        "   font-size: 13px;"
        "}"
        "QCheckBox::indicator {"
        "   width: 18px;"
        "   height: 18px;"
        "}"
    );
    showCurveCheck->setChecked(true);
    controlLayout->addWidget(showCurveCheck);
    
    clearButton = new QPushButton("Clear All Points");
    clearButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   font-size: 13px;"
        "   padding: 10px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color:rgb(82, 79, 79);"
        "}"
    );
    controlLayout->addWidget(clearButton);
    
    controlLayout->addStretch();
    rightLayout->addWidget(controlGroup);
    rightLayout->addStretch();
    
    mainLayout->addLayout(rightLayout);
    
    // 连接信号和槽
    connect(showCurveCheck, &QCheckBox::toggled, canvas, &Canvas::setShowCurve);
    connect(clearButton, &QPushButton::clicked, canvas, &Canvas::clearPoints);
    
    // 曲线类型选择信号
    connect(originalSplineRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            canvas->setCurveType(Canvas::OriginalSpline);
            updateLegend();
        }
    });
    
    connect(bezierCurveRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            canvas->setCurveType(Canvas::BezierCurve);
            updateLegend();
        }
    });
    
    connect(quadraticSplineRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            canvas->setCurveType(Canvas::QuadraticSpline);
            updateLegend();
        }
    });
    
    connect(cubicSplineRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            canvas->setCurveType(Canvas::CubicSpline);
            updateLegend();
        }
    });
    
    // 画布悬停信号
    connect(canvas, &Canvas::pointHovered, this, &CurveDesigner::updatePointInfo);
    connect(canvas, &Canvas::noPointHovered, this, &CurveDesigner::clearPointInfo);
    
    // 初始更新图例
    updateLegend();
}

void CurveDesigner::updateLegend() {
    QString html = "<html><body style='font-family:Arial; font-size:12pt; color:white;'>";
    html += "<h3 style='color:white; margin-top:0; font-size:14px;'>Active Curve</h3>";
    
    QString curveName;
    switch (canvas->getCurveType()) {
        case Canvas::OriginalSpline: curveName = "Original Spline"; break;
        case Canvas::BezierCurve: curveName = "Bezier Curve"; break;
        case Canvas::QuadraticSpline: curveName = "Quadratic Spline"; break;
        case Canvas::CubicSpline: curveName = "Cubic Spline"; break;
    }
    
    QString color = curveColors[curveName].name();
    html += QString("<div style='margin-bottom:10px;'><span style='color:%1; font-weight:bold; font-size:16px;'>■ </span>%2</div>").arg(color).arg(curveName);
    
    html += "<div style='margin-top:15px;'>";
    html += "<div style='font-weight:bold; margin-bottom:5px; font-size:14px;'>Controls:</div>";
    html += "<div style='font-size:12px;'>• Left click: Add point</div>";
    html += "<div style='font-size:12px;'>• Drag point: Move control point</div>";
    html += "<div style='font-size:12px;'>• Right click: Delete point</div>";
    html += "<div style='font-size:12px;'>• Drag tangent: Adjust curve</div>";
    html += "</div>";
    
    html += "</body></html>";
    legendLabel->setText(html);
}

void CurveDesigner::updatePointInfo(const QPointF& point) {
    QString info = QString("Screen Coordinates:\n(%1, %2)\n\n")
                  .arg(point.x(), 0, 'f', 1)
                  .arg(point.y(), 0, 'f', 1);
    
    // 转换为数学坐标（Y轴翻转）
    QPointF mathPoint(point.x(), canvas->height() - point.y());
    info += QString("Math Coordinates:\n(%1, %2)")
           .arg(mathPoint.x(), 0, 'f', 1)
           .arg(mathPoint.y(), 0, 'f', 1);
    
    pointInfoLabel->setText(info);
    pointInfoLabel->setStyleSheet(
        "QLabel {"
        "   background-color: #2A4A6A;"
        "   color: white;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "   font-size: 13px;"
        "}"
    );
}

void CurveDesigner::clearPointInfo() {
    pointInfoLabel->setText("Hover over a point to see coordinates");
    pointInfoLabel->setStyleSheet(
        "QLabel {"
        "   background-color: #3A3A3A;"
        "   color: white;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "   font-size: 13px;"
        "}"
    );
}