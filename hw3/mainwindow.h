#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>


class CanvasWidget;
class QCheckBox;
class QLabel;
class QSlider;
class QRadioButton;
class QButtonGroup;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    
private slots:
    void updateLegend();
    void updatePointInfo(const QPointF& point);
    void clearPointInfo();
    
    void updateDegreeValue(int value);
    void updateSigmaValue(int value);
    void updateLambdaValue(int value);
    
    void showDeleteMessage();
    void parameterizationMethodChanged(int id);
    
private:
    QHBoxLayout* createSliderLayout(QSlider* slider, QLabel* valueLabel);
    
    CanvasWidget *canvas;
    
    // Left panel controls
    QButtonGroup *paramGroup;
    QRadioButton *uniformParam;
    QRadioButton *chordalParam;
    QRadioButton *centripetalParam;
    QRadioButton *foleyParam;
    
    // Right panel controls
    QCheckBox *polyInterpCheck;
    QCheckBox *gaussInterpCheck;
    QCheckBox *leastSquaresCheck;
    QCheckBox *ridgeRegCheck;
    
    QLabel *legendLabel;
    QLabel *pointInfoLabel;
    QMap<QString, QColor> curveColors;
    
    QSlider *degreeSlider;
    QSlider *sigmaSlider;
    QSlider *lambdaSlider;
    
    QLabel *degreeValueLabel;
    QLabel *sigmaValueLabel;
    QLabel *lambdaValueLabel;
    
    QTimer *deleteMessageTimer;
};

#endif // MAINWINDOW_H