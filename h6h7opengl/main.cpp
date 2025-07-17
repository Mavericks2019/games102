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
        
        solidRadio->setChecked(true);
        
        layout->addWidget(solidRadio);
        layout->addWidget(gaussianRadio);
        layout->addWidget(meanRadio);
        layout->addWidget(maxRadio);
        
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

    // 创建显示选项组（双视图）- 新增函数
    QGroupBox* createDisplayOptionsGroupForDualView(GLWidget* leftView, GLWidget* rightView) {
        QGroupBox *group = new QGroupBox("Display Options");
        QVBoxLayout *layout = new QVBoxLayout(group);
        
        QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
        wireframeCheckbox->setStyleSheet("color: white;");
        QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [leftView, rightView](int state) {
            leftView->setShowWireframeOverlay(state == Qt::Checked);
            rightView->setShowWireframeOverlay(state == Qt::Checked);
        });
        
        QCheckBox *faceCheckbox = new QCheckBox("Hide Faces");
        faceCheckbox->setStyleSheet("color: white;");
        QObject::connect(faceCheckbox, &QCheckBox::stateChanged, [leftView, rightView](int state) {
            leftView->setHideFaces(state == Qt::Checked);
            rightView->setHideFaces(state == Qt::Checked);
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

    QGroupBox* createRenderingModeGroupForDualView(GLWidget* leftView, GLWidget* rightView) {
        QGroupBox *group = new QGroupBox("Rendering Mode");
        QVBoxLayout *layout = new QVBoxLayout(group);
        
        QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
        QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
        QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
        QRadioButton *maxRadio = new QRadioButton("Max Curvature");
        QRadioButton *textureRadio = new QRadioButton("Texture Mapping"); // 新增纹理映射选项
        
        solidRadio->setChecked(true);
        
        layout->addWidget(solidRadio);
        layout->addWidget(gaussianRadio);
        layout->addWidget(meanRadio);
        layout->addWidget(maxRadio);
        layout->addWidget(textureRadio); // 添加纹理选项
        
        // 连接信号：同时设置左右视图
        auto connectMode = [leftView, rightView](QRadioButton* radio, GLWidget::RenderMode mode) {
            QObject::connect(radio, &QRadioButton::clicked, [leftView, rightView, mode]() {
                leftView->setRenderMode(mode);
                rightView->setRenderMode(mode);
            });
        };
        
        connectMode(solidRadio, GLWidget::BlinnPhong);
        connectMode(gaussianRadio, GLWidget::GaussianCurvature);
        connectMode(meanRadio, GLWidget::MeanCurvature);
        connectMode(maxRadio, GLWidget::MaxCurvature);
        connectMode(textureRadio, GLWidget::TextureMapping); // 纹理映射模式
        
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

    // 创建参数化视图选项卡
    QWidget* createParameterizationTab(GLWidget* glWidget) {
        QWidget *tab = new QWidget;
        QHBoxLayout *layout = new QHBoxLayout(tab);
        
        // 使用分割器创建左右视图
        QSplitter *splitter = new QSplitter(Qt::Horizontal);
        
        // 创建专用视图
        GLWidget *leftView = new GLWidget;
        GLWidget *rightView = new GLWidget;
        
        // 设置右侧视图为参数化视图（禁止旋转）
        rightView->isParameterizationView = false;// 新增
        
        // 添加视图
        splitter->addWidget(leftView);
        splitter->addWidget(rightView);
        splitter->setSizes(QList<int>() << 500 << 500);
        
        layout->addWidget(splitter);
        
        // 保存视图指针
        tab->setProperty("leftGLWidget", QVariant::fromValue(leftView));
        tab->setProperty("rightGLWidget", QVariant::fromValue(rightView));
        
        return tab;
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
        
        layout->addStretch();
        return panel;
    }

    // 创建参数化控制面板
    QWidget* createParameterizationControlPanel(GLWidget* glWidget, QWidget* paramTab, QTabWidget* tabWidget) {
        QWidget *panel = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(panel);
        
        // 获取视图
        GLWidget *leftView = paramTab->property("leftGLWidget").value<GLWidget*>();
        GLWidget *rightView = paramTab->property("rightGLWidget").value<GLWidget*>();
        
        // 添加加载按钮
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
        QObject::connect(loadButton, &QPushButton::clicked, [rightView, leftView]() {
            QString filePath = QFileDialog::getOpenFileName(
                nullptr, "Open OBJ File", "", "OBJ Files (*.obj)");
            
            if (!filePath.isEmpty()) {
                rightView->loadOBJ(filePath);
                leftView->loadOBJ(filePath);
            }
        });
        layout->addWidget(loadButton);
        
        // 添加渲染模式组
        QGroupBox *renderModeGroup = new QGroupBox("Rendering Mode");
        QVBoxLayout *renderLayout = new QVBoxLayout(renderModeGroup);
        
        QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
        QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
        QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
        QRadioButton *maxRadio = new QRadioButton("Max Curvature");
        
        solidRadio->setChecked(true);
        
        renderLayout->addWidget(solidRadio);
        renderLayout->addWidget(gaussianRadio);
        renderLayout->addWidget(meanRadio);
        renderLayout->addWidget(maxRadio);
        
        // 连接信号：同时设置左右视图
        auto connectMode = [leftView, rightView](QRadioButton* radio, GLWidget::RenderMode mode) {
            QObject::connect(radio, &QRadioButton::clicked, [leftView, rightView, mode]() {
                leftView->setRenderMode(mode);
                rightView->setRenderMode(mode);
            });
        };
        
        connectMode(solidRadio, GLWidget::BlinnPhong);
        connectMode(gaussianRadio, GLWidget::GaussianCurvature);
        connectMode(meanRadio, GLWidget::MeanCurvature);
        connectMode(maxRadio, GLWidget::MaxCurvature);
        
        layout->addWidget(renderModeGroup);
        
        // 添加显示选项组（双视图）- 新增
        layout->addWidget(createDisplayOptionsGroupForDualView(leftView, rightView));
        
        // 添加边界选项
        QGroupBox *boundaryGroup = new QGroupBox("Boundary Type");
        boundaryGroup->setStyleSheet("QGroupBox { color: white; }");
        QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryGroup);
        
        QRadioButton *rectRadio = new QRadioButton("Rectangle");
        QRadioButton *circleRadio = new QRadioButton("Circle");
        rectRadio->setChecked(true);

        // 连接边界选项信号
        QObject::connect(rectRadio, &QRadioButton::clicked, [rightView]() {
            rightView->setBoundaryType(GLWidget::Rectangle);
        });
        QObject::connect(circleRadio, &QRadioButton::clicked, [rightView]() {
            rightView->setBoundaryType(GLWidget::Circle);
        });
        // 添加渲染模式组（使用新的双视图渲染模式组）
        layout->addWidget(createRenderingModeGroupForDualView(leftView, rightView));
        boundaryLayout->addWidget(rectRadio);
        boundaryLayout->addWidget(circleRadio);
        layout->addWidget(boundaryGroup);
        
        // 添加参数化按钮
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

        // 连接参数化按钮信号
        QObject::connect(paramButton, &QPushButton::clicked, [glWidget, paramTab]() {
            // 获取左视图并执行参数化
            GLWidget* leftView = paramTab->property("leftGLWidget").value<GLWidget*>();
            // leftView->performParameterization();
            
            // 在右视图显示参数化结果
            GLWidget* rightView = paramTab->property("rightGLWidget").value<GLWidget*>();
            //rightView->openMesh = leftView->openMesh; // 复制网格
            rightView->performParameterization();
        });
        
        layout->addWidget(paramButton);
        
        // 添加参数化选项
        QGroupBox *optionsGroup = new QGroupBox("Parameterization Options");
        optionsGroup->setStyleSheet("QGroupBox { color: white; }");
        QFormLayout *optionsLayout = new QFormLayout(optionsGroup);
        
        QDoubleSpinBox *toleranceSpinBox = new QDoubleSpinBox;
        toleranceSpinBox->setRange(0.0001, 0.1);
        toleranceSpinBox->setValue(0.001);
        toleranceSpinBox->setDecimals(5);
        
        QSpinBox *maxIterSpinBox = new QSpinBox;
        maxIterSpinBox->setRange(10, 10000);
        maxIterSpinBox->setValue(500);
        
        optionsLayout->addRow("Convergence Tolerance:", toleranceSpinBox);
        optionsLayout->addRow("Max Iterations:", maxIterSpinBox);
        layout->addWidget(optionsGroup);
        
        layout->addStretch();
        return panel;
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
    QWidget *modelTab = new QWidget;
    QHBoxLayout *modelTabLayout = new QHBoxLayout(modelTab);
    modelTabLayout->addWidget(glWidget);
    tabWidget->addTab(modelTab, "OBJ Model");
    
    // 创建参数化标签页
    QWidget *paramTab = UIUtils::createParameterizationTab(glWidget);
    tabWidget->addTab(paramTab, "Parameterization");
    
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
    stackedLayout->addWidget(UIUtils::createModelControlPanel(
        glWidget, modelInfoLabel, &mainWindow, tabWidget
    ));
    
    // 创建参数化控制面板
    stackedLayout->addWidget(UIUtils::createParameterizationControlPanel(
        glWidget, paramTab, tabWidget
    ));
    
    // ==== 网格操作组 ====
    controlLayout->addWidget(UIUtils::createMeshOperationsGroup(glWidget, tabWidget));
    controlLayout->addWidget(UIUtils::createLoopSubdivisionGroup(glWidget, tabWidget));
    
    // 连接标签切换信号
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [stackedLayout](int index) {
        stackedLayout->setCurrentIndex(index == 1 ? 1 : 0);
    });
    
    // 添加控制面板到主布局
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}