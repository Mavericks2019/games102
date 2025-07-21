#include "../glwidget/glwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QStackedLayout>
#include <QTabWidget>
#include <QFileInfo>
#include <QSlider>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

// 声明UIUtils命名空间中的函数
namespace UIUtils {
    GLWidget* getCurrentGLWidget(GLWidget* mainGLWidget, QTabWidget* tabWidget);
}

// 创建模型标签页
QWidget* createModelTab(GLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建OBJ文件加载按钮
QWidget* createModelLoadButton(GLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow, QTabWidget* tabWidget) {
    QPushButton *button = new QPushButton("Load OBJ File");
    button->setStyleSheet(
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
    QObject::connect(button, &QPushButton::clicked, [glWidget, infoLabel, mainWindow, tabWidget]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
            if (targetWidget) {
                targetWidget->loadOBJ(filePath);
                infoLabel->setText("Model loaded: " + QFileInfo(filePath).fileName());
                mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName());
            }
        }
    });
    return button;
}

// 创建渲染模式选择组
QGroupBox* createRenderingModeGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
    QGroupBox *group = new QGroupBox("Rendering Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
    QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
    QRadioButton *maxRadio = new QRadioButton("Max Curvature");
    QRadioButton *textureRadio = new QRadioButton("Texture Mapping");
    
    solidRadio->setChecked(true);
    
    layout->addWidget(solidRadio);
    layout->addWidget(gaussianRadio);
    layout->addWidget(meanRadio);
    layout->addWidget(maxRadio);
    layout->addWidget(textureRadio);
    
    // 连接渲染模式信号
    auto connectMode = [glWidget, tabWidget](QRadioButton* radio, GLWidget::RenderMode mode) {
        QObject::connect(radio, &QRadioButton::clicked, [glWidget, tabWidget, mode]() {
            GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
            if (targetWidget) {
                targetWidget->setRenderMode(mode);
            }
        });
    };
    
    connectMode(solidRadio, GLWidget::BlinnPhong);
    connectMode(gaussianRadio, GLWidget::GaussianCurvature);
    connectMode(meanRadio, GLWidget::MeanCurvature);
    connectMode(maxRadio, GLWidget::MaxCurvature);
    connectMode(textureRadio, GLWidget::TextureMapping);
    
    return group;
}

// 创建显示选项组（单视图）
QGroupBox* createDisplayOptionsGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
    QGroupBox *group = new QGroupBox("Display Options");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeCheckbox->setStyleSheet("color: white;");
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [glWidget, tabWidget](int state) {
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setShowWireframeOverlay(state == Qt::Checked);
        }
    });
    
    QCheckBox *faceCheckbox = new QCheckBox("Hide Faces");
    faceCheckbox->setStyleSheet("color: white;");
    QObject::connect(faceCheckbox, &QCheckBox::stateChanged, [glWidget, tabWidget](int state) {
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setHideFaces(state == Qt::Checked);
        }
    });
    
    layout->addWidget(wireframeCheckbox);
    layout->addWidget(faceCheckbox);
    return group;
}

// 创建视图重置按钮
QPushButton* createViewResetButton(GLWidget* glWidget, QTabWidget* tabWidget) {
    QPushButton *button = new QPushButton("Reset View");
    button->setStyleSheet(
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
    QObject::connect(button, &QPushButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->resetView();
        }
    });
    return button;
}

// 创建自适应视图按钮
QPushButton* createCenterViewButton(GLWidget* glWidget, QTabWidget* tabWidget) {
    QPushButton *button = new QPushButton("Center View");
    button->setStyleSheet(
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
    QObject::connect(button, &QPushButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->centerView();
        }
    });
    return button;
}

// 创建迭代方法选择组
QGroupBox* createIterationMethodGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
    QGroupBox *group = new QGroupBox("Iteration Method");
    group->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *uniformRadio = new QRadioButton("Uniform Laplacian");
    QRadioButton *cotangentRadio = new QRadioButton("Cotangent Weights");
    QRadioButton *cotangentAreaRadio = new QRadioButton("Cotangent with Area (Laplace-Beltrami)"); 
    QRadioButton *eigenRadio = new QRadioButton("Eigen Sparse Solver");

    uniformRadio->setChecked(true);
    
    layout->addWidget(uniformRadio);
    layout->addWidget(cotangentRadio);
    layout->addWidget(cotangentAreaRadio);
    layout->addWidget(eigenRadio);

    // 连接迭代方法信号
    auto connectMethod = [glWidget, tabWidget](QRadioButton* radio, GLWidget::IterationMethod method) {
        QObject::connect(radio, &QRadioButton::clicked, [glWidget, tabWidget, method]() {
            GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
            if (targetWidget) {
                targetWidget->setIterationMethod(method);
            }
        });
    };
    
    connectMethod(uniformRadio, GLWidget::UniformLaplacian);
    connectMethod(cotangentRadio, GLWidget::CotangentWeights);
    connectMethod(cotangentAreaRadio, GLWidget::CotangentWithArea);
    connectMethod(eigenRadio, GLWidget::EigenSparseSolver);

    return group;
}

// 创建极小曲面控制组
QGroupBox* createMinimalSurfaceGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
    QGroupBox *group = new QGroupBox("Minimal Surface Iteration");
    group->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QFormLayout *layout = new QFormLayout(group);
    
    QSpinBox *iterationsSpinBox = new QSpinBox;
    iterationsSpinBox->setRange(1, 1000);
    iterationsSpinBox->setValue(10);
    
    QDoubleSpinBox *lambdaSpinBox = new QDoubleSpinBox;
    lambdaSpinBox->setRange(0.0001, 0.5);
    lambdaSpinBox->setValue(0.1);
    lambdaSpinBox->setSingleStep(0.01);
    lambdaSpinBox->setDecimals(4);
    
    QPushButton *applyButton = new QPushButton("Apply Iteration");
    applyButton->setStyleSheet(
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

    QPushButton *eigenButton = new QPushButton("Solve with Eigen");
    eigenButton->setStyleSheet(
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
    
    QObject::connect(applyButton, &QPushButton::clicked, [glWidget, iterationsSpinBox, lambdaSpinBox, tabWidget]() {
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->performMinimalSurfaceIteration(
                iterationsSpinBox->value(),
                lambdaSpinBox->value()
            );
        }
    });

    QObject::connect(eigenButton, &QPushButton::clicked, [glWidget, tabWidget]() {
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->setIterationMethod(GLWidget::EigenSparseSolver);
            targetWidget->performMinimalSurfaceIteration(0, 0);
        }
    });
    
    layout->addRow("Iterations:", iterationsSpinBox);
    layout->addRow("Step Size (λ):", lambdaSpinBox);
    layout->addRow(applyButton);
    layout->addRow(eigenButton);

    return group;
}

// 创建网格操作控制组
QGroupBox* createMeshOperationsGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
    QGroupBox *group = new QGroupBox("Mesh Operations");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(50);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setTickInterval(10);
    
    QLabel *statusLabel = new QLabel("Current: Original Mesh");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-weight: bold; color: white;");
    
    // 连接滑动条信号
    QObject::connect(slider, &QSlider::valueChanged, [glWidget, statusLabel, tabWidget](int value) {
        if (value != 100) {
            float ratio = 0.1f + (value / 50.0f) * 0.9f;
            statusLabel->setText(QString("Simplify: %1%").arg(qRound(100 * (1.0 - ratio))));
        } else {
            statusLabel->setText("Original Mesh");
        }
        
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->applyMeshOperation(value);
        }
    });
    
    // 添加控件
    layout->addWidget(new QLabel("Original                                                     simplify"));
    layout->addWidget(slider);
    layout->addWidget(statusLabel);
    
    // 添加重置按钮
    QPushButton *resetButton = new QPushButton("Reset to Original");
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
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget, slider, statusLabel, tabWidget]() {
        slider->setValue(0);
        statusLabel->setText("Original Mesh");
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->resetMeshOperation();
        }
    });
    layout->addWidget(resetButton);
    
    return group;
}

// 创建Loop细分控制组
QGroupBox* createLoopSubdivisionGroup(GLWidget* glWidget, QTabWidget* tabWidget) {
    QGroupBox *group = new QGroupBox("Loop Subdivision");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 当前细分级别标签
    QLabel *levelLabel = new QLabel("Current Level: 0");
    levelLabel->setAlignment(Qt::AlignCenter);
    levelLabel->setStyleSheet("font-weight: bold; color: white;");
    
    // 细分按钮
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
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
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
        GLWidget* targetWidget = UIUtils::getCurrentGLWidget(glWidget, tabWidget);
        if (targetWidget) {
            targetWidget->resetMeshOperation();
            levelLabel->setText("Current Level: 0");
            subdivideButton->setEnabled(true);
        }
    });
    
    // 添加控件
    layout->addWidget(levelLabel);
    layout->addWidget(subdivideButton);
    layout->addWidget(resetButton);
    
    return group;
}

// 创建OBJ模型控制面板
QWidget* createModelControlPanel(GLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow, QTabWidget* tabWidget) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加控件组
    layout->addWidget(createModelLoadButton(glWidget, infoLabel, mainWindow, tabWidget));
    layout->addWidget(createRenderingModeGroup(glWidget, tabWidget));
    layout->addWidget(createDisplayOptionsGroup(glWidget, tabWidget)); // 单视图显示选项
    layout->addWidget(createViewResetButton(glWidget, tabWidget));
    layout->addWidget(createCenterViewButton(glWidget, tabWidget)); // 新增：自适应视图按钮
    layout->addWidget(createIterationMethodGroup(glWidget, tabWidget));
    layout->addWidget(createMinimalSurfaceGroup(glWidget, tabWidget));
    
    // 添加网格操作和Loop细分组（只在模型标签页显示）
    layout->addWidget(createMeshOperationsGroup(glWidget, tabWidget));
    layout->addWidget(createLoopSubdivisionGroup(glWidget, tabWidget));
    
    layout->addStretch();
    return panel;
}