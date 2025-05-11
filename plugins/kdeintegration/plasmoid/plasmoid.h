#ifndef QTNOTE_PLASMOID_H
#define QTNOTE_PLASMOID_H

#include <QAbstractListModel>
#include <QString>
#include <qqmlintegration.h>

class QDBusInterface;
class QDBusServiceWatcher;

class NotesModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    enum Role {
        NoteIdRole = Qt::UserRole + 1,
        StorageIdRole,
        TitleRole,
        ModifiedRole,
    };
    Q_ENUM(Role)

    explicit NotesModel(QObject *parent = nullptr);

    int                    rowCount(const QModelIndex &parent = {}) const override;
    QVariant               data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool available() const;
    bool loading() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void openNote(int row);
    Q_INVOKABLE void createNote();
    Q_INVOKABLE void showNoteManager();
    Q_INVOKABLE void showOptions();
    Q_INVOKABLE void showAbout();
    Q_INVOKABLE void quit();

signals:
    void countChanged();
    void availableChanged();
    void loadingChanged();

private slots:
    void serviceRegistered();
    void serviceUnregistered();

private:
    struct Item {
        QString id;
        QString storageId;
        QString title;
        QString modified;
    };

    void setAvailable(bool available);
    void setLoading(bool loading);
    void call(const QString &method);
    void createInterface();
    bool startBackend();
    void callOrStart(const QString &method);
    void runPendingCall();

    QList<Item>          m_items;
    QDBusInterface      *m_interface      = nullptr;
    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    QString              m_pendingCall;
    bool                 m_available     = false;
    bool                 m_loading       = false;
    bool                 m_starting      = false;
    quint64              m_requestSerial = 0;
};

#endif
