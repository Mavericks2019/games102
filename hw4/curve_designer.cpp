#include "curve_designer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QColorDialog>

CurveDesigner::CurveDesigner(QWidget *parent) : QWidget(parent) {
    // 设置窗口属性
    setWindowTitle("Curve Designer");
    setMinimumSize(1000, 700);
    
    // 创建画布
    canvas = new Canvas();
    
    // 创建右侧控制面板
    QWidget *controlPanel = new QWidget();
    controlPanel->setFixedWidth(250);
    controlPanel->setStyleSheet(
        "background-color: #2c313c;"
        "padding: 15px;"
        "border-left: 1px solid #4a4f5a;"
    );
    
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlLayout->setSpacing(15);
    controlLayout->setContentsMargins(10, 10, 10, 10);
    
    // 标题
    QLabel *title = new QLabel("CURVE CONTROLS");
    title->setStyleSheet(
        "font-weight: bold;"
        "font-size: 18px;"
        "color: #ecf0f1;"
        "margin-bottom: 20px;"
        "padding: 5px;"
        "border-bottom: 2px solid #4a4f5a;"
    );
    controlLayout->addWidget(title);
    
    // 显示曲线选项
    showCurveCheck = new QCheckBox("Show Curve");
    showCurveCheck->setStyleSheet(
        "QCheckBox {"
        "   color: #ecf0f1;"
        "   font-size: 14px;"
        "   padding: 8px;"
        "}"
        "QCheckBox::indicator {"
        "   width: 20px;"
        "   height: 20px;"
        "}"
    );
    showCurveCheck->setChecked(true);
    controlLayout->addWidget(showCurveCheck);
    
    // 曲线颜色选择
    QPushButton *colorButton = new QPushButton("Select Curve Color");
    colorButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #3498db;"
        "   color: white;"
        "   font-size: 14px;"
        "   padding: 12px;"
        "   border-radius: 5px;"
        "   min-height: 40px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #2980b9;"
        "}"
    );
    controlLayout->addWidget(colorButton);
    
    // 清空按钮
    clearButton = new QPushButton("Clear All Points");
    clearButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #e74c3c;"
        "   color: white;"
        "   font-size: 14px;"
        "   padding: 12px;"
        "   border-radius: 5px;"
        "   min-height: 40px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #c0392b;"
        "}"
    );
    controlLayout->addWidget(clearButton);
    
    // 分隔线
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #4a4f5a; margin: 20px 0;");
    controlLayout->addWidget(line);
    
    // 操作说明
    QLabel *helpLabel = new QLabel("INSTRUCTIONS:");
    helpLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #ecf0f1;"
        "font-size: 16px;"
        "margin-bottom: 10px;"
    );
    controlLayout->addWidget(helpLabel);
    
    // 使用更大的字体和间距
    QLabel *help1 = new QLabel("• Left click: Add point");
    QLabel *help2 = new QLabel("• Drag point: Move control point");
    QLabel *help3 = new QLabel("• Click point: Show tangents");
    QLabel *help4 = new QLabel("• Drag tangent: Adjust curve");
    QLabel *help5 = new QLabel("• Right click: Delete point");
    
    QString labelStyle = 
        "color: #bdc3c7;"
        "font-size: 13px;"
        "margin-bottom: 8px;"
        "min-height: 25px;";
    
    help1->setStyleSheet(labelStyle);
    help2->setStyleSheet(labelStyle);
    help3->setStyleSheet(labelStyle);
    help4->setStyleSheet(labelStyle);
    help5->setStyleSheet(labelStyle);
    
    controlLayout->addWidget(help1);
    controlLayout->addWidget(help2);
    controlLayout->addWidget(help3);
    controlLayout->addWidget(help4);
    controlLayout->addWidget(help5);
    
    // 添加状态信息标签
    statusLabel = new QLabel("Points: 0 | Curve: Visible");
    statusLabel->setStyleSheet(
        "color: #95a5a6;"
        "font-size: 12px;"
        "margin-top: 20px;"
        "padding-top: 10px;"
        "border-top: 1px solid #4a4f5a;"
    );
    controlLayout->addWidget(statusLabel);
    
    controlLayout->addStretch();
    
    // 主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(canvas, 1);
    mainLayout->addWidget(controlPanel);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 连接信号和槽
    connect(showCurveCheck, &QCheckBox::toggled, this, [this](bool checked) {
        canvas->setShowCurve(checked);
        updateStatus();
    });
    
    connect(colorButton, &QPushButton::clicked, this, [this]() {
        // 直接访问公有成员变量 curveColor
        QColor color = QColorDialog::getColor(canvas->curveColor, this, "Select Curve Color");
        if (color.isValid()) {
            canvas->curveColor = color; // 直接赋值
            canvas->update();
        }
    });
    
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        canvas->clearPoints();
        updateStatus();
    });
    
    // 初始状态更新
    updateStatus();
}

void CurveDesigner::updateStatus() {
    int pointCount = canvas->getPointCount();
    QString curveState = showCurveCheck->isChecked() ? "Visible" : "Hidden";
    statusLabel->setText(QString("Points: %1 | Curve: %2").arg(pointCount).arg(curveState));
}