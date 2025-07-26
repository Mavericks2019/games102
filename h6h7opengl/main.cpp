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
#include "../tabs/cvt_weight_tab.h" // 新增头文件

// 声明模型标签页函数
QWidget* createModelTab(GLWidget* glWidget);
QWidget* createModelControlPanel(GLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow, QTabWidget* tabWidget);

// 声明参数化标签页函数
QWidget* createParameterizationTab(GLWidget* glWidget);
QWidget* createParameterizationControlPanel(GLWidget* glWidget, QWidget* paramTab);

// 声明CVT标签页函数
QWidget* createCVTTab(CVTGLWidget* glWidget);
QWidget* createCVTControlPanel(CVTGLWidget* glWidget, QWidget* cvtTab);

// 声明CVT Weight标签页函数
QWidget* createCVTWeightTab(CVTImageGLWidget* glWidget); // 新增
QWidget* createCVTWeightControlPanel(CVTImageGLWidget* glWidget, QWidget* cvtWeightTab); // 新增

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

    // 创建图像信息显示组（新增）
    QGroupBox* createImageInfoGroup(QLabel** infoLabel = nullptr) {
        QGroupBox *group = new QGroupBox("Image Information");
        QVBoxLayout *layout = new QVBoxLayout(group);
        
        QLabel *label = new QLabel("No image loaded");
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
    CVTGLWidget *cvtglWidget = new CVTGLWidget;
    CVTImageGLWidget *cvtImageWidget = new CVTImageGLWidget; // 新增
    
    // 创建标签页控件
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 1px solid #444;"
        "    border-radius: 8px;"
        "    margin-top: 4px;"
        "}"
        "QTabBar::tab {"
        "    background: #505050;"
        "    color: white;"
        "    padding: 8px 16px;"
        "    border-top-left-radius: 8px;"
        "    border-top-right-radius: 8px;"
        "    border: 1px solid #666;"
        "    border-bottom: none;"
        "    margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "    background: #606060;"
        "    border-color: #888;"
        "}"
        "QTabBar::tab:!selected {"
        "    background: #404040;"
        "    border-color: #555;"
        "}"
        "QTabBar::tab:first {"
        "    margin-left: 4px;"
        "}"
    );
    
    // 创建OBJ模型标签页
    QWidget *modelTab = createModelTab(glWidget);
    tabWidget->addTab(modelTab, "OBJ Model");
    
    // 创建参数化标签页
    QWidget *paramTab = createParameterizationTab(glWidget);
    tabWidget->addTab(paramTab, "Parameterization");
    
    // 创建CVT标签页
    QWidget *cvtTab = createCVTTab(cvtglWidget);
    tabWidget->addTab(cvtTab, "CVT");

    // 新增CVT Weight标签页
    QWidget *cvtWeightTab = createCVTWeightTab(cvtImageWidget); // 新增
    tabWidget->addTab(cvtWeightTab, "CVT Weight"); // 新增

    // 添加标签页到主布局
    mainLayout->addWidget(tabWidget, 8); // 8:2比例

    // 创建右侧控制面板
    QWidget *controlPanel = new QWidget;
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(400);
    
    // ==== 颜色设置组 ====
    controlLayout->addWidget(UIUtils::createColorSettingsGroup(glWidget, &mainWindow, tabWidget));
    
    // ==== 动态控制面板 ====
    QStackedLayout *stackedLayout = new QStackedLayout;
    controlLayout->addLayout(stackedLayout);
    
    // 创建模型控制面板
    QLabel *modelInfoLabel = nullptr;
    QGroupBox *modelInfoGroup = UIUtils::createModelInfoGroup(&modelInfoLabel);
    
    QWidget* modelControlPanel = createModelControlPanel(
        glWidget, modelInfoLabel, &mainWindow, tabWidget
    );
    QVBoxLayout* modelControlLayout = new QVBoxLayout();
    modelControlLayout->addWidget(modelInfoGroup);
    modelControlLayout->addWidget(modelControlPanel);
    QWidget* modelControlContainer = new QWidget();
    modelControlContainer->setLayout(modelControlLayout);
    stackedLayout->addWidget(modelControlContainer);
    
    // 创建参数化控制面板
    QLabel *paramInfoLabel = nullptr;
    QGroupBox *paramInfoGroup = UIUtils::createModelInfoGroup(&paramInfoLabel);
    
    QWidget* paramControlPanel = createParameterizationControlPanel(
        glWidget, paramTab
    );
    QVBoxLayout* paramControlLayout = new QVBoxLayout();
    paramControlLayout->addWidget(paramInfoGroup);
    paramControlLayout->addWidget(paramControlPanel);
    QWidget* paramControlContainer = new QWidget();
    paramControlContainer->setLayout(paramControlLayout);
    stackedLayout->addWidget(paramControlContainer);
    
    // 创建CVT控制面板
    stackedLayout->addWidget(createCVTControlPanel(
        cvtglWidget, cvtTab
    ));

    // 创建CVT Weight控制面板（新增）
    stackedLayout->addWidget(createCVTWeightControlPanel(
        cvtImageWidget, cvtWeightTab
    ));
    
    // 连接标签切换信号
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [stackedLayout](int index) {
        stackedLayout->setCurrentIndex(index);
    });

    // 连接标签切换信号 - CVT视图处理
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [=](int index) {
        // 重置所有视图的CVT状态
       cvtglWidget->setCVTView(false);
        
        // 获取参数化视图
        QWidget* paramTab = tabWidget->widget(1);
        GLWidget* paramView = paramTab ? paramTab->property("leftGLWidget").value<GLWidget*>() : nullptr;
        
        // 获取CVT视图
        CVTGLWidget* cvtView = cvtTab ? cvtTab->property("cvtGLWidget").value<CVTGLWidget*>() : nullptr;
        
        if (cvtView) {
            // 如果切换到CVT标签页
            if (index == 2) {
                cvtView->setCVTView(true);
                cvtView->update();
            } else {
                // 离开CVT标签页时重置状态
                cvtView->setCVTView(false);
            }
        }
    });
    
    // 添加控制面板到主布局
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}