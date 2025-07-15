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
#include <QSplitter>

// 获取当前激活的GLWidget
GLWidget* getCurrentGLWidget(GLWidget* mainGLWidget, QTabWidget* tabWidget) {
    int currentTab = tabWidget->currentIndex();
    if (currentTab == 0) { // OBJ Model 标签页
        return mainGLWidget;
    } else if (currentTab == 1) { // Parameterization 标签页
        // 获取参数化标签页中的左右视图
        QWidget* paramTab = tabWidget->widget(1);
        GLWidget* leftGLWidget = paramTab->property("leftGLWidget").value<GLWidget*>();
        // 这里返回左侧视图，或者根据需要返回右侧视图
        return leftGLWidget;
    }
    return nullptr;
}

// 创建网格操作组
QGroupBox* createMeshOpGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
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
    QObject::connect(meshOpSlider, &QSlider::valueChanged, [glWidget, meshOpLabel, tabWidget](int value) {
        if (value != 100) {
            float ratio = 0.1f + (value / 50.0f) * 0.9f;
            meshOpLabel->setText(QString("Simplify: %1%").arg(qRound(100 * (1.0 - ratio))));
        } else {
            meshOpLabel->setText("Original Mesh");
        }
        
        // 应用网格操作到当前视图
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->applyMeshOperation(value);
        }
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
    QObject::connect(resetMeshOpButton, &QPushButton::clicked, [glWidget, meshOpSlider, meshOpLabel, tabWidget]() {
        meshOpSlider->setValue(0);
        meshOpLabel->setText("Original Mesh");
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->resetMeshOperation();
        }
    });
    meshOpLayout->addWidget(resetMeshOpButton);
    
    return meshOpGroup;
}

// 创建参数化选项卡
QWidget* createParameterizationTab(GLWidget* glWidget) {
    QWidget *paramTab = new QWidget;
    QHBoxLayout *paramLayout = new QHBoxLayout(paramTab);
    
    // 使用分割器创建左右两个视图
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // 创建专用左侧视图
    GLWidget *leftGLWidget = new GLWidget;
    
    // 创建右侧视图并初始化
    GLWidget *rightGLWidget = new GLWidget;
    
    // 添加视图到分割器
    splitter->addWidget(leftGLWidget);
    splitter->addWidget(rightGLWidget);
    splitter->setSizes(QList<int>() << 500 << 500); // 平均分配空间
    
    paramLayout->addWidget(splitter);
    
    // 保存视图指针供后续使用
    paramTab->setProperty("leftGLWidget", QVariant::fromValue(leftGLWidget));
    paramTab->setProperty("rightGLWidget", QVariant::fromValue(rightGLWidget));
    
    return paramTab;
}

// 创建Loop细分控制组
QGroupBox* createLoopSubdivisionGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
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
    QObject::connect(subdivideButton, &QPushButton::clicked, [glWidget, levelLabel, subdivideButton, tabWidget]() {
        // 执行细分操作到当前视图
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->performLoopSubdivision();
            
            // 更新级别标签
            int currentLevel = targetWidget->getCurrentSubdivisionLevel();
            levelLabel->setText(QString("Current Level: %1").arg(currentLevel));
            
            // 如果达到最大级别，禁用按钮
            if (currentLevel >= 3) {
                subdivideButton->setEnabled(false);
            }
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
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget, levelLabel, subdivideButton, tabWidget]() {
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->resetMeshOperation();
            levelLabel->setText("Current Level: 0");
            subdivideButton->setEnabled(true);
        }
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
QGroupBox* createColorGroup(GLWidget* glWidget, QWidget* mainWindow, QTabWidget* tabWidget) {
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
    QObject::connect(bgColorButton, &QPushButton::clicked, [glWidget, mainWindow, tabWidget]() {
        QColor color = QColorDialog::getColor(Qt::black, mainWindow, "Select Background Color");
        if (color.isValid()) {
            // 设置当前视图的背景色
            GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
            if (targetWidget) {
                targetWidget->setBackgroundColor(color);
            }
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
    QObject::connect(lineColorButton, &QPushButton::clicked, [glWidget, mainWindow, tabWidget]() {
        QColor color = QColorDialog::getColor(Qt::red, mainWindow, "Select Wireframe Color");
        if (color.isValid()) {
            // 设置当前视图的线框颜色
            GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
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
    QObject::connect(surfaceColorButton, &QPushButton::clicked, [glWidget, mainWindow, tabWidget]() {
        QColor color = QColorDialog::getColor(QColor(179, 179, 204), mainWindow, "Select Surface Color");
        if (color.isValid()) {
            // 设置当前视图的表面颜色
            GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
            if (targetWidget) {
                targetWidget->setSurfaceColor(QVector3D(
                    color.redF(), 
                    color.greenF(), 
                    color.blueF()
                ));
            }
        }
    });
    colorLayout->addWidget(surfaceColorButton);
    
    // 关闭高光复选框
    QCheckBox *disableSpecularCheckbox = new QCheckBox("Disable Specular Highlight");
    disableSpecularCheckbox->setStyleSheet("color: white;");
    QObject::connect(disableSpecularCheckbox, &QCheckBox::stateChanged, [glWidget, tabWidget](int state) {
        // 设置当前视图的高光状态
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setSpecularEnabled(state != Qt::Checked);
        }
    });
    colorLayout->addWidget(disableSpecularCheckbox);
    
    colorLayout->addStretch();
    return colorGroup;
}

// 创建OBJ加载按钮
QWidget* createLoadButton(GLWidget* glWidget, QLabel* pointInfoLabel, QWidget* mainWindow, QTabWidget* tabWidget) {
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
    QObject::connect(loadButton, &QPushButton::clicked, [glWidget, pointInfoLabel, mainWindow, tabWidget]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            // 加载到当前视图
            GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
            if (targetWidget) {
                targetWidget->loadOBJ(filePath);
                pointInfoLabel->setText("Model loaded: " + QFileInfo(filePath).fileName());
                mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName());
            }
        }
    });
    return loadButton;
}

// 创建渲染模式选择组
QGroupBox* createRenderModeGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
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
    
    QObject::connect(solidRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        // 设置当前视图的渲染模式
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setRenderMode(GLWidget::BlinnPhong);
        }
    });
    QObject::connect(gaussianRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setRenderMode(GLWidget::GaussianCurvature);
        }
    });
    QObject::connect(meanRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setRenderMode(GLWidget::MeanCurvature);
        }
    });
    QObject::connect(maxRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setRenderMode(GLWidget::MaxCurvature);
        }
    });
    
    return renderModeGroup;
}

// 创建显示选项组
QWidget* createDisplayOptions(GLWidget* glWidget, QTabWidget* tabWidget) {
    QWidget *displayOptions = new QWidget;
    QVBoxLayout *optionsLayout = new QVBoxLayout(displayOptions);
    
    QCheckBox *wireframeOverlayCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeOverlayCheckbox->setStyleSheet("color: white;");
    QObject::connect(wireframeOverlayCheckbox, &QCheckBox::stateChanged, [glWidget, tabWidget](int state) {
        // 设置当前视图的线框覆盖显示
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setShowWireframeOverlay(state == Qt::Checked);
        }
    });
    
    QCheckBox *hideFacesCheckbox = new QCheckBox("Hide Faces");
    hideFacesCheckbox->setStyleSheet("color: white;");
    QObject::connect(hideFacesCheckbox, &QCheckBox::stateChanged, [glWidget, tabWidget](int state) {
        // 设置当前视图的面隐藏
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setHideFaces(state == Qt::Checked);
        }
    });
    
    optionsLayout->addWidget(wireframeOverlayCheckbox);
    optionsLayout->addWidget(hideFacesCheckbox);
    return displayOptions;
}

// 创建重置视图按钮
QPushButton* createResetViewButton(GLWidget* glWidget, QTabWidget* tabWidget) {
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
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget, tabWidget]() {
        // 重置当前视图
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->resetView();
        }
    });
    return resetButton;
}

// 创建迭代方法选择组
QGroupBox* createMethodGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
    QGroupBox *methodGroup = new QGroupBox("Iteration Method");
    methodGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    
    QRadioButton *uniformRadio = new QRadioButton("Uniform Laplacian");
    QRadioButton *cotangentRadio = new QRadioButton("Cotangent Weights");
    QRadioButton *cotangentAreaRadio = new QRadioButton("Cotangent with Area (Laplace-Beltrami)"); 
    QRadioButton *eigenRadio = new QRadioButton("Eigen Sparse Solver");

    uniformRadio->setChecked(true);
    
    methodLayout->addWidget(uniformRadio);
    methodLayout->addWidget(cotangentRadio);
    methodLayout->addWidget(cotangentAreaRadio);
    methodLayout->addWidget(eigenRadio);

    QObject::connect(uniformRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        // 设置当前视图的迭代方法
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setIterationMethod(GLWidget::UniformLaplacian);
        }
    });
    QObject::connect(cotangentRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setIterationMethod(GLWidget::CotangentWeights);
        }
    });
    QObject::connect(cotangentAreaRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setIterationMethod(GLWidget::CotangentWithArea);
        }
    });
    QObject::connect(eigenRadio, &QRadioButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setIterationMethod(GLWidget::EigenSparseSolver);
        }
    });

    return methodGroup;
}

// 创建极小曲面迭代控制组
QGroupBox* createMinimalSurfaceGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
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

    // Eigen求解按钮
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
    
    QObject::connect(applyIterationButton, &QPushButton::clicked, [glWidget, iterationsSpinBox, lambdaSpinBox, tabWidget]() {
        // 对当前视图执行迭代
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->performMinimalSurfaceIteration(
                iterationsSpinBox->value(),
                lambdaSpinBox->value()
            );
        }
    });

    // 连接Eigen求解按钮
    QObject::connect(eigenSolveButton, &QPushButton::clicked, [glWidget, tabWidget]() {
        // 对当前视图使用Eigen求解器
        GLWidget* targetWidget = getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setIterationMethod(GLWidget::EigenSparseSolver);
            targetWidget->performMinimalSurfaceIteration(0, 0); // 参数无意义，但会调用Eigen求解器
        }
    });
    
    minimalLayout->addRow("Iterations:", iterationsSpinBox);
    minimalLayout->addRow("Step Size (λ):", lambdaSpinBox);
    minimalLayout->addRow(applyIterationButton);
    minimalLayout->addRow(eigenSolveButton);

    return minimalSurfaceGroup;
}

// 创建OBJ控制面板
QWidget* createOBJControlPanel(GLWidget* glWidget, QLabel* pointInfoLabel, QWidget* mainWindow, QTabWidget* tabWidget) {
    QWidget *objControlPanel = new QWidget;
    QVBoxLayout *objControlLayout = new QVBoxLayout(objControlPanel);
    
    // 使用子函数创建各个控件组
    objControlLayout->addWidget(createLoadButton(glWidget, pointInfoLabel, mainWindow, tabWidget));
    objControlLayout->addWidget(createRenderModeGroup(glWidget, tabWidget));
    objControlLayout->addWidget(createDisplayOptions(glWidget, tabWidget));
    objControlLayout->addWidget(createResetViewButton(glWidget, tabWidget));
    objControlLayout->addWidget(createMethodGroup(glWidget, tabWidget));
    objControlLayout->addWidget(createMinimalSurfaceGroup(glWidget, tabWidget));
    
    objControlLayout->addStretch();
    return objControlPanel;
}

// 创建参数化控制面板
QWidget* createParameterizationControlPanel(GLWidget* glWidget, QWidget* paramTab, QTabWidget* tabWidget) {
    QWidget *paramControlPanel = new QWidget;
    QVBoxLayout *paramLayout = new QVBoxLayout(paramControlPanel);
    
    // 获取参数化选项卡中的视图
    GLWidget *leftGLWidget = paramTab->property("leftGLWidget").value<GLWidget*>();
    GLWidget *rightGLWidget = paramTab->property("rightGLWidget").value<GLWidget*>();
    
    // 添加加载OBJ按钮（加载到左侧视图）
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
    QObject::connect(loadButton, &QPushButton::clicked, [leftGLWidget]() {
        QString filePath = QFileDialog::getOpenFileName(
            nullptr, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            // 加载到左侧视图
            leftGLWidget->loadOBJ(filePath);
        }
    });
    paramLayout->addWidget(loadButton);
    
    // 添加渲染模式组（控制两个视图）
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
    
    // 连接信号：同时设置左右两个视图的渲染模式
    QObject::connect(solidRadio, &QRadioButton::clicked, [leftGLWidget, rightGLWidget]() {
        leftGLWidget->setRenderMode(GLWidget::BlinnPhong);
        rightGLWidget->setRenderMode(GLWidget::BlinnPhong);
    });
    QObject::connect(gaussianRadio, &QRadioButton::clicked, [leftGLWidget, rightGLWidget]() {
        leftGLWidget->setRenderMode(GLWidget::GaussianCurvature);
        rightGLWidget->setRenderMode(GLWidget::GaussianCurvature);
    });
    QObject::connect(meanRadio, &QRadioButton::clicked, [leftGLWidget, rightGLWidget]() {
        leftGLWidget->setRenderMode(GLWidget::MeanCurvature);
        rightGLWidget->setRenderMode(GLWidget::MeanCurvature);
    });
    QObject::connect(maxRadio, &QRadioButton::clicked, [leftGLWidget, rightGLWidget]() {
        leftGLWidget->setRenderMode(GLWidget::MaxCurvature);
        rightGLWidget->setRenderMode(GLWidget::MaxCurvature);
    });
    
    paramLayout->addWidget(renderModeGroup);
    
    // 添加边界选项
    QGroupBox *boundaryGroup = new QGroupBox("Boundary Type");
    boundaryGroup->setStyleSheet("QGroupBox { color: white; }");
    QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryGroup);
    
    QRadioButton *rectRadio = new QRadioButton("Rectangle");
    QRadioButton *circleRadio = new QRadioButton("Circle");
    rectRadio->setChecked(true);
    
    boundaryLayout->addWidget(rectRadio);
    boundaryLayout->addWidget(circleRadio);
    paramLayout->addWidget(boundaryGroup);
    
    // 添加执行按钮
    QPushButton *paramButton = new QPushButton("Perform Parameterization");
    paramButton->setStyleSheet(
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
    paramLayout->addWidget(paramButton);
    
    // 添加参数化选项
    QGroupBox *paramOptionsGroup = new QGroupBox("Parameterization Options");
    paramOptionsGroup->setStyleSheet("QGroupBox { color: white; }");
    QFormLayout *optionsLayout = new QFormLayout(paramOptionsGroup);
    
    QDoubleSpinBox *toleranceSpinBox = new QDoubleSpinBox;
    toleranceSpinBox->setRange(0.0001, 0.1);
    toleranceSpinBox->setValue(0.001);
    toleranceSpinBox->setDecimals(5);
    
    QSpinBox *maxIterSpinBox = new QSpinBox;
    maxIterSpinBox->setRange(10, 10000);
    maxIterSpinBox->setValue(500);
    
    optionsLayout->addRow("Convergence Tolerance:", toleranceSpinBox);
    optionsLayout->addRow("Max Iterations:", maxIterSpinBox);
    paramLayout->addWidget(paramOptionsGroup);
    
    paramLayout->addStretch();
    return paramControlPanel;
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
    mainWindow.resize(2480, 1200); // 扩大窗口尺寸
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(&mainWindow);
    mainLayout->setContentsMargins(10, 10, 10, 10); // 增加边距
    
    // 创建OpenGL窗口
    GLWidget *glWidget = new GLWidget;
    
    // 创建TabWidget
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #444; }"
        "QTabBar::tab { background: #505050; color: white; padding: 8px; }"
        "QTabBar::tab:selected { background: #606060; }"
    );
    
    // 创建OBJ选项卡
    QWidget *objTab = new QWidget;
    QHBoxLayout *objTabLayout = new QHBoxLayout(objTab);
    objTabLayout->addWidget(glWidget); // 主视图全屏显示
    tabWidget->addTab(objTab, "OBJ Model");
    
    // 创建参数化选项卡
    QWidget *paramTab = createParameterizationTab(glWidget);
    tabWidget->addTab(paramTab, "Parameterization");
    
    // 增加TabWidget在布局中的权重
    mainLayout->addWidget(tabWidget, 8); // 8:2比例分配空间

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
    controlLayout->addWidget(createColorGroup(glWidget, &mainWindow, tabWidget));
    
    // 创建堆叠布局用于切换控制面板
    QStackedLayout *stackedControlLayout = new QStackedLayout;
    controlLayout->addLayout(stackedControlLayout);

    // ==== OBJ控制面板 ====
    stackedControlLayout->addWidget(createOBJControlPanel(glWidget, pointInfoLabel, &mainWindow, tabWidget));
    
    // ==== 参数化控制面板 ====
    stackedControlLayout->addWidget(createParameterizationControlPanel(glWidget, paramTab, tabWidget));
    
    // ==== 网格操作组 ====
    controlLayout->addWidget(createMeshOpGroup(glWidget, tabWidget));
    controlLayout->addWidget(createLoopSubdivisionGroup(glWidget, tabWidget));
    
    // 连接Tab切换信号
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [stackedControlLayout](int index) {
        if (index == 1) { // 参数化选项卡
            stackedControlLayout->setCurrentIndex(1); // 显示参数化控制面板
        } else {
            stackedControlLayout->setCurrentIndex(0); // 显示普通控制面板
        }
    });
    
    // 将控制面板添加到主布局
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}