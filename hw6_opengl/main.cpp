#include "glwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QStyleFactory>
#include <QPalette>
#include <QGroupBox>
#include <QFormLayout>
#include <QStackedLayout>
#include <QTabWidget>
#include <QFileInfo>
#include <QSlider>
#include <QColorDialog>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

// 创建网格操作组
QGroupBox* createMeshOpGroup(GLWidget* glWidget) {
    QGroupBox *meshOpGroup = new QGroupBox("Mesh Operations");
    QVBoxLayout *meshOpLayout = new QVBoxLayout(meshOpGroup);
    
    // 创建滑动条
    QSlider *meshOpSlider = new QSlider(Qt::Horizontal);
    meshOpSlider->setRange(0, 100);
    meshOpSlider->setValue(50);
    meshOpSlider->setTickPosition(QSlider::TicksBelow);
    meshOpSlider->setTickInterval(10);
    
    // 创建标签显示当前操作
    QLabel *meshOpLabel = new QLabel("Current: Original Mesh");
    meshOpLabel->setAlignment(Qt::AlignCenter);
    meshOpLabel->setStyleSheet("font-weight: bold; color: white;");
    
    // 连接滑动条信号
    QObject::connect(meshOpSlider, &QSlider::valueChanged, [glWidget, meshOpLabel](int value) {
        if (value != 100) {
            float ratio = 0.1f + (value / 50.0f) * 0.9f;
            meshOpLabel->setText(QString("Simplify: %1%").arg(qRound(100 * (1.0 - ratio))));
        } else {
            meshOpLabel->setText("Original Mesh");
        }
        
        // 应用网格操作
        glWidget->applyMeshOperation(value);
    });
    
    // 添加滑动条和标签
    meshOpLayout->addWidget(new QLabel("Original                                                     simplify"));
    meshOpLayout->addWidget(meshOpSlider);
    meshOpLayout->addWidget(meshOpLabel);
    
    // 添加重置按钮
    QPushButton *resetMeshOpButton = new QPushButton("Reset to Original");
    resetMeshOpButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(resetMeshOpButton, &QPushButton::clicked, [glWidget, meshOpSlider, meshOpLabel]() {
        meshOpSlider->setValue(0);
        meshOpLabel->setText("Original Mesh");
        glWidget->resetMeshOperation();
    });
    meshOpLayout->addWidget(resetMeshOpButton);
    
    return meshOpGroup;
}

// 创建Loop细分控制组
QGroupBox* createLoopSubdivisionGroup(GLWidget* glWidget) {
    QGroupBox *loopGroup = new QGroupBox("Loop Subdivision");
    QVBoxLayout *loopLayout = new QVBoxLayout(loopGroup);
    
    // 创建当前细分级别标签
    QLabel *levelLabel = new QLabel("Current Level: 0");
    levelLabel->setAlignment(Qt::AlignCenter);
    levelLabel->setStyleSheet("font-weight: bold; color: white;");
    
    // 创建细分按钮
    QPushButton *subdivideButton = new QPushButton("Perform Loop Subdivision");
    subdivideButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
        "QPushButton:disabled { background-color: #404040; color: #808080; }"
    );
    
    // 连接按钮信号
    QObject::connect(subdivideButton, &QPushButton::clicked, [glWidget, levelLabel, subdivideButton]() {
        // 执行细分操作
        glWidget->performLoopSubdivision();
        
        // 更新级别标签
        int currentLevel = glWidget->getCurrentSubdivisionLevel();
        levelLabel->setText(QString("Current Level: %1").arg(currentLevel));
        
        // 如果达到最大级别，禁用按钮
        if (currentLevel >= 3) {
            subdivideButton->setEnabled(false);
        }
    });
    
    // 添加重置按钮
    QPushButton *resetButton = new QPushButton("Reset Subdivision");
    resetButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget, levelLabel, subdivideButton]() {
        glWidget->resetMeshOperation();
        levelLabel->setText("Current Level: 0");
        subdivideButton->setEnabled(true);
    });
    
    // 添加控件到布局
    loopLayout->addWidget(levelLabel);
    loopLayout->addWidget(subdivideButton);
    loopLayout->addWidget(resetButton);
    
    return loopGroup;
}

// 创建模型信息组
QGroupBox* createModelInfoGroup() {
    QGroupBox *pointGroup = new QGroupBox("Model Information");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    QLabel *pointInfoLabel = new QLabel("No model loaded");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(50);
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    pointInfoLabel->setWordWrap(true);
    
    pointLayout->addWidget(pointInfoLabel);
    return pointGroup;
}

// 创建颜色设置组
QGroupBox* createColorGroup(GLWidget* glWidget, QWidget* mainWindow) {
    QGroupBox *colorGroup = new QGroupBox("Color Settings");
    QVBoxLayout *colorLayout = new QVBoxLayout(colorGroup);
    colorLayout->setSpacing(10);
    
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
    QObject::connect(bgColorButton, &QPushButton::clicked, [glWidget, mainWindow]() {
        QColor color = QColorDialog::getColor(Qt::black, mainWindow, "Select Background Color");
        if (color.isValid()) {
            glWidget->setBackgroundColor(color);
        }
    });
    colorLayout->addWidget(bgColorButton);
    
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
    QObject::connect(lineColorButton, &QPushButton::clicked, [glWidget, mainWindow]() {
        QColor color = QColorDialog::getColor(Qt::red, mainWindow, "Select Wireframe Color");
        if (color.isValid()) {
            glWidget->setWireframeColor(QVector4D(
                color.redF(), 
                color.greenF(), 
                color.blueF(), 
                1.0f
            ));
        }
    });
    colorLayout->addWidget(lineColorButton);
    
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
    QObject::connect(surfaceColorButton, &QPushButton::clicked, [glWidget, mainWindow]() {
        QColor color = QColorDialog::getColor(QColor(179, 179, 204), mainWindow, "Select Surface Color");
        if (color.isValid()) {
            glWidget->setSurfaceColor(QVector3D(
                color.redF(), 
                color.greenF(), 
                color.blueF()
            ));
        }
    });
    colorLayout->addWidget(surfaceColorButton);
    
    // 关闭高光复选框
    QCheckBox *disableSpecularCheckbox = new QCheckBox("Disable Specular Highlight");
    disableSpecularCheckbox->setStyleSheet("color: white;");
    QObject::connect(disableSpecularCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setSpecularEnabled(state != Qt::Checked);
    });
    colorLayout->addWidget(disableSpecularCheckbox);
    
    colorLayout->addStretch();
    return colorGroup;
}

// 创建OBJ加载按钮
QWidget* createLoadButton(GLWidget* glWidget, QLabel* pointInfoLabel, QWidget* mainWindow) {
    QPushButton *loadButton = new QPushButton("Load OBJ File");
    loadButton->setStyleSheet(
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
    QObject::connect(loadButton, &QPushButton::clicked, [glWidget, pointInfoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            glWidget->loadOBJ(filePath);
            pointInfoLabel->setText("Model loaded: " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName());
        }
    });
    return loadButton;
}

// 创建渲染模式选择组
QGroupBox* createRenderModeGroup(GLWidget* glWidget) {
    QGroupBox *renderModeGroup = new QGroupBox("Rendering Mode");
    QVBoxLayout *renderModeLayout = new QVBoxLayout(renderModeGroup);
    
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
    QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
    QRadioButton *maxRadio = new QRadioButton("Max Curvature");
    
    solidRadio->setChecked(true);
    
    renderModeLayout->addWidget(solidRadio);
    renderModeLayout->addWidget(gaussianRadio);
    renderModeLayout->addWidget(meanRadio);
    renderModeLayout->addWidget(maxRadio);
    
    QObject::connect(solidRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::BlinnPhong);
    });
    QObject::connect(gaussianRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::GaussianCurvature);
    });
    QObject::connect(meanRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::MeanCurvature);
    });
    QObject::connect(maxRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(GLWidget::MaxCurvature);
    });
    
    return renderModeGroup;
}

// 创建显示选项组
QWidget* createDisplayOptions(GLWidget* glWidget) {
    QWidget *displayOptions = new QWidget;
    QVBoxLayout *optionsLayout = new QVBoxLayout(displayOptions);
    
    QCheckBox *wireframeOverlayCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeOverlayCheckbox->setStyleSheet("color: white;");
    QObject::connect(wireframeOverlayCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setShowWireframeOverlay(state == Qt::Checked);
    });
    
    QCheckBox *hideFacesCheckbox = new QCheckBox("Hide Faces");
    hideFacesCheckbox->setStyleSheet("color: white;");
    QObject::connect(hideFacesCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setHideFaces(state == Qt::Checked);
    });
    
    optionsLayout->addWidget(wireframeOverlayCheckbox);
    optionsLayout->addWidget(hideFacesCheckbox);
    return displayOptions;
}

// 创建重置视图按钮
QPushButton* createResetViewButton(GLWidget* glWidget) {
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet(
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
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget]() {
        glWidget->resetView();
    });
    return resetButton;
}

// 创建迭代方法选择组
QGroupBox* createMethodGroup(GLWidget* glWidget) {
    QGroupBox *methodGroup = new QGroupBox("Iteration Method");
    methodGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    
    QRadioButton *uniformRadio = new QRadioButton("Uniform Laplacian");
    QRadioButton *cotangentRadio = new QRadioButton("Cotangent Weights");
    QRadioButton *cotangentAreaRadio = new QRadioButton("Cotangent with Area (Laplace-Beltrami)"); 
    QRadioButton *eigenRadio = new QRadioButton("Eigen Sparse Solver"); // 新增Eigen求解器选项

    uniformRadio->setChecked(true);
    
    methodLayout->addWidget(uniformRadio);
    methodLayout->addWidget(cotangentRadio);
    methodLayout->addWidget(cotangentAreaRadio);
    methodLayout->addWidget(eigenRadio); // 添加新选项

    QObject::connect(uniformRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setIterationMethod(GLWidget::UniformLaplacian);
    });
    QObject::connect(cotangentRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setIterationMethod(GLWidget::CotangentWeights);
    });
    QObject::connect(cotangentAreaRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setIterationMethod(GLWidget::CotangentWithArea);
    });
    QObject::connect(eigenRadio, &QRadioButton::clicked, [glWidget]() { // 连接新选项
        glWidget->setIterationMethod(GLWidget::EigenSparseSolver);
    });

    return methodGroup;
}

// 创建极小曲面迭代控制组
QGroupBox* createMinimalSurfaceGroup(GLWidget* glWidget) {
    QGroupBox *minimalSurfaceGroup = new QGroupBox("Minimal Surface Iteration");
    minimalSurfaceGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QFormLayout *minimalLayout = new QFormLayout(minimalSurfaceGroup);
    
    QSpinBox *iterationsSpinBox = new QSpinBox;
    iterationsSpinBox->setRange(1, 1000);
    iterationsSpinBox->setValue(10);
    
    QDoubleSpinBox *lambdaSpinBox = new QDoubleSpinBox;
    lambdaSpinBox->setRange(0.0001, 0.5);
    lambdaSpinBox->setValue(0.1);
    lambdaSpinBox->setSingleStep(0.01);
    lambdaSpinBox->setDecimals(4);
    
    QPushButton *applyIterationButton = new QPushButton("Apply Iteration");
    applyIterationButton->setStyleSheet(
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

    // 新增：Eigen求解按钮
    QPushButton *eigenSolveButton = new QPushButton("Solve with Eigen");
    eigenSolveButton->setStyleSheet(
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
    
    QObject::connect(applyIterationButton, &QPushButton::clicked, [glWidget, iterationsSpinBox, lambdaSpinBox]() {
        glWidget->performMinimalSurfaceIteration(
            iterationsSpinBox->value(),
            lambdaSpinBox->value()
        );
    });

    // 连接Eigen求解按钮
    QObject::connect(eigenSolveButton, &QPushButton::clicked, [glWidget]() {
        glWidget->setIterationMethod(GLWidget::EigenSparseSolver);
        glWidget->performMinimalSurfaceIteration(0, 0); // 参数无意义，但会调用Eigen求解器
    });
    
    minimalLayout->addRow("Iterations:", iterationsSpinBox);
    minimalLayout->addRow("Step Size (λ):", lambdaSpinBox);
    minimalLayout->addRow(applyIterationButton);
    minimalLayout->addRow(eigenSolveButton); // 添加新按钮

    return minimalSurfaceGroup;
}

// 创建OBJ控制面板（使用拆分后的子函数）
QWidget* createOBJControlPanel(GLWidget* glWidget, QLabel* pointInfoLabel, QWidget* mainWindow) {
    QWidget *objControlPanel = new QWidget;
    QVBoxLayout *objControlLayout = new QVBoxLayout(objControlPanel);
    
    // 使用子函数创建各个控件组
    objControlLayout->addWidget(createLoadButton(glWidget, pointInfoLabel, mainWindow));
    objControlLayout->addWidget(createRenderModeGroup(glWidget));
    objControlLayout->addWidget(createDisplayOptions(glWidget));
    objControlLayout->addWidget(createResetViewButton(glWidget));
    objControlLayout->addWidget(createMethodGroup(glWidget));
    objControlLayout->addWidget(createMinimalSurfaceGroup(glWidget));
    
    objControlLayout->addStretch();
    return objControlPanel;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置Fusion样式
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 设置深色主题
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
    
    // 设置字体
    QFont defaultFont("Arial", 12);
    app.setFont(defaultFont);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.resize(1920, 1080);
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(&mainWindow);
    
    // 创建OpenGL窗口
    GLWidget *glWidget = new GLWidget;
    
    // 创建TabWidget
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->addTab(glWidget, "OBJ Model");
    mainLayout->addWidget(tabWidget, 5);
    
    // 创建右侧控制面板
    QWidget *controlPanel = new QWidget;
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(400);

    // ==== 模型信息组 ====
    QGroupBox *pointGroup = createModelInfoGroup();
    QLabel *pointInfoLabel = pointGroup->findChild<QLabel*>(); // 获取标签引用
    controlLayout->addWidget(pointGroup);
    
    // ==== 颜色设置组 ====
    controlLayout->addWidget(createColorGroup(glWidget, &mainWindow));
    
    // 创建堆叠布局用于切换控制面板
    QStackedLayout *stackedControlLayout = new QStackedLayout;
    controlLayout->addLayout(stackedControlLayout);

    // ==== OBJ控制面板 ====
    stackedControlLayout->addWidget(createOBJControlPanel(glWidget, pointInfoLabel, &mainWindow));

    // ==== 网格操作组 ====
    controlLayout->addWidget(createMeshOpGroup(glWidget));
    controlLayout->addWidget(createLoopSubdivisionGroup(glWidget));
    // 将控制面板添加到主布局
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}