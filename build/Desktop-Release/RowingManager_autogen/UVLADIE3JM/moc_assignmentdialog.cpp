/****************************************************************************
** Meta object code from reading C++ file 'assignmentdialog.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/assignmentdialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'assignmentdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN16AssignmentDialogE_t {};
} // unnamed namespace

template <> constexpr inline auto AssignmentDialog::qt_create_metaobjectdata<qt_meta_tag_ZN16AssignmentDialogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AssignmentDialog",
        "onGenerate",
        "",
        "onCheck",
        "onSelectAllBoats",
        "onSelectNoneBoats",
        "onSelectAllRowers",
        "onSelectNoneRowers",
        "onMovePriorityUp",
        "onMovePriorityDown",
        "onAutoSelectBoatsToggled",
        "checked",
        "onAddGroup",
        "onRemoveGroup",
        "onGroupSelectionChanged",
        "onAddRowerToGroup",
        "onRemoveRowerFromGroup",
        "onGroupBoatChanged",
        "comboIndex",
        "refreshSelectionTabStates",
        "onSelectionChanged",
        "onAddSteeringOnly",
        "onRemoveSteeringOnly"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onGenerate'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCheck'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSelectAllBoats'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSelectNoneBoats'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSelectAllRowers'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSelectNoneRowers'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMovePriorityUp'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMovePriorityDown'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAutoSelectBoatsToggled'
        QtMocHelpers::SlotData<void(bool)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 11 },
        }}),
        // Slot 'onAddGroup'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveGroup'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGroupSelectionChanged'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAddRowerToGroup'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveRowerFromGroup'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGroupBoatChanged'
        QtMocHelpers::SlotData<void(int)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Slot 'refreshSelectionTabStates'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSelectionChanged'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAddSteeringOnly'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveSteeringOnly'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AssignmentDialog, qt_meta_tag_ZN16AssignmentDialogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AssignmentDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16AssignmentDialogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16AssignmentDialogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16AssignmentDialogE_t>.metaTypes,
    nullptr
} };

void AssignmentDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AssignmentDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onGenerate(); break;
        case 1: _t->onCheck(); break;
        case 2: _t->onSelectAllBoats(); break;
        case 3: _t->onSelectNoneBoats(); break;
        case 4: _t->onSelectAllRowers(); break;
        case 5: _t->onSelectNoneRowers(); break;
        case 6: _t->onMovePriorityUp(); break;
        case 7: _t->onMovePriorityDown(); break;
        case 8: _t->onAutoSelectBoatsToggled((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 9: _t->onAddGroup(); break;
        case 10: _t->onRemoveGroup(); break;
        case 11: _t->onGroupSelectionChanged(); break;
        case 12: _t->onAddRowerToGroup(); break;
        case 13: _t->onRemoveRowerFromGroup(); break;
        case 14: _t->onGroupBoatChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->refreshSelectionTabStates(); break;
        case 16: _t->onSelectionChanged(); break;
        case 17: _t->onAddSteeringOnly(); break;
        case 18: _t->onRemoveSteeringOnly(); break;
        default: ;
        }
    }
}

const QMetaObject *AssignmentDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AssignmentDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16AssignmentDialogE_t>.strings))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int AssignmentDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}
QT_WARNING_POP
