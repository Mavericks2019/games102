#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QToolButton>
#include <QStyleFactory>
#include <<QMessageBox>>
#include <opencv2/opencv.hpp>

using namespace cv;

// 控制点结构体
struct ControlPoint {
    QPointF position;       // 点位置
    QPointF leftTangent;    // 左切线向量
    QPointF rightTangent;   // 右切线向量
    bool tangentLocked;     // 切线是否被锁定
    
    ControlPoint(QPointF pos) : position(pos), leftTangent(-30, 0), rightTangent(30, 0), tangentLocked(false) {}
};

class CurveEditor : public QWidget {
    Q_OBJECT
    
public:
    explicit CurveEditor(QWidget *parent = nullptr) : QWidget(parent), 
        selectedPoint(-1), tangentSelected(NONE), showTangents(true), showCurve(true) {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        
        // 添加默认点
        controlPoints.push_back(ControlPoint(QPointF(100, 300)));
        controlPoints.push_back(ControlPoint(QPointF(200, 200)));
        controlPoints.push_back(ControlPoint(QPointF(300, 250)));
        controlPoints.push_back(ControlPoint(QPointF(400, 150)));
        controlPoints.push_back(ControlPoint(QPointF(500, 300)));
        
        updateCurve();
    }

    void setShowCurve(bool show) {
        showCurve = show;
        update();
    }
    
    void setShowTangents(bool show) {
        showTangents = show;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        
        // 绘制背景
        painter.fillRect(rect(), QColor(45, 45, 55));
        
        // 绘制网格
        drawGrid(painter);
        
        // 绘制曲线
        if (showCurve) {
            drawCurve(painter);
        }
        
        // 绘制控制点和切线
        drawControlPoints(painter);
        
        // 绘制标题
        drawTitle(painter);
    }
    
    void mousePressEvent(QMouseEvent *event) override {
        // 兼容所有Qt版本的获取鼠标位置方法
        QPointF pos = event->pos();
        
        if (event->button() == Qt::LeftButton) {
            // 检查是否点击了切线控制点
            tangentSelected = NONE;
            if (selectedPoint >= 0 && showTangents) {
                QPointF leftHandle = controlPoints[selectedPoint].position + controlPoints[selectedPoint].leftTangent;
                QPointF rightHandle = controlPoints[selectedPoint].position + controlPoints[selectedPoint].rightTangent;
                
                if (distance(pos, leftHandle) < 10) {
                    tangentSelected = LEFT;
                    return;
                }
                if (distance(pos, rightHandle) < 10) {
                    tangentSelected = RIGHT;
                    return;
                }
            }
            
            // 检查是否点击了控制点
            int prevSelected = selectedPoint;
            selectedPoint = -1;
            for (int i = 0; i < controlPoints.size(); ++i) {
                if (distance(pos, controlPoints[i].position) < 10) {
                    selectedPoint = i;
                    break;
                }
            }
            
            // 如果选中的点发生变化，更新显示
            if (prevSelected != selectedPoint) {
                update();
            }
            
            // 如果没点到控制点，添加新点
            if (selectedPoint == -1 && event->modifiers() == Qt::NoModifier) {
                // 找到最近的线段插入点
                int insertIndex = findClosestSegment(pos);
                if (insertIndex >= 0) {
                    controlPoints.insert(controlPoints.begin() + insertIndex + 1, ControlPoint(pos));
                    selectedPoint = insertIndex + 1;
                } else {
                    controlPoints.push_back(ControlPoint(pos));
                    selectedPoint = controlPoints.size() - 1;
                }
                updateCurve();
                update();
            }
        }
        else if (event->button() == Qt::RightButton) {
            // 右键删除点
            int pointToDelete = -1;
            for (int i = 0; i < controlPoints.size(); ++i) {
                if (distance(pos, controlPoints[i].position) < 10) {
                    pointToDelete = i;
                    break;
                }
            }
            
            if (pointToDelete != -1) {
                controlPoints.erase(controlPoints.begin() + pointToDelete);
                if (selectedPoint == pointToDelete) {
                    selectedPoint = -1;
                } else if (selectedPoint > pointToDelete) {
                    selectedPoint--;
                }
                updateCurve();
                update();
            }
        }
    }
    
    void mouseMoveEvent(QMouseEvent *event) override {
        // 兼容所有Qt版本的获取鼠标位置方法
        QPointF pos = event->pos();
        
        if (event->buttons() & Qt::LeftButton) {
            if (selectedPoint >= 0 && selectedPoint < controlPoints.size()) {
                if (tangentSelected == NONE) {
                    // 移动控制点
                    controlPoints[selectedPoint].position = pos;
                    updateCurve();
                } else if (tangentSelected == LEFT) {
                    // 调整左切线
                    QPointF tangent = pos - controlPoints[selectedPoint].position;
                    controlPoints[selectedPoint].leftTangent = tangent;
                    
                    // 如果切线锁定，调整右切线保持对称
                    if (controlPoints[selectedPoint].tangentLocked) {
                        controlPoints[selectedPoint].rightTangent = -tangent;
                    }
                    updateCurve();
                } else if (tangentSelected == RIGHT) {
                    // 调整右切线
                    QPointF tangent = pos - controlPoints[selectedPoint].position;
                    controlPoints[selectedPoint].rightTangent = tangent;
                    
                    // 如果切线锁定，调整左切线保持对称
                    if (controlPoints[selectedPoint].tangentLocked) {
                        controlPoints[selectedPoint].leftTangent = -tangent;
                    }
                    updateCurve();
                }
            }
        }
        
        // 更新状态栏
        QString status = QString("Position: (%1, %2)").arg(pos.x()).arg(pos.y());
        if (selectedPoint >= 0) {
            status += QString(" | Selected Point: %1").arg(selectedPoint);
            if (controlPoints[selectedPoint].tangentLocked) {
                status += " | Tangents Locked";
            }
        }
        emit statusChanged(status);
        
        update();
    }
    
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && selectedPoint >= 0) {
            // 切换切线锁定状态
            controlPoints[selectedPoint].tangentLocked = !controlPoints[selectedPoint].tangentLocked;
            update();
        }
    }
    
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Delete && selectedPoint >= 0) {
            // 删除选中的点
            controlPoints.erase(controlPoints.begin() + selectedPoint);
            selectedPoint = -1;
            updateCurve();
            update();
        } else if (event->key() == Qt::Key_T) {
            // 切换切线显示
            showTangents = !showTangents;
            update();
        } else if (event->key() == Qt::Key_C) {
            // 清空所有点
            controlPoints.clear();
            selectedPoint = -1;
            updateCurve();
            update();
        } else if (event->key() == Qt::Key_R) {
            // 重置曲线
            controlPoints.clear();
            controlPoints.push_back(ControlPoint(QPointF(100, 300)));
            controlPoints.push_back(ControlPoint(QPointF(200, 200)));
            controlPoints.push_back(ControlPoint(QPointF(300, 250)));
            controlPoints.push_back(ControlPoint(QPointF(400, 150)));
            controlPoints.push_back(ControlPoint(QPointF(500, 300)));
            selectedPoint = -1;
            updateCurve();
            update();
        }
    }

signals:
    void statusChanged(const QString &message);

private:
    enum TangentSelection { NONE, LEFT, RIGHT };
    
    std::vector<ControlPoint> controlPoints;
    std::vector<QPointF> curvePoints;
    int selectedPoint;
    TangentSelection tangentSelected;
    bool showTangents;
    bool showCurve;
    
    double distance(const QPointF &p1, const QPointF &p2) {
        return std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2));
    }
    
    int findClosestSegment(const QPointF &pos) {
        if (curvePoints.size() < 2) return -1;
        
        int closestIndex = -1;
        double minDistance = 20; // 最小距离阈值
        
        for (int i = 0; i < curvePoints.size() - 1; ++i) {
            QPointF p1 = curvePoints[i];
            QPointF p2 = curvePoints[i+1];
            
            // 计算点到线段的距离
            double A = pos.x() - p1.x();
            double B = pos.y() - p1.y();
            double C = p2.x() - p1.x();
            double D = p2.y() - p1.y();
            
            double dot = A * C + B * D;
            double len_sq = C * C + D * D;
            double param = (len_sq != 0) ? dot / len_sq : -1;
            
            double xx, yy;
            if (param < 0) {
                xx = p1.x();
                yy = p1.y();
            } else if (param > 1) {
                xx = p2.x();
                yy = p2.y();
            } else {
                xx = p1.x() + param * C;
                yy = p1.y() + param * D;
            }
            
            double dx = pos.x() - xx;
            double dy = pos.y() - yy;
            double dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist < minDistance) {
                minDistance = dist;
                closestIndex = i;
            }
        }
        
        // 找到最近的线段对应的控制点索引
        if (closestIndex != -1) {
            double curvePos = closestIndex / 20.0;
            return std::min(static_cast<int>(curvePos), static_cast<int>(controlPoints.size()) - 2);
        }
        
        return -1;
    }
    
    void drawGrid(QPainter &painter) {
        painter.setPen(QPen(QColor(80, 80, 100), 1));
        
        // 绘制网格
        for (int x = 0; x < width(); x += 20) {
            painter.drawLine(x, 0, x, height());
        }
        
        for (int y = 0; y < height(); y += 20) {
            painter.drawLine(0, y, width(), y);
        }
        
        // 绘制坐标轴
        painter.setPen(QPen(Qt::white, 2));
        painter.drawLine(0, height()/2, width(), height()/2);
        painter.drawLine(width()/2, 0, width()/2, height());
    }
    
    void drawCurve(QPainter &painter) {
        if (curvePoints.size() < 2) return;
        
        // 绘制曲线
        painter.setPen(QPen(QColor(0, 200, 255), 3));
        for (size_t i = 0; i < curvePoints.size() - 1; ++i) {
            painter.drawLine(curvePoints[i], curvePoints[i+1]);
        }
    }
    
    void drawControlPoints(QPainter &painter) {
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            const ControlPoint &cp = controlPoints[i];
            
            // 绘制切线 - 只有选中的点才显示切线
            if (showTangents && static_cast<int>(i) == selectedPoint) {
                painter.setPen(QPen(Qt::yellow, 1));
                
                // 左切线
                painter.drawLine(cp.position, cp.position + cp.leftTangent);
                
                // 右切线
                painter.drawLine(cp.position, cp.position + cp.rightTangent);
                
                // 切线控制点
                painter.setBrush(Qt::yellow);
                painter.drawEllipse(cp.position + cp.leftTangent, 5, 5);
                painter.drawEllipse(cp.position + cp.rightTangent, 5, 5);
            }
            
            // 绘制控制点
            painter.setPen(Qt::black);
            if (static_cast<int>(i) == selectedPoint) {
                painter.setBrush(cp.tangentLocked ? QColor(255, 100, 100) : Qt::red);
                painter.drawEllipse(cp.position, 8, 8);
            } else {
                painter.setBrush(QColor(100, 200, 100));
                painter.drawEllipse(cp.position, 6, 6);
            }
        }
    }
    
    void drawTitle(QPainter &painter) {
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(16);
        font.setBold(true);
        painter.setFont(font);
        
        painter.drawText(20, 40, "曲线设计与编辑工具");
        
        font.setPointSize(10);
        painter.setFont(font);
        painter.drawText(20, height() - 30, "提示: 左键添加/移动点 | 右键删除点 | 双击锁定切线 | T键切换切线显示");
        painter.drawText(20, height() - 10, "R键重置 | C键清空 | Delete键删除点");
    }
    
    void updateCurve() {
        curvePoints.clear();
        if (controlPoints.size() < 2) return;
        
        // 使用三次Hermite样条生成曲线（考虑切线）
        for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
            const QPointF &p0 = controlPoints[i].position;
            const QPointF &p1 = controlPoints[i+1].position;
            const QPointF &m0 = controlPoints[i].rightTangent; // 当前点的右切线
            const QPointF &m1 = controlPoints[i+1].leftTangent; // 下一点的左切线
            
            // 添加曲线段
            for (int j = 0; j <= 20; j++) {
                double t = j / 20.0;
                double t2 = t * t;
                double t3 = t2 * t;
                
                // Hermite基函数
                double h00 = 2*t3 - 3*t2 + 1;
                double h10 = t3 - 2*t2 + t;
                double h01 = -2*t3 + 3*t2;
                double h11 = t3 - t2;
                
                QPointF pt = h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
                curvePoints.push_back(pt);
            }
        }
    }
};

class MainWindow : public QMainWindow {
public:
    MainWindow() : QMainWindow() {
        setWindowTitle("曲线设计与编辑工具 - 基于Qt和OpenCV");
        setMinimumSize(800, 600);
        
        // 创建曲线编辑器
        editor = new CurveEditor(this);
        setCentralWidget(editor);
        
        // 创建工具栏（放在右侧）
        QToolBar *toolBar = new QToolBar("控制工具", this);
        toolBar->setIconSize(QSize(32, 32));
        toolBar->setStyleSheet(
            "QToolBar { background-color: #353535; border-left: 1px solid #555; }"
            "QToolButton { background-color: #454545; border-radius: 4px; padding: 5px; }"
            "QToolButton:hover { background-color: #555555; }"
            "QToolButton:pressed { background-color: #656565; }"
            "QToolButton:checked { background-color: #2a82da; }"
        );
        addToolBar(Qt::RightToolBarArea, toolBar);
        
        // 添加显示曲线按钮
        QAction *showCurveAction = new QAction("显示曲线", this);
        showCurveAction->setCheckable(true);
        showCurveAction->setChecked(true);
        showCurveAction->setIcon(createIcon(QColor(0, 200, 255)));
        toolBar->addAction(showCurveAction);
        
        // 添加显示切线按钮
        QAction *showTangentsAction = new QAction("显示切线", this);
        showTangentsAction->setCheckable(true);
        showTangentsAction->setChecked(true);
        showTangentsAction->setIcon(createIcon(Qt::yellow));
        toolBar->addAction(showTangentsAction);
        
        // 添加重置按钮
        QAction *resetAction = new QAction("重置", this);
        resetAction->setIcon(createIcon(Qt::green));
        toolBar->addAction(resetAction);
        
        // 添加分隔线
        toolBar->addSeparator();
        
        // 添加帮助按钮
        QAction *helpAction = new QAction("帮助", this);
        helpAction->setIcon(createIcon(QColor(200, 150, 255)));
        toolBar->addAction(helpAction);
        
        // 连接信号和槽
        connect(showCurveAction, &QAction::toggled, editor, &CurveEditor::setShowCurve);
        connect(showTangentsAction, &QAction::toggled, editor, &CurveEditor::setShowTangents);
        connect(resetAction, &QAction::triggered, [this]() {
            editor->keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier));
        });
        connect(helpAction, &QAction::triggered, []() {
            QMessageBox::information(nullptr, "帮助", 
                "曲线设计与编辑工具\n\n"
                "操作指南:\n"
                "1. 左键点击空白处添加控制点\n"
                "2. 拖动控制点调整位置\n"
                "3. 右键点击控制点删除\n"
                "4. 双击控制点锁定/解锁切线\n"
                "5. 拖动切线控制点调整曲线形状\n\n"
                "快捷键:\n"
                "T: 切换切线显示\n"
                "R: 重置曲线\n"
                "C: 清空所有点\n"
                "Delete: 删除选中点");
        });
        
        // 设置状态栏样式
        statusBar()->setStyleSheet("background-color: #333; color: #eee;");
        
        // 连接状态更新
        connect(editor, &CurveEditor::statusChanged, this, [this](const QString &msg) {
            statusBar()->showMessage(msg);
        });
    }

private:
    CurveEditor *editor;
    
    // 创建工具栏图标
    QIcon createIcon(const QColor &color) {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(4, 4, 24, 24);
        
        return QIcon(pixmap);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用程序样式
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // 创建深色主题
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35,35,35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42,130,218));
    darkPalette.setColor(QPalette::Highlight, QColor(42,130,218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    
    // 设置全局样式表
    app.setStyleSheet(
        "QMainWindow { background-color: #353535; }"
        "QStatusBar { background-color: #333; color: #eee; }"
    );
    
    MainWindow mainWin;
    mainWin.show();
    
    return app.exec();
}

#include "main.moc"

void Canvas::mousePressEvent(QMouseEvent *event) {
    QPointF pos = event->pos();
    
    // 左键点击
    if (event->button() == Qt::LeftButton) {
        // ... [其他代码保持不变] ...
        
        // 添加新点
        ControlPoint newPoint(pos, nextId++);
        
        // 自动设置合理的切线方向
        if (!controlPoints.empty()) {
            // 计算与前一个点的方向
            QPointF prevDir = controlPoints.back().pos - pos;
            qreal length = sqrt(prevDir.x() * prevDir.x() + prevDir.y() * prevDir.y());
            
            // 设置合理的切线长度（距离的30%）
            qreal tangentLength = length * 0.3;
            if (tangentLength < 5) tangentLength = 5; // 最小长度
            
            // 归一化并缩放
            if (length > 0.001) {
                prevDir = prevDir * (tangentLength / length);
            } else {
                prevDir = QPointF(-20, 0); // 默认值
            }
            
            // 设置新点的切线
            newPoint.leftTangent = prevDir;
            newPoint.rightTangent = -prevDir;
        }
        
        controlPoints.push_back(newPoint);
        draggingPoint = controlPoints.size() - 1;
        draggingLeftTangent = false;
        draggingRightTangent = false;
        update();
    } 
    // ... [其他代码保持不变] ...
}