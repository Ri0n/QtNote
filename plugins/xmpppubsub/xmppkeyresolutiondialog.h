#ifndef XMPPKEYRESOLUTIONDIALOG_H
#define XMPPKEYRESOLUTIONDIALOG_H

#include "xmppdto.h"

#include <QWizard>

#include <functional>

class QLabel;
class QTableWidget;

namespace QtNote {

/**
 * @brief Wizard for restoring a missing/divergent note master key.
 *
 * The wizard first establishes OMEMO trust, audits candidate keys received
 * from this account's devices, lets the user select a canonical key, and then
 * asks the backend to re-encrypt accessible notes. Protocol work is injected
 * as callbacks so the UI remains independent of QXmpp and Iris.
 */
class XmppKeyResolutionDialog final : public QWizard {
    Q_OBJECT

public:
    using StatusCompletion = std::function<void(XmppStatusResult)>;
    using AuditCompletion  = std::function<void(XmppKeyAuditResult)>;
    using RekeyCompletion  = std::function<void(XmppRekeyResult)>;
    using TrustDevices     = std::function<void(const QList<QByteArray> &, StatusCompletion)>;
    using AuditKeys        = std::function<void(AuditCompletion)>;
    using RekeyStorage     = std::function<void(const QList<QByteArray> &, const QByteArray &, RekeyCompletion)>;

    explicit XmppKeyResolutionDialog(bool localKeyMissing, const QList<XmppDeviceInfo> &devices,
                                     const QString &deviceError, TrustDevices trustDevices, AuditKeys auditKeys,
                                     RekeyStorage rekeyStorage, QWidget *parent = nullptr);

    QByteArray      canonicalKey() const;
    XmppRekeyResult rekeyResult() const { return rekeyResult_; }

protected:
    bool validateCurrentPage() override;

private:
    /// Fixed page order used by validateCurrentPage()'s asynchronous transitions.
    enum Page { ProblemPage, DevicesPage, KeysPage, ReviewPage, ResultPage };

    void              populateDevices(const QList<XmppDeviceInfo> &devices, const QString &error);
    void              populateKeys(const XmppKeyAuditResult &audit);
    void              updateSummary();
    QList<QByteArray> selectedDeviceKeys() const;
    QList<QByteArray> availableKeys() const;
    void              setWorking(const QString &message);

    QList<XmppDeviceInfo> devices_;
    XmppKeyAuditResult    audit_;
    XmppRekeyResult       rekeyResult_;
    TrustDevices          trustDevices_;
    AuditKeys             auditKeys_;
    RekeyStorage          rekeyStorage_;
    QTableWidget         *devicesTable_;
    QTableWidget         *keysTable_;
    QLabel               *deviceStatus_;
    QLabel               *keyStatus_;
    QLabel               *summary_;
    QLabel               *result_;
    bool                  devicesComplete_ { false };  ///< OMEMO trust stage completed successfully.
    bool                  rekeyComplete_ { false };    ///< Remote migration completed successfully.
    bool                  operationPending_ { false }; ///< Prevents navigation while a callback is outstanding.
};

} // namespace QtNote

#endif // XMPPKEYRESOLUTIONDIALOG_H
