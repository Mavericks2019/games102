/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[27];
    char stringdata0[375];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 15), // "updatePointInfo"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 5), // "point"
QT_MOC_LITERAL(4, 34, 14), // "clearPointInfo"
QT_MOC_LITERAL(5, 49, 17), // "showDeleteMessage"
QT_MOC_LITERAL(6, 67, 16), // "updateCanvasView"
QT_MOC_LITERAL(7, 84, 5), // "index"
QT_MOC_LITERAL(8, 90, 12), // "loadObjModel"
QT_MOC_LITERAL(9, 103, 12), // "resetObjView"
QT_MOC_LITERAL(10, 116, 13), // "updateObjInfo"
QT_MOC_LITERAL(11, 130, 8), // "vertices"
QT_MOC_LITERAL(12, 139, 5), // "faces"
QT_MOC_LITERAL(13, 145, 5), // "edges"
QT_MOC_LITERAL(14, 151, 17), // "updateDegreeValue"
QT_MOC_LITERAL(15, 169, 5), // "value"
QT_MOC_LITERAL(16, 175, 16), // "updateSigmaValue"
QT_MOC_LITERAL(17, 192, 17), // "updateLambdaValue"
QT_MOC_LITERAL(18, 210, 16), // "updateAlphaValue"
QT_MOC_LITERAL(19, 227, 29), // "parameterizationMethodChanged"
QT_MOC_LITERAL(20, 257, 2), // "id"
QT_MOC_LITERAL(21, 260, 21), // "toggleCurveVisibility"
QT_MOC_LITERAL(22, 282, 7), // "visible"
QT_MOC_LITERAL(23, 290, 29), // "toggleControlPointsVisibility"
QT_MOC_LITERAL(24, 320, 30), // "toggleControlPolygonVisibility"
QT_MOC_LITERAL(25, 351, 16), // "setBSplineDegree"
QT_MOC_LITERAL(26, 368, 6) // "degree"

    },
    "MainWindow\0updatePointInfo\0\0point\0"
    "clearPointInfo\0showDeleteMessage\0"
    "updateCanvasView\0index\0loadObjModel\0"
    "resetObjView\0updateObjInfo\0vertices\0"
    "faces\0edges\0updateDegreeValue\0value\0"
    "updateSigmaValue\0updateLambdaValue\0"
    "updateAlphaValue\0parameterizationMethodChanged\0"
    "id\0toggleCurveVisibility\0visible\0"
    "toggleControlPointsVisibility\0"
    "toggleControlPolygonVisibility\0"
    "setBSplineDegree\0degree"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   94,    2, 0x08 /* Private */,
       4,    0,   97,    2, 0x08 /* Private */,
       5,    0,   98,    2, 0x08 /* Private */,
       6,    1,   99,    2, 0x08 /* Private */,
       8,    0,  102,    2, 0x08 /* Private */,
       9,    0,  103,    2, 0x08 /* Private */,
      10,    3,  104,    2, 0x08 /* Private */,
      14,    1,  111,    2, 0x08 /* Private */,
      16,    1,  114,    2, 0x08 /* Private */,
      17,    1,  117,    2, 0x08 /* Private */,
      18,    1,  120,    2, 0x08 /* Private */,
      19,    1,  123,    2, 0x08 /* Private */,
      21,    1,  126,    2, 0x08 /* Private */,
      23,    1,  129,    2, 0x08 /* Private */,
      24,    1,  132,    2, 0x08 /* Private */,
      25,    1,  135,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QPointF,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int,   11,   12,   13,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void, QMetaType::Int,   20,
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void, QMetaType::Int,   26,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->updatePointInfo((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 1: _t->clearPointInfo(); break;
        case 2: _t->showDeleteMessage(); break;
        case 3: _t->updateCanvasView((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->loadObjModel(); break;
        case 5: _t->resetObjView(); break;
        case 6: _t->updateObjInfo((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 7: _t->updateDegreeValue((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->updateSigmaValue((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->updateLambdaValue((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->updateAlphaValue((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->parameterizationMethodChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->toggleCurveVisibility((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->toggleControlPointsVisibility((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 14: _t->toggleControlPolygonVisibility((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 15: _t->setBSplineDegree((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    &QMainWindow::staticMetaObject,
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
