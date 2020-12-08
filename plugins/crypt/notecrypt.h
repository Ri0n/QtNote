#ifndef NOTECRYPT_H
#define NOTECRYPT_H

#include <QObject>

class NoteCrypt : public QObject {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Crypt")
    Q_INTERFACES()
public:
    explicit NoteCrypt(QObject *parent = 0);

signals:

public slots:
};

#endif // NOTECRYPT_H
