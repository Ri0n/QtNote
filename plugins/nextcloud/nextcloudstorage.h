#ifndef NEXTCLOUDSTORAGE_H
#define NEXTCLOUDSTORAGE_H

#include "nextclouddto.h"
#include "notestorage.h"

#include <QHash>
#include <QThread>

namespace QtNote {

class NextcloudData;
class NextcloudWorker;

class NextcloudStorage final : public NoteStorage {
    Q_OBJECT
    Q_DISABLE_COPY(NextcloudStorage)

public:
    explicit NextcloudStorage(QObject *parent = nullptr);
    ~NextcloudStorage() override;

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

    bool loadData(NextcloudData &data);

    static QString storageId;

private:
    NextcloudConfig readConfig() const;
    bool            configIsValid(const NextcloudConfig &config, QString *error) const;
    Note            fromRemote(const NextcloudRemoteNote &remote);
    void            reportError(const QString &error, bool invalidate = false);
    void            applyConfig(const NextcloudConfig &config);

    NextcloudConfig      config_;
    QThread              workerThread_;
    NextcloudWorker     *worker_ { nullptr };
    QHash<QString, Note> cache_;
    bool                 cacheValid_ { false };
    bool                 accessible_ { false };
};

} // namespace QtNote

#endif // NEXTCLOUDSTORAGE_H
