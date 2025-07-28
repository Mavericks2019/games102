#ifndef CVT_WEIGHT_TAB_H
#define CVT_WEIGHT_TAB_H

#include "../cvtimagewidget/cvt_imageglwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QComboBox>

// 创建CVT Weight选项卡
QWidget* createCVTWeightTab(CVTImageGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    // 创建专用视图
    CVTImageGLWidget *cvtImageView = new CVTImageGLWidget;
    layout->addWidget(cvtImageView);
    
    // 保存视图指针
    tab->setProperty("cvtImageGLWidget", QVariant::fromValue(cvtImageView));
    
    return tab;
}

// 创建CVT Weight控制面板
QWidget* createCVTWeightControlPanel(CVTImageGLWidget* glWidget, QWidget* cvtWeightTab) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    
    // ==== 图像信息组 ====
    QGroupBox *imageInfoGroup = new QGroupBox("Image Information");
    QVBoxLayout *imageLayout = new QVBoxLayout(imageInfoGroup);
    
    QLabel *infoLabel = new QLabel("No image loaded");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setFixedHeight(50);
    infoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    infoLabel->setWordWrap(true);
    
    imageLayout->addWidget(infoLabel);
    layout->addWidget(imageInfoGroup);
    
    // ==== 图像加载组 ====
    QGroupBox *imageLoadGroup = new QGroupBox("Image Settings");
    QVBoxLayout *imageLoadLayout = new QVBoxLayout(imageLoadGroup);
    
    // 图像加载按钮
    QPushButton *loadImageButton = new QPushButton("Load Image");
    loadImageButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "   margin-bottom: 10px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    
    // 添加显示图像复选框
    QCheckBox *showImageCheckbox = new QCheckBox("Show Image");
    showImageCheckbox->setStyleSheet("color: white;");
    showImageCheckbox->setChecked(true);
    QObject::connect(showImageCheckbox, &QCheckBox::stateChanged, [cvtWeightTab](int state) {
        CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
        if (cvtImageView) {
            cvtImageView->setShowImage(state == Qt::Checked);
        }
    });
    
    QObject::connect(loadImageButton, &QPushButton::clicked, [cvtWeightTab, infoLabel, showImageCheckbox]() {
        // 打开文件对话框选择图像
        QString fileName = QFileDialog::getOpenFileName(
            nullptr, 
            "Open Image", 
            "", 
            "Image Files (*.png *.jpg *.jpeg *.bmp)"
        );
        
        if (!fileName.isEmpty()) {
            // 更新信息标签
            QImage image(fileName);
            if (!image.isNull()) {
                infoLabel->setText(QString("Image: %1\nSize: %2x%3")
                                  .arg(QFileInfo(fileName).fileName())
                                  .arg(image.width())
                                  .arg(image.height()));
                
                // 加载图像到CVT视图
                CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
                if (cvtImageView) {
                    cvtImageView->loadImage(image);
                    // 确保图像显示复选框被选中
                    showImageCheckbox->setChecked(true);
                }
            } else {
                infoLabel->setText("Failed to load image");
            }
        }
    });
    imageLoadLayout->addWidget(loadImageButton);
    
    // 添加显示图像复选框到布局
    imageLoadLayout->addWidget(showImageCheckbox);
    
    // 权重类型选择
    QHBoxLayout *weightTypeLayout = new QHBoxLayout();
    QLabel *weightLabel = new QLabel("Weight Type:");
    weightLabel->setStyleSheet("color: white;");
    QComboBox *weightComboBox = new QComboBox();
    weightComboBox->addItem("Uniform");
    weightComboBox->addItem("Gradient");
    weightComboBox->addItem("Texture");
    weightComboBox->setStyleSheet(
        "QComboBox {"
        "   background-color: #3A3A3A;"
        "   color: white;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "}"
    );
    weightTypeLayout->addWidget(weightLabel);
    weightTypeLayout->addWidget(weightComboBox);
    
    imageLoadLayout->addLayout(weightTypeLayout);
    layout->addWidget(imageLoadGroup);
    
    // ==== CVT控制组 ====
    QGroupBox *cvtGroup = new QGroupBox("CVT Weight Settings");
    QVBoxLayout *cvtLayout = new QVBoxLayout(cvtGroup);
    
    // 点数控制
    QHBoxLayout *pointCountLayout = new QHBoxLayout();
    QLabel *countLabel = new QLabel("Points:");
    countLabel->setStyleSheet("color: white;");
    QLineEdit *countInput = new QLineEdit("100");
    countInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #3A3A3A;"
        "   color: white;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "}"
    );
    pointCountLayout->addWidget(countLabel);
    pointCountLayout->addWidget(countInput);
    
    // 生成点按钮
    QPushButton *generateButton = new QPushButton("Generate Points");
    generateButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "   margin-top: 10px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(generateButton, &QPushButton::clicked, [cvtWeightTab, countInput]() {
        CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
        if (cvtImageView) {
            bool ok;
            int count = countInput->text().toInt(&ok);
            if (ok && count > 0) {
                cvtImageView->generateRandomPoints(count);
            }
        }
    });
    
    cvtLayout->addLayout(pointCountLayout);
    cvtLayout->addWidget(generateButton);
    
    // 迭代控制
    QHBoxLayout *iterLayout = new QHBoxLayout();
    QLabel *iterLabel = new QLabel("Iterations:");
    iterLabel->setStyleSheet("color: white;");
    QLineEdit *iterInput = new QLineEdit("1");
    iterInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #3A3A3A;"
        "   color: white;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "}"
    );
    iterLayout->addWidget(iterLabel);
    iterLayout->addWidget(iterInput);
    
    // Lloyd松弛按钮
    QPushButton *lloydButton = new QPushButton("Lloyd Relaxation");
    lloydButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "   margin-top: 10px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(lloydButton, &QPushButton::clicked, [cvtWeightTab, iterInput]() {
        CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
        if (cvtImageView) {
            bool ok;
            int iterations = iterInput->text().toInt(&ok);
            if (ok && iterations > 0) {
                for (int i = 0; i < iterations; i++) {
                    cvtImageView->performLloydRelaxation();
                }
            }
        }
    });
    
    cvtLayout->addLayout(iterLayout);
    cvtLayout->addWidget(lloydButton);
    
    // 显示控制
    QCheckBox *showPointsCheckbox = new QCheckBox("Show Points");
    showPointsCheckbox->setStyleSheet("color: white;");
    showPointsCheckbox->setChecked(true);
    QObject::connect(showPointsCheckbox, &QCheckBox::stateChanged, [cvtWeightTab](int state) {
        CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
        if (cvtImageView) {
            cvtImageView->setShowPoints(state == Qt::Checked);
        }
    });
    
    QCheckBox *showVoronoiCheckbox = new QCheckBox("Show Voronoi");
    showVoronoiCheckbox->setStyleSheet("color: white;");
    QObject::connect(showVoronoiCheckbox, &QCheckBox::stateChanged, [cvtWeightTab](int state) {
        CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
        if (cvtImageView) {
            cvtImageView->setShowVoronoiDiagram(state == Qt::Checked);
        }
    });
    
    QCheckBox *showDelaunayCheckbox = new QCheckBox("Show Delaunay");
    showDelaunayCheckbox->setStyleSheet("color: white;");
    QObject::connect(showDelaunayCheckbox, &QCheckBox::stateChanged, [cvtWeightTab](int state) {
        CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
        if (cvtImageView) {
            cvtImageView->setShowDelaunay(state == Qt::Checked);
        }
    });
    
    cvtLayout->addWidget(showPointsCheckbox);
    cvtLayout->addWidget(showVoronoiCheckbox);
    cvtLayout->addWidget(showDelaunayCheckbox);
    
    // 重置视图按钮
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "   margin-top: 10px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(resetButton, &QPushButton::clicked, [cvtWeightTab]() {
        CVTImageGLWidget *cvtImageView = cvtWeightTab->property("cvtImageGLWidget").value<CVTImageGLWidget*>();
        if (cvtImageView) {
            cvtImageView->resetView();
        }
    });
    cvtLayout->addWidget(resetButton);
    
    layout->addWidget(cvtGroup);
    layout->addStretch();
    
    return panel;
}

#endif // CVT_WEIGHT_TAB_H