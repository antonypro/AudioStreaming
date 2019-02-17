/****************************************************************************
** Meta object code from reading C++ file 'spectrumanalyzer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "spectrumanalyzer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QVector>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'spectrumanalyzer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SpectrumAnalyzer_t {
    QByteArrayData data[9];
    char stringdata0[107];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SpectrumAnalyzer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SpectrumAnalyzer_t qt_meta_stringdata_SpectrumAnalyzer = {
    {
QT_MOC_LITERAL(0, 0, 16), // "SpectrumAnalyzer"
QT_MOC_LITERAL(1, 17, 15), // "spectrumChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 23), // "QVector<SpectrumStruct>"
QT_MOC_LITERAL(4, 58, 5), // "start"
QT_MOC_LITERAL(5, 64, 12), // "QAudioFormat"
QT_MOC_LITERAL(6, 77, 6), // "format"
QT_MOC_LITERAL(7, 84, 17), // "calculateSpectrum"
QT_MOC_LITERAL(8, 102, 4) // "data"

    },
    "SpectrumAnalyzer\0spectrumChanged\0\0"
    "QVector<SpectrumStruct>\0start\0"
    "QAudioFormat\0format\0calculateSpectrum\0"
    "data"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SpectrumAnalyzer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   32,    2, 0x0a /* Public */,
       7,    1,   35,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    2,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, QMetaType::QByteArray,    8,

       0        // eod
};

void SpectrumAnalyzer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SpectrumAnalyzer *_t = static_cast<SpectrumAnalyzer *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->spectrumChanged((*reinterpret_cast< QVector<SpectrumStruct>(*)>(_a[1]))); break;
        case 1: _t->start((*reinterpret_cast< const QAudioFormat(*)>(_a[1]))); break;
        case 2: _t->calculateSpectrum((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAudioFormat >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SpectrumAnalyzer::*)(QVector<SpectrumStruct> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SpectrumAnalyzer::spectrumChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SpectrumAnalyzer::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_SpectrumAnalyzer.data,
    qt_meta_data_SpectrumAnalyzer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SpectrumAnalyzer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SpectrumAnalyzer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SpectrumAnalyzer.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SpectrumAnalyzer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void SpectrumAnalyzer::spectrumChanged(QVector<SpectrumStruct> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
