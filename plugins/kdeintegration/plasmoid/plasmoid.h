#ifndef QTNOTE_PLASMOID_H
#define QTNOTE_PLASMOID_H

#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QWindow>
#include <qqmlintegration.h>

class QDBusInterface;
class QDBusServiceWatcher;
class QTimer;

class NotesModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool loadingMore READ loadingMore NOTIFY loadingMoreChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

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

    bool    available() const;
    bool    loading() const;
    bool    loadingMore() const;
    bool    hasMore() const;
    QString query() const;
    void    setQuery(const QString &query);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void openNote(int row, QWindow *activationWindow = nullptr);
    Q_INVOKABLE void createNote(QWindow *activationWindow = nullptr);
    Q_INVOKABLE void showNoteManager(QWindow *activationWindow = nullptr);
    Q_INVOKABLE void showOptions(QWindow *activationWindow = nullptr);
    Q_INVOKABLE void showAbout(QWindow *activationWindow = nullptr);
    Q_INVOKABLE void quit();

signals:
    void countChanged();
    void availableChanged();
    void loadingChanged();
    void loadingMoreChanged();
    void hasMoreChanged();
    void queryChanged();

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
    void setLoadingMore(bool loadingMore);
    void setHasMore(bool hasMore);
    bool parseNotesResponse(const QString &response, QList<Item> *items, bool *hasMore) const;
    void requestPage(int offset, bool append);
    void call(const QString &method);
    void callWithActivationToken(const QString &method, const QVariantList &arguments, QWindow *activationWindow);
    void createInterface();
    bool startBackend();
    void callOrStart(const QString &method);
    void runPendingCall();

    QList<Item>          m_items;
    QDBusInterface      *m_interface         = nullptr;
    QDBusServiceWatcher *m_serviceWatcher    = nullptr;
    QTimer              *m_queryRefreshTimer = nullptr;
    QString              m_pendingCall;
    QString              m_query;
    bool                 m_available     = false;
    bool                 m_loading       = false;
    bool                 m_loadingMore   = false;
    bool                 m_hasMore       = false;
    bool                 m_starting      = false;
    quint64              m_requestSerial = 0;
    int                  m_pageSize      = 50;
};

#endif
