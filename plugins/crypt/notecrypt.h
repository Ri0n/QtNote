#ifndef NOTECRYPT_H
#define NOTECRYPT_H

#include <QObject>

class NoteCrypt : public QObject
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Crypt")
#endif
	Q_INTERFACES()
public:
	explicit NoteCrypt(QObject *parent = 0);

signals:

public slots:

};

#endif // NOTECRYPT_H
