/****************************************************************************
** Meta object code from reading C++ file 'ControlDialog.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../ControlDialog.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ControlDialog.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.6.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_CoordinatorThread_t {
    QByteArrayData data[7];
    char stringdata0[87];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CoordinatorThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CoordinatorThread_t qt_meta_stringdata_CoordinatorThread = {
    {
QT_MOC_LITERAL(0, 0, 17), // "CoordinatorThread"
QT_MOC_LITERAL(1, 18, 11), // "ResultReady"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 14), // "AnalysisResult"
QT_MOC_LITERAL(4, 46, 7), // "oResult"
QT_MOC_LITERAL(5, 54, 19), // "CoordinatorFinished"
QT_MOC_LITERAL(6, 74, 12) // "NextFunction"

    },
    "CoordinatorThread\0ResultReady\0\0"
    "AnalysisResult\0oResult\0CoordinatorFinished\0"
    "NextFunction"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CoordinatorThread[] = {

 // content:
       7,       // revision
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
       5,    0,   32,    2, 0x06 /* Public */,
       6,    0,   33,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CoordinatorThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CoordinatorThread *_t = static_cast<CoordinatorThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->ResultReady((*reinterpret_cast< AnalysisResult(*)>(_a[1]))); break;
        case 1: _t->CoordinatorFinished(); break;
        case 2: _t->NextFunction(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (CoordinatorThread::*_t)(AnalysisResult );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CoordinatorThread::ResultReady)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (CoordinatorThread::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CoordinatorThread::CoordinatorFinished)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (CoordinatorThread::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CoordinatorThread::NextFunction)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject CoordinatorThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_CoordinatorThread.data,
      qt_meta_data_CoordinatorThread,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *CoordinatorThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CoordinatorThread::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_CoordinatorThread.stringdata0))
        return static_cast<void*>(const_cast< CoordinatorThread*>(this));
    return QThread::qt_metacast(_clname);
}

int CoordinatorThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
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
void CoordinatorThread::ResultReady(AnalysisResult _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CoordinatorThread::CoordinatorFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void CoordinatorThread::NextFunction()
{
    QMetaObject::activate(this, &staticMetaObject, 2, Q_NULLPTR);
}
struct qt_meta_stringdata_ControlDialog_t {
    QByteArrayData data[18];
    char stringdata0[238];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ControlDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ControlDialog_t qt_meta_stringdata_ControlDialog = {
    {
QT_MOC_LITERAL(0, 0, 13), // "ControlDialog"
QT_MOC_LITERAL(1, 14, 15), // "PerformAnalysis"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 16), // "std::list<ulong>"
QT_MOC_LITERAL(4, 48, 10), // "aFunctions"
QT_MOC_LITERAL(5, 59, 10), // "slideInIdx"
QT_MOC_LITERAL(6, 70, 3), // "idx"
QT_MOC_LITERAL(7, 74, 13), // "StartAnalysis"
QT_MOC_LITERAL(8, 88, 22), // "StartMicrocodeAnalysis"
QT_MOC_LITERAL(9, 111, 13), // "StartBatchRun"
QT_MOC_LITERAL(10, 125, 18), // "HandleResultNormal"
QT_MOC_LITERAL(11, 144, 14), // "AnalysisResult"
QT_MOC_LITERAL(12, 159, 7), // "oResult"
QT_MOC_LITERAL(13, 167, 17), // "HandleResultBatch"
QT_MOC_LITERAL(14, 185, 19), // "CoordinatorFinished"
QT_MOC_LITERAL(15, 205, 12), // "NextFunction"
QT_MOC_LITERAL(16, 218, 13), // "DisplayResult"
QT_MOC_LITERAL(17, 232, 5) // "index"

    },
    "ControlDialog\0PerformAnalysis\0\0"
    "std::list<ulong>\0aFunctions\0slideInIdx\0"
    "idx\0StartAnalysis\0StartMicrocodeAnalysis\0"
    "StartBatchRun\0HandleResultNormal\0"
    "AnalysisResult\0oResult\0HandleResultBatch\0"
    "CoordinatorFinished\0NextFunction\0"
    "DisplayResult\0index"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ControlDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x06 /* Public */,
       5,    1,   67,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   70,    2, 0x08 /* Private */,
       8,    0,   71,    2, 0x08 /* Private */,
       9,    0,   72,    2, 0x08 /* Private */,
      10,    1,   73,    2, 0x08 /* Private */,
      13,    1,   76,    2, 0x08 /* Private */,
      14,    0,   79,    2, 0x08 /* Private */,
      15,    0,   80,    2, 0x08 /* Private */,
      16,    1,   81,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Int,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QModelIndex,   17,

       0        // eod
};

void ControlDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ControlDialog *_t = static_cast<ControlDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->PerformAnalysis((*reinterpret_cast< const std::list<ulong>(*)>(_a[1]))); break;
        case 1: _t->slideInIdx((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->StartAnalysis(); break;
        case 3: _t->StartMicrocodeAnalysis(); break;
        case 4: _t->StartBatchRun(); break;
        case 5: _t->HandleResultNormal((*reinterpret_cast< AnalysisResult(*)>(_a[1]))); break;
        case 6: _t->HandleResultBatch((*reinterpret_cast< AnalysisResult(*)>(_a[1]))); break;
        case 7: _t->CoordinatorFinished(); break;
        case 8: _t->NextFunction(); break;
        case 9: _t->DisplayResult((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ControlDialog::*_t)(const std::list<unsigned long> & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ControlDialog::PerformAnalysis)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (ControlDialog::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ControlDialog::slideInIdx)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject ControlDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ControlDialog.data,
      qt_meta_data_ControlDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *ControlDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ControlDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_ControlDialog.stringdata0))
        return static_cast<void*>(const_cast< ControlDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int ControlDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void ControlDialog::PerformAnalysis(const std::list<unsigned long> & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ControlDialog::slideInIdx(int _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
