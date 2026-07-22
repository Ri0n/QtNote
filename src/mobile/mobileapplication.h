#ifndef QTNOTE_MOBILEAPPLICATION_H
#define QTNOTE_MOBILEAPPLICATION_H

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>

#include "note.h"

namespace QtNote {

class NoteEditor;
class NoteStorage;
class NotesWorkspaceController;

class EmptyListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    explicit EmptyListModel(QObject *parent = nullptr);

    int                    rowCount(const QModelIndex &parent = {}) const override;
    QVariant               data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
};

class MobileStoragesModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { StorageIdRole = Qt::UserRole + 1, NameRole, AccessibleRole, ConfigurableRole, TooltipRole };
    Q_ENUM(Role)

    explicit MobileStoragesModel(QObject *parent = nullptr);

    int                    rowCount(const QModelIndex &parent = {}) const override;
    QVariant               data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString storagePath() const;
};

class MobileApplication final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *notesModel READ notesModel CONSTANT)
    Q_PROPERTY(QObject *workspace READ workspace CONSTANT)
    Q_PROPERTY(QAbstractItemModel *pluginsModel READ pluginsModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *storagesModel READ storagesModel CONSTANT)
    Q_PROPERTY(QObject *currentNoteEditor READ currentNoteEditor NOTIFY currentNoteEditorChanged)
    Q_PROPERTY(bool askBeforeDelete READ askBeforeDelete WRITE setAskBeforeDelete NOTIFY askBeforeDeleteChanged)
    Q_PROPERTY(int notesPerPage READ notesPerPage WRITE setNotesPerPage NOTIFY notesPerPageChanged)
    Q_PROPERTY(qreal editorFontSize READ editorFontSize WRITE setEditorFontSize NOTIFY editorFontSizeChanged)

public:
    explicit MobileApplication(QObject *parent = nullptr);

    QAbstractItemModel *notesModel();
    QAbstractItemModel *pluginsModel();
    QAbstractItemModel *storagesModel();
    QObject            *currentNoteEditor() const;
    QObject            *workspace();

    bool  askBeforeDelete() const;
    int   notesPerPage() const;
    qreal editorFontSize() const;

    Q_INVOKABLE bool createNote();
    Q_INVOKABLE bool saveCurrentNote();
    Q_INVOKABLE bool closeCurrentNote();
    Q_INVOKABLE bool setPluginEnabled(int row, bool enabled);

public slots:
    void setAskBeforeDelete(bool value);
    void setNotesPerPage(int value);
    void setEditorFontSize(qreal value);

signals:
    void askBeforeDeleteChanged();
    void currentNoteEditorChanged();
    void notesPerPageChanged();
    void editorFontSizeChanged();

private:
    bool openEditor(const Note &note, const QUuid &draftId = {});
    void recoverDraft(NoteStorage *storage);

    MobileStoragesModel       storages_;
    EmptyListModel            plugins_;
    NotesWorkspaceController *workspace_ { nullptr };
    QString                   initializationError_;
    bool                      askBeforeDelete_ { true };
    int                       notesPerPage_ { 30 };
    qreal                     editorFontSize_ { 16.0 };
};

} // namespace QtNote

#endif // QTNOTE_MOBILEAPPLICATION_H
