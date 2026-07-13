#ifndef XMPPSTORAGE_H
#define XMPPSTORAGE_H

#include "notestorage.h"
#include "xmppdto.h"

#include <QHash>

namespace QtNote {

class XmppBackend;
class XmppSettingsWidget;

class XmppStorage final : public NoteStorage {
    Q_OBJECT
    Q_DISABLE_COPY(XmppStorage)

public:
    explicit XmppStorage(QObject *parent = nullptr, XmppBackend *backend = nullptr);
    ~XmppStorage() override;
    void shutdown() override;

    bool            init() override;
    StorageInitJob *initAsync(QObject *owner = nullptr) override;
    const QString   systemName() const override;
    const QString   name() const override;
    QIcon           storageIcon() const override;
    QIcon           noteIcon() const override;
    bool            isAccessible() const override;

    QList<Note::Format> availableFormats() const override;
    QList<Note>         noteList(int limit = 0) override;
    NoteListJob        *refreshNotesAsync(int limit = 0, QObject *owner = nullptr) override;
    Note                note(const QString &id) override;
    NoteLoadJob        *loadNoteAsync(const QString &id, QObject *owner = nullptr) override;
    Note                createNote() override;
    bool                loadNote(Note &note) override;
    bool                saveNote(const Note &note) override;
    NoteSaveJob        *saveNoteAsync(const Note &note, QObject *owner = nullptr) override;
    void                removeNote(const QString &noteId) override;
    NoteRemoveJob      *removeNoteAsync(const QString &noteId, QObject *owner = nullptr) override;

    QWidget *settingsWidget() override;
    QString  tooltip() override;

    static QString storageId;

signals:
    void encryptionKeyChanged(const QByteArray &keyId, const QString &message);

private slots:
    void onRemoteNotePublished(const QtNote::XmppRemoteNote &remote);
    void onRemoteNoteRetracted(const QString &id);
    void onRemoteNodeInvalidated();
    void onConnectionChanged(bool connected);

private:
    XmppConfig     readConfig() const;
    bool           configIsValid(const XmppConfig &config, QString *error) const;
    Note           fromRemote(const XmppRemoteNote &remote);
    void           applyRemote(Note &note, const XmppRemoteNote &remote);
    XmppRemoteNote toRemote(const Note &note) const;
    void           reportError(const QString &error, bool invalidate = false);
    void           enterErrorState(const QString &error, bool invalidate = false);
    void           clearErrorState();
    void           applyConfig(const XmppConfig &config);
    void           installReceivedStorageKey(const QString &jid, const QByteArray &key);
    void           resolveStorageKeys(const QString &jid, XmppSettingsWidget *settings = nullptr);

    XmppConfig           config_;
    XmppBackend         *backend_ { nullptr };
    QHash<QString, Note> cache_;
    bool                 cacheValid_ { false };
    bool                 accessible_ { false };
    bool                 errorState_ { false };
    QString              errorStateMessage_;
    QString              lastReportedError_;
    bool                 keyResolutionInProgress_ { false };
};

} // namespace QtNote

#endif // XMPPSTORAGE_H
