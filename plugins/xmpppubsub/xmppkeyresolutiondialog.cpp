#include "xmppkeyresolutiondialog.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWizardPage>
#include <QXmppTrustLevel.h>

#include <algorithm>

namespace QtNote {
namespace {
    QLabel *wrappedLabel(const QString &text = {})
    {
        auto *label = new QLabel(text);
        label->setWordWrap(true);
        return label;
    }
}

XmppKeyResolutionDialog::XmppKeyResolutionDialog(const QList<XmppDeviceInfo> &devices, const QString &deviceError,
                                                 TrustDevices trustDevices, AuditKeys auditKeys,
                                                 RekeyStorage rekeyStorage, QWidget *parent) :
    QWizard(parent), trustDevices_(std::move(trustDevices)), auditKeys_(std::move(auditKeys)),
    rekeyStorage_(std::move(rekeyStorage)), devicesTable_(new QTableWidget), keysTable_(new QTableWidget),
    deviceStatus_(wrappedLabel()), keyStatus_(wrappedLabel()), summary_(wrappedLabel()), result_(wrappedLabel())
{
    setWindowTitle(tr("Repair XMPP note synchronization"));
    setOption(QWizard::NoBackButtonOnStartPage);
    setOption(QWizard::HaveHelpButton, false);
    resize(760, 480);

    auto *problem = new QWizardPage;
    problem->setTitle(tr("QtNote found incompatible storage keys"));
    problem->setSubTitle(tr("Notes or another QtNote device use a different encryption key."));
    auto *problemLayout = new QVBoxLayout(problem);
    problemLayout->addWidget(wrappedLabel(
        tr("This wizard will locate your other online QtNote devices, establish trusted OMEMO sessions, collect "
           "the storage keys they hold, and safely move every accessible note to one key you choose.")));
    problemLayout->addWidget(wrappedLabel(
        tr("No note or local key is changed until the final recovery step completes. You can cancel now and run "
           "the wizard again later.")));
    problemLayout->addStretch();
    setPage(ProblemPage, problem);

    auto *devicePage = new QWizardPage;
    devicePage->setTitle(tr("Verify your QtNote devices"));
    devicePage->setSubTitle(
        tr("Select devices you recognize. Their fingerprints are used only to establish encrypted OMEMO sessions."));
    devicesTable_->setColumnCount(4);
    devicesTable_->setHorizontalHeaderLabels({ tr("Use"), tr("Device"), tr("OMEMO fingerprint"), tr("Trust") });
    devicesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    devicesTable_->setSelectionMode(QAbstractItemView::NoSelection);
    devicesTable_->verticalHeader()->hide();
    devicesTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    devicesTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    auto *deviceLayout = new QVBoxLayout(devicePage);
    deviceLayout->addWidget(devicesTable_);
    deviceLayout->addWidget(deviceStatus_);
    deviceLayout->addWidget(wrappedLabel(
        tr("If a device is unfamiliar, leave it unchecked and compare its fingerprint on the other machine before "
           "continuing. Trust decisions are stored encrypted and survive restarts.")));
    setPage(DevicesPage, devicePage);

    auto *keysPage = new QWizardPage;
    keysPage->setTitle(tr("Choose the key to keep"));
    keysPage->setSubTitle(tr("QtNote has grouped existing notes and online devices by their storage-key fingerprint."));
    keysTable_->setColumnCount(4);
    keysTable_->setHorizontalHeaderLabels({ tr("Key fingerprint"), tr("Available from"), tr("Notes"), tr("Status") });
    keysTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    keysTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    keysTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    keysTable_->verticalHeader()->hide();
    keysTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    auto *keysLayout = new QVBoxLayout(keysPage);
    keysLayout->addWidget(keysTable_);
    keysLayout->addWidget(keyStatus_);
    keysLayout->addWidget(wrappedLabel(
        tr("Usually you should keep the key that owns the most notes. A key marked unavailable cannot be selected; "
           "bring one of its devices online or import its recovery key first.")));
    setPage(KeysPage, keysPage);

    auto *review = new QWizardPage;
    review->setTitle(tr("Review and repair"));
    review->setSubTitle(tr("QtNote is ready to republish accessible notes with the selected key."));
    auto *reviewLayout = new QVBoxLayout(review);
    reviewLayout->addWidget(summary_);
    reviewLayout->addStretch();
    setPage(ReviewPage, review);

    auto *resultPage = new QWizardPage;
    resultPage->setTitle(tr("Recovery result"));
    auto *resultLayout = new QVBoxLayout(resultPage);
    resultLayout->addWidget(result_);
    resultLayout->addStretch();
    setPage(ResultPage, resultPage);

    populateDevices(devices, deviceError);
    connect(keysTable_, &QTableWidget::itemSelectionChanged, this, &XmppKeyResolutionDialog::updateSummary);
    connect(this, &QWizard::currentIdChanged, this, [this](int) { updateSummary(); });
}

void XmppKeyResolutionDialog::populateDevices(const QList<XmppDeviceInfo> &devices, const QString &error)
{
    devices_ = devices;
    devicesTable_->setRowCount(devices.size());
    for (int row = 0; row < devices.size(); ++row) {
        const auto &device  = devices.at(row);
        const auto  level   = QXmpp::TrustLevel(device.trustLevel);
        const bool  trusted = level == QXmpp::TrustLevel::ManuallyTrusted || level == QXmpp::TrustLevel::Authenticated;
        auto       *use     = new QTableWidgetItem;
        use->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        use->setCheckState(trusted ? Qt::Checked : Qt::Unchecked);
        if (trusted)
            use->setFlags(Qt::ItemIsEnabled);
        devicesTable_->setItem(row, 0, use);
        devicesTable_->setItem(row, 1,
                               new QTableWidgetItem(device.label.isEmpty() ? tr("Unnamed device") : device.label));
        devicesTable_->setItem(row, 2, new QTableWidgetItem(QString::fromLatin1(device.keyId.toHex())));
        devicesTable_->setItem(row, 3, new QTableWidgetItem(trusted ? tr("Trusted") : tr("Needs confirmation")));
    }
    deviceStatus_->setText(error.isEmpty() ? tr("Found %1 OMEMO device(s) for this account.").arg(devices.size())
                                           : tr("Could not refresh OMEMO devices: %1").arg(error));
}

QList<QByteArray> XmppKeyResolutionDialog::selectedDeviceKeys() const
{
    QList<QByteArray> result;
    for (int row = 0; row < devices_.size(); ++row) {
        const auto level   = QXmpp::TrustLevel(devices_.at(row).trustLevel);
        const bool trusted = level == QXmpp::TrustLevel::ManuallyTrusted || level == QXmpp::TrustLevel::Authenticated;
        if (!trusted && devicesTable_->item(row, 0)->checkState() == Qt::Checked)
            result.append(devices_.at(row).keyId);
    }
    return result;
}

void XmppKeyResolutionDialog::populateKeys(const XmppKeyAuditResult &audit)
{
    audit_ = audit;
    keysTable_->setRowCount(audit.candidates.size());
    for (int row = 0; row < audit.candidates.size(); ++row) {
        const auto &candidate = audit.candidates.at(row);
        keysTable_->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(candidate.keyId.left(8).toHex())));
        keysTable_->setItem(
            row, 1, new QTableWidgetItem(candidate.resource.isEmpty() ? tr("Key not received") : candidate.resource));
        keysTable_->setItem(row, 2, new QTableWidgetItem(QString::number(candidate.indexItemCount)));
        keysTable_->setItem(row, 3,
                            new QTableWidgetItem(candidate.key.isEmpty() ? tr("Unavailable")
                                                     : candidate.local   ? tr("Current device")
                                                                         : tr("Available")));
    }
    const auto preferred
        = std::max_element(audit.candidates.cbegin(), audit.candidates.cend(), [](const auto &left, const auto &right) {
              const auto leftScore  = left.key.isEmpty() ? -1 : left.indexItemCount;
              const auto rightScore = right.key.isEmpty() ? -1 : right.indexItemCount;
              return leftScore < rightScore;
          });
    if (preferred != audit.candidates.cend() && !preferred->key.isEmpty())
        keysTable_->selectRow(int(std::distance(audit.candidates.cbegin(), preferred)));
    keyStatus_->setText(audit.error.isEmpty() ? tr("All responding QtNote devices were queried successfully.")
                                              : tr("Some devices could not provide a key:\n%1").arg(audit.error));
    updateSummary();
}

QByteArray XmppKeyResolutionDialog::canonicalKey() const
{
    const auto row = keysTable_->currentRow();
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

void XmppKeyResolutionDialog::setWorking(const QString &message)
{
    deviceStatus_->setText(message);
    setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool XmppKeyResolutionDialog::validateCurrentPage()
{
    if (currentId() == DevicesPage) {
        const auto selected       = selectedDeviceKeys();
        const bool alreadyTrusted = std::any_of(devices_.cbegin(), devices_.cend(), [](const auto &device) {
            const auto level = QXmpp::TrustLevel(device.trustLevel);
            return level == QXmpp::TrustLevel::ManuallyTrusted || level == QXmpp::TrustLevel::Authenticated;
        });
        if (selected.isEmpty() && !alreadyTrusted) {
            deviceStatus_->setText(
                tr("Select at least one QtNote device you recognize, then continue. If no expected device is shown, "
                   "start QtNote on the other machine and reopen this wizard."));
            return false;
        }
        setWorking(tr("Establishing trusted OMEMO sessions and requesting storage keys…"));
        const auto trusted = trustDevices_(selected);
        if (!trusted.ok) {
            QApplication::restoreOverrideCursor();
            setEnabled(true);
            deviceStatus_->setText(tr("Could not trust the selected device: %1").arg(trusted.error));
            return false;
        }
        const auto audit = auditKeys_();
        QApplication::restoreOverrideCursor();
        setEnabled(true);
        if (!audit.ok) {
            deviceStatus_->setText(tr("Could not collect storage keys: %1").arg(audit.error));
            return false;
        }
        populateKeys(audit);
        return true;
    }
    if (currentId() == KeysPage) {
        if (canonicalKey().isEmpty()) {
            keyStatus_->setText(tr("Select an available key before continuing."));
            return false;
        }
        updateSummary();
        return true;
    }
    if (currentId() == ReviewPage) {
        summary_->setText(summary_->text() + tr("\n\nRepairing notes…"));
        setEnabled(false);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        rekeyResult_ = rekeyStorage_(availableKeys(), canonicalKey());
        QApplication::restoreOverrideCursor();
        setEnabled(true);
        if (rekeyResult_.ok) {
            result_->setText(tr("Recovery completed successfully. %1 of %2 note(s) now use the canonical key.\n\n"
                                "The local storage key will be updated when you close this wizard.")
                                 .arg(rekeyResult_.migrated)
                                 .arg(rekeyResult_.total));
        } else {
            result_->setText(tr("Recovery is incomplete: %1\n\nMigrated %2 of %3 note(s). The local storage key "
                                "has not been changed. You can safely run this wizard again.")
                                 .arg(rekeyResult_.error)
                                 .arg(rekeyResult_.migrated)
                                 .arg(rekeyResult_.total));
        }
        return true;
    }
    return QWizard::validateCurrentPage();
}

void XmppKeyResolutionDialog::updateSummary()
{
    if (currentId() != ReviewPage && currentId() != KeysPage)
        return;
    const auto row = keysTable_->currentRow();
    if (row < 0 || row >= audit_.candidates.size() || audit_.candidates.at(row).key.isEmpty()) {
        summary_->setText(tr("No available canonical key is selected."));
        return;
    }
    const auto &canonical    = audit_.candidates.at(row);
    int         inaccessible = 0;
    for (const auto &candidate : audit_.candidates) {
        if (candidate.key.isEmpty())
            inaccessible += candidate.indexItemCount;
    }
    summary_->setText(
        tr("Canonical key: %1\nNotes found: %2\nNotes whose key is unavailable: %3\n\n"
           "For each accessible note QtNote will publish encrypted content first and its index second. Inaccessible "
           "notes are never overwritten or deleted. The operation can be safely resumed after interruption.")
            .arg(QString::fromLatin1(canonical.keyId.left(8).toHex()))
            .arg(audit_.totalIndexItems)
            .arg(inaccessible));
}

} // namespace QtNote
