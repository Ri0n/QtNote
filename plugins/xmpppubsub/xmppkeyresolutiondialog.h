#ifndef XMPPKEYRESOLUTIONDIALOG_H
#define XMPPKEYRESOLUTIONDIALOG_H

#include "xmppdto.h"

#include <QWizard>

class QLabel;
class QTableWidget;

namespace QtNote {

class XmppKeyResolutionDialog final : public QWizard {
    Q_OBJECT

public:
    explicit XmppKeyResolutionDialog(const XmppKeyAuditResult &audit, QWidget *parent = nullptr);

    QByteArray        canonicalKey() const;
    QList<QByteArray> availableKeys() const;

private:
    void updateSummary();

    XmppKeyAuditResult audit_;
    QTableWidget      *keys_;
    QLabel            *summary_;
};

} // namespace QtNote

#endif // XMPPKEYRESOLUTIONDIALOG_H
