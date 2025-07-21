#include "glwidget/glwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QStackedLayout>
#include <QLabel>
#include <QStyleFactory>
#include <QColorDialog>
#include <QPalette>
#include "tabs/model_tab.h"
#include "tabs/parameterization_tab.h"
#include "tabs/cvt_tab.h"

// 声明模型标签页函数
QWidget* createModelTab(GLWidget* glWidget);
QWidget* createModelControlPanel(GLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow, QTabWidget* tabWidget);

// 声明参数化标签页函数
QWidget* createParameterizationTab(GLWidget* glWidget);
QWidget* createParameterizationControlPanel(GLWidget* glWidget, QWidget* paramTab);

// 声明CVT标签页函数
QWidget* createCVTTab(GLWidget* glWidget);
QWidget* createCVTControlPanel(GLWidget* glWidget, QWidget* cvtTab);

namespace UIUtils {
    // 获取当前激活的GLWidget
    GLWidget* getCurrentGLWidget(GLWidget* mainGLWidget, QTabWidget* tabWidget) {
        int currentTab = tabWidget->currentIndex();
        if (currentTab == 0) { // OBJ Model 标签页
            return mainGLWidget;
        } else if (currentTab == 1) { // Parameterization 标签页
            QWidget* paramTab = tabWidget->widget(1);
            GLWidget* leftGLWidget = paramTab->property("leftGLWidget").value<GLWidget*>();
            return leftGLWidget;
        }
        return nullptr;
    }

    // 创建模型信息显示组
    QGroupBox* createModelInfoGroup(QLabel** infoLabel = nullptr) {
        QGroupBox *group = new QGroupBox("Model Information");
        QVBoxLayout *layout = new QVBoxLayout(group);
        
        QLabel *label = new QLabel("No model loaded");
        label->setAlignment(Qt::AlignCenter);
        label->setFixedHeight(50);
        label->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
        label->setWordWrap(true);
        
        layout->addWidget(label);
        
        if (infoLabel) *infoLabel = label;
        return group;
    }

    // 创建颜色设置组
    QGroupBox* createColorSettingsGroup(GLWidget* glWidget, QWidget* mainWindow, QTabWidget* tabWidget) {
        QGroupBox *group = new QGroupBox("Color Settings");
        QVBoxLayout *layout = new QVBoxLayout(group);
        layout->setSpacing(10);
        
        // 背景颜色按钮
        QPushButton *bgColorButton = new QPushButton("Change Background Color");
        bgColorButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #505050;"
            "   color: white;"
            "   border: none;"
            "   padding: 10px 20px;"
            "   font-size: 16px;"
            "   border-radius: 5px;"
            "}"
            "QPushButton:hover { background-color: #606060; }"
        );
        QObject::connect(bgColorButton, &QPushButton::clicked, [glWidget, mainWindow, tabWidget]() {
            QColor color = QColorDialog::getColor(Qt::black, mainWindow, "Select Background Color");
            if (color.isValid()) {
                GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
                if (targetWidget) {
                    targetWidget->setBackgroundColor(color);
                }
            }
        });
        layout->addWidget(bgColorButton);
        
        // 线框颜色按钮
        QPushButton *lineColorButton = new QPushButton("Change Wireframe Color");
        lineColorButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #505050;"
            "   color: white;"
            "   border: none;"
            "   padding: 10px 20px;"
            "   font-size: 16px;"
            "   border-radius: 5px;"
            "}"
            "QPushButton:hover { background-color: #606060; }"
        );
        QObject::connect(lineColorButton, &QPushButton::clicked, [glWidget, mainWindow, tabWidget]() {
            QColor color = QColorDialog::getColor(Qt::red, mainWindow, "Select Wireframe Color");
            if (color.isValid()) {
                GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
                if (targetWidget) {
                    targetWidget->setWireframeColor(QVector4D(
                        color.redF(), 
                        color.greenF(), 
                        color.blueF(), 
                        1.0f
                    ));
                }
            }
        });
        layout->addWidget(lineColorButton);
        
        // 表面颜色按钮
        QPushButton *surfaceColorButton = new QPushButton("Change Surface Color");
        surfaceColorButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #505050;"
            "   color: white;"
            "   border: none;"
            "   padding: 10px 20px;"
            "   font-size: 16px;"
            "   border-radius: 5px;"
            "}"
            "QPushButton:hover { background-color: #606060; }"
        );
        QObject::connect(surfaceColorButton, &QPushButton::clicked, [glWidget, mainWindow, tabWidget]() {
            QColor color = QColorDialog::getColor(QColor(179, 179, 204), mainWindow, "Select Surface Color");
            if (color.isValid()) {
                GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
                if (targetWidget) {
                    targetWidget->setSurfaceColor(QVector3D(
                        color.redF(), 
                        color.greenF(), 
                        color.blueF()
                    ));
                }
            }
        });
        layout->addWidget(surfaceColorButton);
        
        // 高光控制复选框
        QCheckBox *specularCheckbox = new QCheckBox("Disable Specular Highlight");
        specularCheckbox->setStyleSheet("color: white;");
        QObject::connect(specularCheckbox, &QCheckBox::stateChanged, [glWidget, tabWidget](int state) {
            GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
            if (targetWidget) {
                targetWidget->setSpecularEnabled(state != Qt::Checked);
            }
        });
        layout->addWidget(specularCheckbox);
        
        layout->addStretch();
        return group;
    }

    // 应用深色主题
    void applyDarkTheme(QApplication& app) {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
        app.setPalette(palette);
        
        QFont defaultFont("Arial", 12);
        app.setFont(defaultFont);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    UIUtils::applyDarkTheme(app);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.resize(2480, 1200);
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(&mainWindow);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建OpenGL窗口
    GLWidget *glWidget = new GLWidget;
    
    // 创建标签页控件
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #444; }"
        "QTabBar::tab { background: #505050; color: white; padding: 8px; }"
        "QTabBar::tab:selected { background: #606060; }"
    );
    
    // 创建OBJ模型标签页
    QWidget *modelTab = createModelTab(glWidget);
    tabWidget->addTab(modelTab, "OBJ Model");
    
    // 创建参数化标签页
    QWidget *paramTab = createParameterizationTab(glWidget);
    tabWidget->addTab(paramTab, "Parameterization");
    
    // 创建CVT标签页
    QWidget *cvtTab = createCVTTab(glWidget);
    tabWidget->addTab(cvtTab, "CVT");
    
    // 添加标签页到主布局
    mainLayout->addWidget(tabWidget, 8); // 8:2比例

    // 创建右侧控制面板
    QWidget *controlPanel = new QWidget;
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(400);
    
    // ==== 模型信息组 ====
    QLabel *modelInfoLabel = nullptr;
    controlLayout->addWidget(UIUtils::createModelInfoGroup(&modelInfoLabel));
    
    // ==== 颜色设置组 ====
    controlLayout->addWidget(UIUtils::createColorSettingsGroup(glWidget, &mainWindow, tabWidget));
    
    // ==== 动态控制面板 ====
    QStackedLayout *stackedLayout = new QStackedLayout;
    controlLayout->addLayout(stackedLayout);
    
    // 创建模型控制面板
    stackedLayout->addWidget(createModelControlPanel(
        glWidget, modelInfoLabel, &mainWindow, tabWidget
    ));
    
    // 创建参数化控制面板
    stackedLayout->addWidget(createParameterizationControlPanel(
        glWidget, paramTab
    ));
    
    // 创建CVT控制面板
    stackedLayout->addWidget(createCVTControlPanel(
        glWidget, cvtTab
    ));
    
    // 连接标签切换信号
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [stackedLayout](int index) {
        stackedLayout->setCurrentIndex(index);
    });
    
    // 添加控制面板到主布局
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}