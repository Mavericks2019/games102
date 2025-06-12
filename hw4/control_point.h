#ifndef CONTROL_POINT_H
#define CONTROL_POINT_H

#include <QPointF>

struct ControlPoint {
    QPointF pos;
    QPointF leftTangent;    // 左侧切线向量
    QPointF rightTangent;   // 右侧切线向量
    bool leftTangentFixed;  // 左侧切线是否被固定
    bool rightTangentFixed; // 右侧切线是否被固定
    bool selected;          // 是否被选中
    int id;                 // 唯一标识符
    
    ControlPoint(QPointF p = QPointF(), int idx = 0);
    
    // 添加相等运算符
    bool operator==(const ControlPoint& other) const;
};

#endif // CONTROL_POINT_H