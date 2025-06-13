#include "curve_designer.h"
#include <QApplication>
#include <QMainWindow>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置Fusion样式 - 与插值工具一致
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 设置深色主题 - 与插值工具一致
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25,25,25));
    palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(palette);
    
    // 设置应用样式 - 与插值工具一致
    app.setStyleSheet(
        "QGroupBox {"
        "  border: 1px solid #3A3939;"
        "  border-radius: 5px;"
        "  margin-top: 1ex;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top center;"
        "  padding: 0 5px;"
        "  background-color: #3A3939;"
        "  color: white;"
        "  font-weight: bold;"
        "}"
        "QLabel {"
        "  color: white;"
        "}"
        "QSlider::groove:horizontal {"
        "  background: #505050;"
        "  height: 8px;"
        "  border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #42a4d8;"
        "  width: 16px;"
        "  margin: -4px 0;"
        "  border-radius: 8px;"
        "}"
        "QSlider::add-page:horizontal {"
        "  background: #505050;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #42a4d8;"
        "}"
        "QRadioButton, QCheckBox {"
        "  color: white;"
        "  font-size: 13px;"
        "}"
        "QPushButton {"
        "  background-color: #505050;"
        "  color: white;"
        "  border-radius: 5px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #606060;"
        "}"
    );
    
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Curve Design Tool");
    // 设置与插值工具相同的窗口大小
    mainWindow.resize(1400, 800);
    mainWindow.setMinimumSize(1400, 800);
    
    CurveDesigner *designer = new CurveDesigner();
    mainWindow.setCentralWidget(designer);
    
    mainWindow.show();
    return app.exec();
}