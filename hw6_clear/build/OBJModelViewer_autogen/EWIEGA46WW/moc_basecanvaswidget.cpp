/****************************************************************************
** Meta object code from reading C++ file 'basecanvaswidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../basecanvaswidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'basecanvaswidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BaseCanvasWidget_t {
    QByteArrayData data[6];
    char stringdata0[65];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BaseCanvasWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BaseCanvasWidget_t qt_meta_stringdata_BaseCanvasWidget = {
    {
QT_MOC_LITERAL(0, 0, 16), // "BaseCanvasWidget"
QT_MOC_LITERAL(1, 17, 12), // "pointHovered"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 5), // "point"
QT_MOC_LITERAL(4, 37, 14), // "noPointHovered"
QT_MOC_LITERAL(5, 52, 12) // "pointDeleted"

    },
    "BaseCanvasWidget\0pointHovered\0\0point\0"
    "noPointHovered\0pointDeleted"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BaseCanvasWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,
       4,    0,   32,    2, 0x06 /* Public */,
       5,    0,   33,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QPointF,    3,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void BaseCanvasWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BaseCanvasWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->pointHovered((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 1: _t->noPointHovered(); break;
        case 2: _t->pointDeleted(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BaseCanvasWidget::*)(const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseCanvasWidget::pointHovered)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BaseCanvasWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseCanvasWidget::noPointHovered)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BaseCanvasWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseCanvasWidget::pointDeleted)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BaseCanvasWidget::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_BaseCanvasWidget.data,
    qt_meta_data_BaseCanvasWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BaseCanvasWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BaseCanvasWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BaseCanvasWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int BaseCanvasWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void BaseCanvasWidget::pointHovered(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void BaseCanvasWidget::noPointHovered()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BaseCanvasWidget::pointDeleted()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
