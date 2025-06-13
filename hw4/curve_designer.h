// [文件]: curve_designer.h
#ifndef CURVE_DESIGNER_H
#define CURVE_DESIGNER_H

#include <QWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include "canvas.h"

class CurveDesigner : public QWidget {
    Q_OBJECT
public:
    CurveDesigner(QWidget *parent = nullptr);

private:
    void setupUI();
    void updateLegend();
    void updatePointInfo(const QPointF& point);
    void clearPointInfo();
    
    Canvas *canvas;
    
    // 左侧面板
    QGroupBox *curveTypeGroup;
    QRadioButton *originalSplineRadio;
    QRadioButton *bezierCurveRadio;
    QRadioButton *quadraticSplineRadio;
    QRadioButton *cubicSplineRadio;
    
    // 右侧面板
    QGroupBox *pointInfoGroup;
    QLabel *pointInfoLabel;
    
    QGroupBox *legendGroup;
    QLabel *legendLabel;
    
    QGroupBox *controlGroup;
    QCheckBox *showCurveCheck;
    QPushButton *clearButton;
    
    QMap<QString, QColor> curveColors;
};

#endif // CURVE_DESIGNER_H