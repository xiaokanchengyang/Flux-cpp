/****************************************************************************
** Meta object code from reading C++ file 'browse_page.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../flux-gui/src/views/browse_page.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'browse_page.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.0. It"
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
struct qt_meta_tag_ZN10BrowsePageE_t {};
} // unnamed namespace

template <> constexpr inline auto BrowsePage::qt_create_metaobjectdata<qt_meta_tag_ZN10BrowsePageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "BrowsePage",
        "extractRequested",
        "",
        "archivePath",
        "filePaths",
        "outputDir",
        "addFilesRequested",
        "deleteFilesRequested",
        "previewRequested",
        "filePath",
        "setArchiveEntries",
        "QList<ArchiveEntry>",
        "entries",
        "setPreviewContent",
        "content",
        "mimeType",
        "showError",
        "message",
        "onTreeSelectionChanged",
        "onTreeDoubleClicked",
        "QModelIndex",
        "index",
        "onTreeContextMenu",
        "QPoint",
        "pos",
        "onSearchTextChanged",
        "text",
        "onExtractSelected",
        "onAddFiles",
        "onDeleteSelected",
        "onRefresh",
        "onExpandAll",
        "onCollapseAll",
        "onCopyPath",
        "onProperties",
        "updateStatistics",
        "onPreviewTimer"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'extractRequested'
        QtMocHelpers::SignalData<void(const QString &, const QStringList &, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QStringList, 4 }, { QMetaType::QString, 5 },
        }}),
        // Signal 'addFilesRequested'
        QtMocHelpers::SignalData<void(const QString &, const QStringList &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QStringList, 4 },
        }}),
        // Signal 'deleteFilesRequested'
        QtMocHelpers::SignalData<void(const QString &, const QStringList &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QStringList, 4 },
        }}),
        // Signal 'previewRequested'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 9 },
        }}),
        // Slot 'setArchiveEntries'
        QtMocHelpers::SlotData<void(const QList<struct ArchiveEntry> &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Slot 'setPreviewContent'
        QtMocHelpers::SlotData<void(const QString &, const QByteArray &, const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 }, { QMetaType::QByteArray, 14 }, { QMetaType::QString, 15 },
        }}),
        // Slot 'showError'
        QtMocHelpers::SlotData<void(const QString &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Slot 'onTreeSelectionChanged'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTreeDoubleClicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 20, 21 },
        }}),
        // Slot 'onTreeContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 23, 24 },
        }}),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 26 },
        }}),
        // Slot 'onExtractSelected'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAddFiles'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDeleteSelected'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRefresh'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExpandAll'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCollapseAll'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCopyPath'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProperties'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateStatistics'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPreviewTimer'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<BrowsePage, qt_meta_tag_ZN10BrowsePageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject BrowsePage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10BrowsePageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10BrowsePageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10BrowsePageE_t>.metaTypes,
    nullptr
} };

void BrowsePage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BrowsePage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->extractRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 1: _t->addFilesRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 2: _t->deleteFilesRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 3: _t->previewRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->setArchiveEntries((*reinterpret_cast<std::add_pointer_t<QList<ArchiveEntry>>>(_a[1]))); break;
        case 5: _t->setPreviewContent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 6: _t->showError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->onTreeSelectionChanged(); break;
        case 8: _t->onTreeDoubleClicked((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 9: _t->onTreeContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 10: _t->onSearchTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->onExtractSelected(); break;
        case 12: _t->onAddFiles(); break;
        case 13: _t->onDeleteSelected(); break;
        case 14: _t->onRefresh(); break;
        case 15: _t->onExpandAll(); break;
        case 16: _t->onCollapseAll(); break;
        case 17: _t->onCopyPath(); break;
        case 18: _t->onProperties(); break;
        case 19: _t->updateStatistics(); break;
        case 20: _t->onPreviewTimer(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (BrowsePage::*)(const QString & , const QStringList & , const QString & )>(_a, &BrowsePage::extractRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (BrowsePage::*)(const QString & , const QStringList & )>(_a, &BrowsePage::addFilesRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (BrowsePage::*)(const QString & , const QStringList & )>(_a, &BrowsePage::deleteFilesRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (BrowsePage::*)(const QString & , const QString & )>(_a, &BrowsePage::previewRequested, 3))
            return;
    }
}

const QMetaObject *BrowsePage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BrowsePage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10BrowsePageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int BrowsePage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void BrowsePage::extractRequested(const QString & _t1, const QStringList & _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3);
}

// SIGNAL 1
void BrowsePage::addFilesRequested(const QString & _t1, const QStringList & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void BrowsePage::deleteFilesRequested(const QString & _t1, const QStringList & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void BrowsePage::previewRequested(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}
QT_WARNING_POP
