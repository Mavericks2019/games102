#include "curve_designer.h"
#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Curve Design Tool");
    mainWindow.setMinimumSize(1100, 750);
    mainWindow.setStyleSheet("background-color: #2c3e50;");
    
    CurveDesigner *designer = new CurveDesigner();
    mainWindow.setCentralWidget(designer);
    
    mainWindow.show();
    return app.exec();
}