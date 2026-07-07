#ifndef KDEINTEGRATIONTRAY_H
#define KDEINTEGRATIONTRAY_H

#include "trayimpl.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class QAction;
class QMenu;
class KStatusNotifierItem;
#endif

namespace QtNote {

class Main;

class KDEIntegrationTray : public TrayImpl {
    Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Q_CLASSINFO("D-Bus Interface", "com.github.ri0n.QtNote")
#endif

public:
    explicit KDEIntegrationTray(Main *qtnote, QObject *parent);
    ~KDEIntegrationTray() override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
signals:
    Q_SCRIPTABLE void notesChanged();

public slots:
    Q_SCRIPTABLE QString notesJson(int offset, int limit, const QString &query) const;
    Q_SCRIPTABLE void    openNote(const QString &storageId, const QString &noteId);
    Q_SCRIPTABLE void    createNote();
    Q_SCRIPTABLE void    showNoteManager();
    Q_SCRIPTABLE void    showOptions();
    Q_SCRIPTABLE void    showAbout();
    Q_SCRIPTABLE void    quit();

private:
    void askAddPlasmoid();
    void addPlasmoidToPanel();
    bool isPlasmaSession() const;
    bool isPlasmoidInstalled() const;
#ifdef QTNOTE_DEVEL
    bool ensureDevelopmentPlasmoidLinks();
    void cleanupDevelopmentPlasmoidLinks();
    void clearDevelopmentPlasmaCache();
    void resetDevelopmentPlasmoidOnPanel();
    void removeDevelopmentPlasmoidFromPanel();
    bool updateDevelopmentPackage(const QString &sourcePath, const QString &packagePath, const QString &qmlSourcePath);
    bool updateDevelopmentFile(const QString &sourcePath, const QString &filePath);
    bool updateDevelopmentMainQml(const QString &sourcePath, const QString &filePath);
    bool updateDevelopmentQmlBackend(const QString &sourcePath, const QString &backendPath);
    bool updateDevelopmentFileContent(const QString &filePath, const QByteArray &content);
    bool updateDevelopmentLink(const QString &sourcePath, const QString &linkPath);
#endif

    bool registeredService = false;
#else
private slots:
    void showNotes(bool active, const QPoint &pos);

private:
    Main                *qtnote;
    KStatusNotifierItem *sni;
    QAction             *actNew;
    QMenu               *currentMenu = nullptr;
#endif
};

}

#endif // KDEINTEGRATIONTRAY_H
