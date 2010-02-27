/****************************************************************************
** Meta object code from reading C++ file 'ltr_gui.h'
**
** Created: Sun Feb 21 14:06:42 2010
**      by: The Qt Meta Object Compiler version 61 (Qt 4.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "ltr_gui.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ltr_gui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 61
#error "This file was generated using the moc from 4.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_LtrGuiForm[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   12, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x08,
      37,   11,   11,   11, 0x08,
      62,   11,   11,   11, 0x08,
      86,   11,   11,   11, 0x08,
     110,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_LtrGuiForm[] = {
    "LtrGuiForm\0\0on_startButton_pressed()\0"
    "on_pauseButton_pressed()\0"
    "on_wakeButton_pressed()\0on_stopButton_pressed()\0"
    "update()\0"
};

const QMetaObject LtrGuiForm::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_LtrGuiForm,
      qt_meta_data_LtrGuiForm, 0 }
};

const QMetaObject *LtrGuiForm::metaObject() const
{
    return &staticMetaObject;
}

void *LtrGuiForm::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_LtrGuiForm))
        return static_cast<void*>(const_cast< LtrGuiForm*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int LtrGuiForm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: on_startButton_pressed(); break;
        case 1: on_pauseButton_pressed(); break;
        case 2: on_wakeButton_pressed(); break;
        case 3: on_stopButton_pressed(); break;
        case 4: update(); break;
        default: ;
        }
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
