#ifndef CURVE_DESIGNER_H
#define CURVE_DESIGNER_H

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "canvas.h"

class CurveDesigner : public QWidget {
    Q_OBJECT
public:
    CurveDesigner(QWidget *parent = nullptr);

private:
    void updateStatus();

    Canvas *canvas;
    QCheckBox *showCurveCheck;
    QPushButton *clearButton;
    QLabel *statusLabel;
};

#endif // CURVE_DESIGNER_H