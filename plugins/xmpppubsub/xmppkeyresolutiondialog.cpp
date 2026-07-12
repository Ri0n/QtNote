#include "xmppkeyresolutiondialog.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWizardPage>

#include <algorithm>

namespace QtNote {

XmppKeyResolutionDialog::XmppKeyResolutionDialog(const XmppKeyAuditResult &audit, QWidget *parent) :
    QWizard(parent), audit_(audit), keys_(new QTableWidget(audit.candidates.size(), 4)), summary_(new QLabel)
{
    setWindowTitle(tr("Resolve XMPP storage keys"));
    setOption(QWizard::NoBackButtonOnStartPage);
    resize(720, 430);

    auto *choose = new QWizardPage;
    choose->setTitle(tr("Choose the canonical storage key"));
    choose->setSubTitle(
        tr("QtNote found storage keys used by connected devices or existing notes. Select the key all notes and "
           "devices should use. Keys themselves are transferred only through trusted OMEMO sessions."));
    keys_->setHorizontalHeaderLabels({ tr("Key fingerprint"), tr("Available from"), tr("Notes"), tr("Status") });
    keys_->setSelectionBehavior(QAbstractItemView::SelectRows);
    keys_->setSelectionMode(QAbstractItemView::SingleSelection);
    keys_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    keys_->verticalHeader()->hide();
    keys_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int row = 0; row < audit.candidates.size(); ++row) {
        const auto &candidate = audit.candidates.at(row);
        keys_->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(candidate.keyId.left(8).toHex())));
        keys_->setItem(
            row, 1, new QTableWidgetItem(candidate.resource.isEmpty() ? tr("No online device") : candidate.resource));
        keys_->setItem(row, 2, new QTableWidgetItem(QString::number(candidate.indexItemCount)));
        QString status;
        if (candidate.key.isEmpty())
            status = tr("Key unavailable");
        else if (candidate.local)
            status = tr("Current device");
        else
            status = tr("Available");
        keys_->setItem(row, 3, new QTableWidgetItem(status));
    }
    const auto firstAvailable = std::find_if(audit.candidates.cbegin(), audit.candidates.cend(),
                                             [](const auto &candidate) { return !candidate.key.isEmpty(); });
    if (firstAvailable != audit.candidates.cend())
        keys_->selectRow(int(std::distance(audit.candidates.cbegin(), firstAvailable)));
    auto *chooseLayout = new QVBoxLayout(choose);
    chooseLayout->addWidget(keys_);
    auto *note = new QLabel(
        tr("Recovery is non-destructive: QtNote decrypts each accessible note with its original key, publishes the "
           "content first, then publishes its index with the canonical key. If interrupted, run this wizard again."));
    note->setWordWrap(true);
    chooseLayout->addWidget(note);
    addPage(choose);

    auto *confirm = new QWizardPage;
    confirm->setTitle(tr("Review recovery"));
    summary_->setWordWrap(true);
    auto *confirmLayout = new QVBoxLayout(confirm);
    confirmLayout->addWidget(summary_);
    confirmLayout->addStretch();
    addPage(confirm);
    connect(this, &QWizard::currentIdChanged, this, [this](int) { updateSummary(); });
    connect(keys_, &QTableWidget::itemSelectionChanged, this, &XmppKeyResolutionDialog::updateSummary);
    updateSummary();
}

QByteArray XmppKeyResolutionDialog::canonicalKey() const
{
    const auto row = keys_->currentRow();
    return row >= 0 && row < audit_.candidates.size() ? audit_.candidates.at(row).key : QByteArray {};
}

QList<QByteArray> XmppKeyResolutionDialog::availableKeys() const
{
    QList<QByteArray> result;
    for (const auto &candidate : audit_.candidates) {
        if (!candidate.key.isEmpty() && !result.contains(candidate.key))
            result.append(candidate.key);
    }
    return result;
}

void XmppKeyResolutionDialog::updateSummary()
{
    const auto row   = keys_->currentRow();
    const bool valid = row >= 0 && row < audit_.candidates.size() && !audit_.candidates.at(row).key.isEmpty();
    button(QWizard::NextButton)->setEnabled(valid);
    button(QWizard::FinishButton)->setEnabled(valid);
    if (!valid) {
        summary_->setText(tr("The selected key is not available. Bring one of its QtNote devices online or import its "
                             "recovery key before continuing."));
        return;
    }
    const auto &canonical    = audit_.candidates.at(row);
    int         inaccessible = 0;
    for (const auto &candidate : audit_.candidates) {
        if (candidate.key.isEmpty())
            inaccessible += candidate.indexItemCount;
    }
    summary_->setText(
        tr("Canonical key: %1\nNotes found: %2\nNotes whose key is currently unavailable: %3\n\n"
           "QtNote will not delete or overwrite inaccessible notes. The canonical key is installed locally only "
           "after every note has been republished successfully.")
            .arg(QString::fromLatin1(canonical.keyId.left(8).toHex()))
            .arg(audit_.totalIndexItems)
            .arg(inaccessible));
}

} // namespace QtNote
