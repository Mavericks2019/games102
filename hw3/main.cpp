#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置Fusion风格
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 设置调色板
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
    
    // 设置应用样式
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
        "  background:rgb(76, 145, 182);"
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
    );
    
    MainWindow mainWin;
    mainWin.show();
    
    return app.exec();
}