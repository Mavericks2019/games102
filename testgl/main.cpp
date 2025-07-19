#include <QApplication>
#include "renderwidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    RenderWidget renderer;
    renderer.resize(800, 600);
    renderer.setWindowTitle("Path Tracing Renderer");
    renderer.show();
    
    return app.exec();
}