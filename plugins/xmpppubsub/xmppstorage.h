#ifndef XMPPSTORAGE_H
#define XMPPSTORAGE_H

#include "notestorage.h"
#include "xmppdto.h"

#include <QHash>
#include <QThread>

namespace QtNote {

class XmppData;
class XmppWorker;

class XmppStorage final : public NoteStorage {
    Q_OBJECT
    Q_DISABLE_COPY(XmppStorage)

public:
    explicit XmppStorage(QObject *parent = nullptr);
    ~XmppStorage() override;

    bool          init() override;
    const QString systemName() const override;
    const QString name() const override;
    QIcon         storageIcon() const override;
    QIcon         noteIcon() const override;
    bool          isAccessible() const override;

    QList<Note::Format> availableFormats() const override;
    QList<Note>         noteList(int limit = 0) override;
    Note                note(const QString &id) override;
    Note                createNote() override;
    bool                saveNote(const Note &note) override;
    void                removeNote(const QString &noteId) override;

    QWidget *settingsWidget() override;
    QString  tooltip() override;

    bool loadData(XmppData &data);

    static QString storageId;

private slots:
    void onRemoteNotePublished(const QtNote::XmppRemoteNote &remote);
    void onRemoteNoteRetracted(const QString &id);
    void onRemoteNodeInvalidated();
    void onConnectionChanged(bool connected);

private:
    XmppConfig readConfig() const;
    bool       configIsValid(const XmppConfig &config, QString *error) const;
    Note       fromRemote(const XmppRemoteNote &remote);
    void       reportError(const QString &error, bool invalidate = false);
    void       enterErrorState(const QString &error, bool invalidate = false);
    void       clearErrorState();
    void       applyConfig(const XmppConfig &config);

    XmppConfig           config_;
    QThread              workerThread_;
    XmppWorker          *worker_ { nullptr };
    QHash<QString, Note> cache_;
    bool                 cacheValid_ { false };
    bool                 accessible_ { false };
    bool                 errorState_ { false };
    QString              errorStateMessage_;
};

} // namespace QtNote

#endif // XMPPSTORAGE_H
