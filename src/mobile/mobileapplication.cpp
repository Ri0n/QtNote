#include "mobileapplication.h"

#include "draftmanager.h"
#include "noteeditor.h"
#include "notemanager.h"
#include "ptfstorage.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include <memory>

namespace QtNote {

namespace {
    constexpr auto PtfStorageId = "ptf";

    QString ptfStoragePath()
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/')
            + QLatin1String(PtfStorageId);
    }
}

EmptyListModel::EmptyListModel(QObject *parent) : QAbstractListModel(parent) { }

int EmptyListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 0;
}

QVariant EmptyListModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(index)
    Q_UNUSED(role)
    return {};
}

QHash<int, QByteArray> EmptyListModel::roleNames() const
{
    return {
        { Qt::UserRole + 1, "id" },          { Qt::UserRole + 2, "title" },        { Qt::UserRole + 3, "name" },
        { Qt::UserRole + 4, "description" }, { Qt::UserRole + 5, "versionText" },  { Qt::UserRole + 6, "enabled" },
        { Qt::UserRole + 7, "loaded" },      { Qt::UserRole + 8, "configurable" }, { Qt::UserRole + 9, "pluginId" },
        { Qt::UserRole + 10, "storageId" },  { Qt::UserRole + 11, "accessible" },  { Qt::UserRole + 12, "tooltip" },
    };
}

MobileStoragesModel::MobileStoragesModel(QObject *parent) : QAbstractListModel(parent) { QDir().mkpath(storagePath()); }

int MobileStoragesModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : 1; }

QVariant MobileStoragesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() != 0)
        return {};

    const auto path = storagePath();
    switch (role) {
    case StorageIdRole:
        return QLatin1String(PtfStorageId);
    case Qt::DisplayRole:
    case NameRole:
        return tr("Plain Text Storage");
    case AccessibleRole:
        return QFileInfo(path).isReadable();
    case ConfigurableRole:
        return false;
    case Qt::ToolTipRole:
    case TooltipRole:
        return path;
    default:
        return {};
    }
}

QHash<int, QByteArray> MobileStoragesModel::roleNames() const
{
    return {
        { StorageIdRole, "storageId" },       { NameRole, "name" },       { AccessibleRole, "accessible" },
        { ConfigurableRole, "configurable" }, { TooltipRole, "tooltip" },
    };
}

QString MobileStoragesModel::storagePath() const { return ptfStoragePath(); }

MobileApplication::MobileApplication(QObject *parent) : QObject(parent), storages_(this), plugins_(this), notes_(this)
{
    QSettings settings;
    askBeforeDelete_ = settings.value(QStringLiteral("ui.ask-on-delete"), true).toBool();
    notesPerPage_    = settings.value(QStringLiteral("mobile.notes-per-page"), 30).toInt();
    editorFontSize_  = settings.value(QStringLiteral("mobile.editor-font-size"), 16.0).toReal();

    if (!DraftManager::instance()->initialize(&initializationError_))
        qWarning() << "Failed to initialize encrypted draft store:" << initializationError_;

    auto *notes = NoteManager::instance();
    connect(notes, &NoteManager::storageReady, this,
            [this](const NoteStorage::Ptr &storage) { recoverDraft(storage.data()); });
    if (!notes->storage(PtfStorageId))
        notes->registerStorage(std::make_unique<PTFStorage>());
}

QAbstractItemModel *MobileApplication::notesModel() { return &notes_; }

QAbstractItemModel *MobileApplication::pluginsModel() { return &plugins_; }

QAbstractItemModel *MobileApplication::storagesModel() { return &storages_; }

QObject *MobileApplication::currentNoteEditor() const { return currentNoteEditor_; }

bool MobileApplication::askBeforeDelete() const { return askBeforeDelete_; }

int MobileApplication::notesPerPage() const { return notesPerPage_; }

qreal MobileApplication::editorFontSize() const { return editorFontSize_; }

bool MobileApplication::createNote()
{
    if (!DraftManager::instance()->isReady()) {
        qWarning() << "Cannot create note; draft store is unavailable:" << initializationError_;
        return false;
    }

    auto storage = NoteManager::instance()->defaultStorage();
    if (!storage || !storage->isAccessible()) {
        qWarning() << "Cannot create note; default storage is unavailable";
        return false;
    }

    auto note = storage->createNote();
    if (note.isNull()) {
        qWarning() << "Cannot create note shell";
        return false;
    }

    return openEditor(note);
}

bool MobileApplication::openEditor(const Note &note, const QUuid &draftId)
{
    if (currentNoteEditor_ && !closeCurrentNote())
        return false;
    currentNoteEditor_ = new NoteEditor(note, draftId, this);
    emit currentNoteEditorChanged();
    return true;
}

void MobileApplication::recoverDraft(NoteStorage *storage)
{
    if (!storage || currentNoteEditor_ || !DraftManager::instance()->isReady())
        return;
    for (const auto &draft : DraftManager::instance()->recoverableDrafts()) {
        if (!draft.storageId.isEmpty() && draft.storageId != storage->systemName())
            continue;
        auto note = draft.remoteNoteId.isEmpty() ? storage->createNote() : storage->note(draft.remoteNoteId);
        if (!note.isNull() && openEditor(note, draft.id))
            return;
    }
}

bool MobileApplication::saveCurrentNote()
{
    if (!currentNoteEditor_)
        return false;
    return currentNoteEditor_->save();
}

bool MobileApplication::closeCurrentNote()
{
    if (!currentNoteEditor_)
        return true;
    const auto editor = currentNoteEditor_;
    if (!editor->close())
        return false;
    editor->deleteLater();
    currentNoteEditor_.clear();
    emit currentNoteEditorChanged();
    return true;
}

bool MobileApplication::setPluginEnabled(int row, bool enabled)
{
    Q_UNUSED(row)
    Q_UNUSED(enabled)
    return false;
}

void MobileApplication::setAskBeforeDelete(bool value)
{
    if (askBeforeDelete_ == value)
        return;
    askBeforeDelete_ = value;
    QSettings().setValue(QStringLiteral("ui.ask-on-delete"), value);
    emit askBeforeDeleteChanged();
}

void MobileApplication::setNotesPerPage(int value)
{
    value = qBound(10, value, 200);
    if (notesPerPage_ == value)
        return;
    notesPerPage_ = value;
    QSettings().setValue(QStringLiteral("mobile.notes-per-page"), value);
    emit notesPerPageChanged();
}

void MobileApplication::setEditorFontSize(qreal value)
{
    value = qBound(10.0, value, 36.0);
    if (qFuzzyCompare(editorFontSize_, value))
        return;
    editorFontSize_ = value;
    QSettings().setValue(QStringLiteral("mobile.editor-font-size"), value);
    emit editorFontSizeChanged();
}

} // namespace QtNote
