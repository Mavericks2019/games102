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
    QByteArrayData data[19];
    char stringdata0[263];
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
QT_MOC_LITERAL(10, 116, 15), // "toggleShowFaces"
QT_MOC_LITERAL(11, 132, 4), // "show"
QT_MOC_LITERAL(12, 137, 22), // "updateAmbientIntensity"
QT_MOC_LITERAL(13, 160, 5), // "value"
QT_MOC_LITERAL(14, 166, 22), // "updateDiffuseIntensity"
QT_MOC_LITERAL(15, 189, 23), // "updateSpecularIntensity"
QT_MOC_LITERAL(16, 213, 15), // "updateShininess"
QT_MOC_LITERAL(17, 229, 11), // "setDrawMode"
QT_MOC_LITERAL(18, 241, 21) // "changeBackgroundColor"

    },
    "MainWindow\0updatePointInfo\0\0point\0"
    "clearPointInfo\0showDeleteMessage\0"
    "updateCanvasView\0index\0loadObjModel\0"
    "resetObjView\0toggleShowFaces\0show\0"
    "updateAmbientIntensity\0value\0"
    "updateDiffuseIntensity\0updateSpecularIntensity\0"
    "updateShininess\0setDrawMode\0"
    "changeBackgroundColor"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   79,    2, 0x0a /* Public */,
       4,    0,   82,    2, 0x0a /* Public */,
       5,    0,   83,    2, 0x0a /* Public */,
       6,    1,   84,    2, 0x0a /* Public */,
       8,    0,   87,    2, 0x0a /* Public */,
       9,    0,   88,    2, 0x0a /* Public */,
      10,    1,   89,    2, 0x0a /* Public */,
      12,    1,   92,    2, 0x0a /* Public */,
      14,    1,   95,    2, 0x0a /* Public */,
      15,    1,   98,    2, 0x0a /* Public */,
      16,    1,  101,    2, 0x0a /* Public */,
      17,    1,  104,    2, 0x0a /* Public */,
      18,    0,  107,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::QPointF,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,

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
        case 6: _t->toggleShowFaces((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->updateAmbientIntensity((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->updateDiffuseIntensity((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->updateSpecularIntensity((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->updateShininess((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->setDrawMode((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->changeBackgroundColor(); break;
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
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
