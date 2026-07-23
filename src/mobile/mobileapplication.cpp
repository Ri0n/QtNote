#include "mobileapplication.h"

#include "corestorageregistry.h"
#include "draftmanager.h"
#include "mobilebundledplugins.h"
#include "noteeditor.h"
#include "notemanager.h"
#include "notesmodel.h"
#include "notesworkspacecontroller.h"

#include <QSettings>

namespace QtNote {

MobileApplication::MobileApplication(QObject *parent) :
    QObject(parent), bundledPlugins_(this), plugins_(&bundledPlugins_, this), storages_(this)
{
    workspace_ = new NotesWorkspaceController(this);
    connect(workspace_, &NotesWorkspaceController::currentEditorChanged, this,
            &MobileApplication::currentNoteEditorChanged);

    QSettings settings;
    askBeforeDelete_ = settings.value(QStringLiteral("ui.ask-on-delete"), true).toBool();
    notesPerPage_    = settings.value(QStringLiteral("mobile.notes-per-page"), 30).toInt();
    workspace_->sourceModel()->setPageSize(notesPerPage_);
    editorFontSize_ = settings.value(QStringLiteral("mobile.editor-font-size"), 16.0).toReal();

    if (!DraftManager::instance()->initialize(&initializationError_))
        qWarning() << "Failed to initialize encrypted draft store:" << initializationError_;

    auto *notes = NoteManager::instance();
    connect(notes, &NoteManager::storageReady, this,
            [this](const NoteStorage::Ptr &storage) { recoverDraft(storage.data()); });
    registerCoreStorages();

    registerMobileBundledPlugins(bundledPlugins_);
    bundledPlugins_.initializeEnabledPlugins();
}

QAbstractItemModel *MobileApplication::notesModel() { return workspace_->recentNotesModel(); }
QAbstractItemModel *MobileApplication::pluginsModel() { return &plugins_; }
QAbstractItemModel *MobileApplication::storagesModel() { return &storages_; }
QObject            *MobileApplication::currentNoteEditor() const { return workspace_->currentEditor(); }
QObject            *MobileApplication::workspace() { return workspace_; }

bool  MobileApplication::askBeforeDelete() const { return askBeforeDelete_; }
int   MobileApplication::notesPerPage() const { return notesPerPage_; }
qreal MobileApplication::editorFontSize() const { return editorFontSize_; }

bool MobileApplication::createNote()
{
    if (!DraftManager::instance()->isReady()) {
        qWarning() << "Cannot create note; draft store is unavailable:" << initializationError_;
        return false;
    }
    return workspace_->createNote();
}

bool MobileApplication::openEditor(const Note &note, const QUuid &draftId)
{
    return workspace_->openNote(note, draftId);
}

void MobileApplication::recoverDraft(NoteStorage *storage)
{
    if (!storage || workspace_->currentEditor() || !DraftManager::instance()->isReady())
        return;
    for (const auto &draft : DraftManager::instance()->recoverableDrafts()) {
        if (!draft.storageId.isEmpty() && draft.storageId != storage->systemName())
            continue;
        auto note = draft.remoteNoteId.isEmpty() ? storage->createNote() : storage->note(draft.remoteNoteId);
        if (!note.isNull() && openEditor(note, draft.id))
            return;
    }
}

bool MobileApplication::saveCurrentNote() { return workspace_->saveCurrentNote(); }
bool MobileApplication::closeCurrentNote() { return workspace_->closeCurrentNote(); }

bool MobileApplication::setPluginEnabled(int row, bool enabled) { return plugins_.setEnabled(row, enabled); }

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
    workspace_->sourceModel()->setPageSize(value);
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
