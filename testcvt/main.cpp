#include <QApplication>
#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPushButton>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QLabel>
#include <random>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2<K> Delaunay;
typedef CGAL::Delaunay_triangulation_adaptation_traits_2<Delaunay> AT;
typedef CGAL::Delaunay_triangulation_caching_degeneracy_removal_policy_2<Delaunay> AP;
typedef CGAL::Voronoi_diagram_2<Delaunay, AT, AP> Voronoi;
typedef K::Point_2 Point;
typedef K::Segment_2 Segment;
typedef Voronoi::Edge Edge;
typedef Voronoi::Face_handle Face_handle;
typedef Voronoi::Ccb_halfedge_circulator Ccb_halfedge_circulator;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    GLWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent), iterationCount(0) {
        setMinimumSize(600, 600);
        resetPoints();
    }

    void resetPoints() {
        points.clear();
        iterationCount = 0;
        
        // 添加4个边界点
        points.push_back(Point(-1, -1));
        points.push_back(Point(-1, 1));
        points.push_back(Point(1, 1));
        points.push_back(Point(1, -1));
        
        // 添加10个随机内部点
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-0.9, 0.9);
        
        for (int i = 0; i < 10; i++) {
            points.push_back(Point(dis(gen), dis(gen)));
        }
        
        updateVoronoi();
        update();
    }

    void performIteration() {
        iterationCount++;
        std::vector<Point> newPoints = points;
        
        // 更新内部点（跳过前4个边界点）
        for (int i = 4; i < points.size(); i++) {
            Face_handle face = vd.locate(points[i]);
            if (face->is_unbounded()) continue;
            
            std::vector<Point> cell;
            Ccb_halfedge_circulator ec_start = face->ccb();
            Ccb_halfedge_circulator ec = ec_start;
            
            do {
                if (ec->has_source()) {
                    Point p = ec->source()->point();
                    cell.push_back(p);
                }
            } while (++ec != ec_start);
            
            // 裁剪多边形到边界
            clipVoronoiToBoundary(cell);
            
            // 计算重心并更新点位置
            if (!cell.empty()) {
                Point centroid = computeCentroid(cell);
                newPoints[i] = centroid;
            }
        }
        
        points = newPoints;
        updateVoronoi();
        update();
    }

    int getIterationCount() const { return iterationCount; }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 设置正交投影
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1.1, 1.1, -1.1, 1.1, -1.0, 1.0);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // 绘制边界
        glColor3f(0.3f, 0.3f, 0.3f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f(-1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(1.0f, -1.0f);
        glEnd();
        
        // 绘制Voronoi边
        glColor3f(0.2f, 0.5f, 0.8f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        
        for (Voronoi::Edge_iterator eit = vd.edges_begin(); eit != vd.edges_end(); ++eit) {
            if (eit->is_segment()) {
                Segment s = eit->dual()->segment();
                glVertex2f(s.source().x(), s.source().y());
                glVertex2f(s.target().x(), s.target().y());
            }
        }
        
        glEnd();
        
        // 绘制点
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        
        // 绘制边界点（红色）
        glColor3f(1.0f, 0.2f, 0.2f);
        for (int i = 0; i < 4; i++) {
            glVertex2f(points[i].x(), points[i].y());
        }
        
        // 绘制内部点（绿色）
        glColor3f(0.2f, 1.0f, 0.2f);
        for (int i = 4; i < points.size(); i++) {
            glVertex2f(points[i].x(), points[i].y());
        }
        
        glEnd();
    }

private:
    void updateVoronoi() {
        vd.clear();
        vd.insert(points.begin(), points.end());
    }

    void clipVoronoiToBoundary(std::vector<Point>& cell) {
        // 边界矩形 [-1, -1] 到 [1, 1]
        double xmin = -1.0, xmax = 1.0;
        double ymin = -1.0, ymax = 1.0;
        
        std::vector<Point> clipped;
        
        // 只保留边界内的点
        for (const Point& p : cell) {
            if (p.x() >= xmin && p.x() <= xmax && 
                p.y() >= ymin && p.y() <= ymax) {
                clipped.push_back(p);
            }
        }
        
        // 添加边界交点
        for (size_t i = 0; i < clipped.size(); i++) {
            size_t j = (i + 1) % clipped.size();
            Point p1 = clipped[i];
            Point p2 = clipped[j];
            
            // 检查是否需要添加交点
            if ((p1.x() < xmin && p2.x() >= xmin) || (p1.x() > xmax && p2.x() <= xmax) ||
                (p1.y() < ymin && p2.y() >= ymin) || (p1.y() > ymax && p2.y() <= ymax)) {
                
                // 计算交点
                double t = 0.0;
                Point intersect;
                
                // 左边界
                if (p1.x() < xmin && p2.x() >= xmin) {
                    t = (xmin - p1.x()) / (p2.x() - p1.x());
                    intersect = Point(xmin, p1.y() + t * (p2.y() - p1.y()));
                }
                // 右边界
                else if (p1.x() > xmax && p2.x() <= xmax) {
                    t = (xmax - p1.x()) / (p2.x() - p1.x());
                    intersect = Point(xmax, p1.y() + t * (p2.y() - p1.y()));
                }
                // 下边界
                else if (p1.y() < ymin && p2.y() >= ymin) {
                    t = (ymin - p1.y()) / (p2.y() - p1.y());
                    intersect = Point(p1.x() + t * (p2.x() - p1.x()), ymin);
                }
                // 上边界
                else if (p1.y() > ymax && p2.y() <= ymax) {
                    t = (ymax - p1.y()) / (p2.y() - p1.y());
                    intersect = Point(p1.x() + t * (p2.x() - p1.x()), ymax);
                }
                
                clipped.insert(clipped.begin() + i + 1, intersect);
                i++; // 跳过新添加的点
            }
        }
        
        cell = clipped;
    }

    Point computeCentroid(const std::vector<Point>& polygon) {
        if (polygon.empty()) 
            return Point(0, 0);
        
        double area = 0.0;
        double cx = 0.0, cy = 0.0;
        size_t n = polygon.size();
        
        for (size_t i = 0; i < n; i++) {
            size_t j = (i + 1) % n;
            double factor = polygon[i].x() * polygon[j].y() - polygon[j].x() * polygon[i].y();
            area += factor;
            cx += (polygon[i].x() + polygon[j].x()) * factor;
            cy += (polygon[i].y() + polygon[j].y()) * factor;
        }
        
        area *= 0.5;
        cx /= (6.0 * area);
        cy /= (6.0 * area);
        
        return Point(cx, cy);
    }

    std::vector<Point> points;
    Voronoi vd;
    int iterationCount;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Voronoi迭代可视化");
        
        // 创建OpenGL部件
        glWidget = new GLWidget(this);
        
        // 创建按钮
        QPushButton *iterateButton = new QPushButton("执行迭代", this);
        QPushButton *resetButton = new QPushButton("重置点集", this);
        
        // 状态栏
        statusLabel = new QLabel("迭代次数: 0", this);
        statusBar()->addWidget(statusLabel);
        
        // 连接按钮信号
        connect(iterateButton, &QPushButton::clicked, this, &MainWindow::onIterate);
        connect(resetButton, &QPushButton::clicked, this, &MainWindow::onReset);
        
        // 布局
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        
        // 添加OpenGL部件
        mainLayout->addWidget(glWidget);
        
        // 添加按钮
        QHBoxLayout *buttonLayout = new QHBoxLayout;
        buttonLayout->addWidget(iterateButton);
        buttonLayout->addWidget(resetButton);
        mainLayout->addLayout(buttonLayout);
        
        setCentralWidget(centralWidget);
    }

private slots:
    void onIterate() {
        glWidget->performIteration();
        statusLabel->setText(QString("迭代次数: %1").arg(glWidget->getIterationCount()));
    }

    void onReset() {
        glWidget->resetPoints();
        statusLabel->setText("迭代次数: 0");
    }

private:
    GLWidget *glWidget;
    QLabel *statusLabel;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow mainWindow;
    mainWindow.resize(800, 800);
    mainWindow.show();
    
    return app.exec();
}

#include "main.moc"