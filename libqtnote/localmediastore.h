#ifndef LOCALMEDIASTORE_H
#define LOCALMEDIASTORE_H

#include "mediareference.h"

#include <QByteArray>
#include <QString>

namespace QtNote {

struct QTNOTE_EXPORT LocalMediaResult {
    MediaReference value;
    QString        error;
    explicit       operator bool() const { return error.isEmpty(); }
};

struct QTNOTE_EXPORT LocalMediaDataResult {
    QByteArray value;
    QString    error;
    explicit   operator bool() const { return error.isEmpty(); }
};

class QTNOTE_EXPORT LocalMediaStore {
public:
    explicit LocalMediaStore(const QString &rootPath = {}, const QByteArray &masterKey = {});
    static LocalMediaStore *instance();

    LocalMediaResult     importFile(const QString &fileName, const QUuid &attachmentId = {});
    LocalMediaResult     importData(const QByteArray &data, const QString &originalName, const QString &mediaType,
                                    const QUuid &attachmentId = {});
    LocalMediaDataResult data(const QByteArray &blobId) const;
    bool                 contains(const QByteArray &blobId) const;

private:
    QByteArray masterKey(QString *error) const;
    QString    blobPath(const QByteArray &blobId) const;

    QString    rootPath_;
    QByteArray masterKey_;
};

} // namespace QtNote

#endif // LOCALMEDIASTORE_H
