#include <QCheckBox>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QPrintDialog>
#include <QPrinter>
#include <QSettings>
#include <QStandardPaths>
#include <QStyle>
#include <QTextFragment>
#include <QToolButton>
#include <QToolTip>
#include <QUuid>

#include "defaults.h"
#include "note.h"
#include "notehighlighter.h"
#include "notewidget.h"
#include "speechaudiorecorder.h"
#include "speechrecognitionprovider.h"
#include "typeaheadfind.h"
#include "ui_notewidget.h"
#include "utils.h"

namespace QtNote {

static std::shared_ptr<HighlighterExtension> firstLineHighlighter;
static QColor                                firstLineColor;

class FirstLineHighlighter : public HighlighterExtension {
public:
    using HighlighterExtension::HighlighterExtension;

    void reset() { }

    void highlight(NoteHighlighter *nh, const QString &text)
    {
        Q_UNUSED(text)
        QTextBlock tb = nh->currentBlock();
        if (tb.position() == 0) {
            QTextCharFormat titleHighlightFormat;
            titleHighlightFormat.setForeground(firstLineColor);
            titleHighlightFormat.setFontPointSize(tb.charFormat().font().pointSize() * 1.5);
            nh->addFormat(0, tb.length(), titleHighlightFormat);
        }
    }
};

class CurrentLinkHighlighter : public HighlighterExtension {
    NoteWidget *noteWidget;
    QTextBlock  currentBlock;
    int         currentPos;
    int         currentLength;

public:
    CurrentLinkHighlighter(NoteWidget *nw) : noteWidget(nw) { }

    void reset()
    {
        currentBlock  = {};
        currentPos    = {};
        currentLength = {};
    }

    void init()
    {
        auto                                  ew = noteWidget->editWidget();
        std::weak_ptr<CurrentLinkHighlighter> weak_self
            = std::dynamic_pointer_cast<CurrentLinkHighlighter>(shared_from_this());
        ew->connect(ew, &NoteEdit::linkHovered, ew, [this, weak_self]() {
            auto self = weak_self.lock();
            if (!self)
                return;
            auto  ew  = noteWidget->editWidget();
            auto &hlp = ew->hoveredLinkPosition();
            ew->blockSignals(true);
            rehighlightLine(hlp.block, hlp.pos, hlp.length);
            ew->blockSignals(false);
        });

        ew->connect(ew, &NoteEdit::linkUnhovered, ew, [this, weak_self]() {
            auto self = weak_self.lock();
            if (self) {
                auto ew = noteWidget->editWidget();
                ew->blockSignals(true);
                rehighlightLine();
                ew->blockSignals(false);
            }
        });
    }

    void rehighlightLine(const QTextBlock &block = QTextBlock(), int pos = 0, int length = 0)
    {
        QTextBlock previousBlock = currentBlock;
        currentBlock             = block;
        currentPos               = pos;
        currentLength            = length;

        NoteHighlighter *nh = noteWidget->highlighter();
        if (previousBlock.isValid() && previousBlock != currentBlock) {
            nh->rehighlightBlock(previousBlock); // drop highlighting from previous
        }
        if (currentBlock.isValid()) {
            nh->rehighlightBlock(currentBlock);
        }
    }

    void highlight(NoteHighlighter *nh, const QString &text)
    {
        Q_UNUSED(text);
        if (nh->currentBlock() != currentBlock) {
            return;
        }
        QTextCharFormat linkHighlightFormat;
        linkHighlightFormat.setForeground(qApp->palette().color(QPalette::Link));
        linkHighlightFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        nh->addFormat(currentPos, currentLength, linkHighlightFormat);
    }
};

NoteWidget::NoteWidget(const QString &storageId, const QString &noteId) :
    ui(new Ui::NoteWidget), _storageId(storageId), _noteId(noteId)
{
    ui->setupUi(this);

    setFocusProxy(ui->noteEdit);
    // ui->saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    QHBoxLayout *hb3a = new QHBoxLayout();
    findBar           = new TypeAheadFindBar(ui->noteEdit, QString(), this);
    hb3a->addWidget(findBar);
    ui->noteLayout->addLayout(hb3a);

    _autosaveTimer.setInterval(10000);
    _lastChangeElapsed.start();
    connect(&_autosaveTimer, SIGNAL(timeout()), SLOT(autosave()));
    // connect(parent, SIGNAL(destroyed()), SLOT(close()));

    QToolBar *tbar = new QToolBar(this);
    ui->toolbarLayout->addWidget(tbar);

    QAction *act = initAction(nullptr, tr("Save"), tr("Save note to file"), "Ctrl+S");
    act->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onSaveClicked()));

    mdModeAct = initAction(":/svg/markdown", tr("Markdown"), tr("Render markdown"), "Ctrl+M");
    tbar->addAction(mdModeAct);
    connect(mdModeAct, &QAction::triggered, this, &NoteWidget::switchToMarkdown);
    mdModeAct->setVisible(false); // we are initially in rich text mode
    ui->noteEdit->setUnconditionalLinks(true);

    txtModeAct = initAction(":/svg/txt", tr("Text"), tr("Plain text mode"), "Ctrl+T");
    tbar->addAction(txtModeAct);
    connect(txtModeAct, &QAction::triggered, this, &NoteWidget::switchToText);

    act = initAction(":/icons/copy", tr("Copy"), tr("Copy note to clipboard"), "Ctrl+Shift+C");
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onCopyClicked()));

    act = initAction(":/icons/print", tr("Print"), tr("Print note"), "Ctrl+P");
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onPrintClicked()));

    tbar->addSeparator();

    act = initAction(":/icons/find", tr("Find"), tr("Find text in note"), "Ctrl+F");
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onFindTriggered()));

    QToolButton *findButton = dynamic_cast<QToolButton *>(tbar->widgetForAction(act));
    findButton->setPopupMode(QToolButton::InstantPopup);

    act = initAction(":/icons/replace-text", tr("Replace"), tr("Replace text in note"), "Ctrl+R");
    // tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onReplaceTriggered()));
    findButton->addAction(act);

    tbar->addSeparator();

    speechRecorder = new SpeechAudioRecorder(this);
    speechButton   = new QToolButton(tbar);
    speechButton->setIcon(QIcon::fromTheme(QLatin1String("audio-input-microphone")));
    speechButton->setText(tr("Mic"));
    speechButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    speechButton->setToolTip(tr("Hold to dictate text"));
    speechButton->setAutoRaise(true);
    speechButton->setVisible(false);
    tbar->addWidget(speechButton);
    connect(speechButton, &QToolButton::pressed, this, &NoteWidget::startSpeechRecognition);
    connect(speechButton, &QToolButton::released, this, &NoteWidget::finishSpeechRecognition);
    connect(speechRecorder, &SpeechAudioRecorder::elapsedChanged, this, &NoteWidget::updateSpeechRecognitionProgress);
    connect(speechRecorder, &SpeechAudioRecorder::maxDurationReached, this, &NoteWidget::finishSpeechRecognition);
    connect(speechRecorder, &SpeechAudioRecorder::failed, this, [this](const QString &error) {
        QToolTip::showText(speechButton->mapToGlobal(QPoint(0, -speechButton->height())), error, speechButton);
    });

    act = initAction(":/icons/trash", tr("Delete"), tr("Delete note"), "Ctrl+D");
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onTrashClicked()));

    QSettings s;
    auto      fs = s.value("ui.default-font").toString();
    if (!fs.isEmpty()) {
        QFont defaultFont;
        defaultFont.fromString(fs);
        setFont(defaultFont);
    }

    connect(ui->noteEdit, SIGNAL(textChanged()), SLOT(textChanged()));

    ui->noteEdit->setText(""); // to force update event

    updateFirstLineColor();
    if (!firstLineHighlighter) {
        firstLineHighlighter = std::make_shared<FirstLineHighlighter>();
    }
    _highlighter = new NoteHighlighter(ui->noteEdit);
    _highlighter->addExtension(firstLineHighlighter, NoteHighlighter::Title);

    _linkHighlighter = std::make_shared<CurrentLinkHighlighter>(this);
    std::dynamic_pointer_cast<CurrentLinkHighlighter>(_linkHighlighter)->init();
    _highlighter->addExtension(_linkHighlighter, NoteHighlighter::Other, NoteHighlighter::Normal);

    connect(ui->noteEdit, SIGNAL(focusLost()), SLOT(save()));
    connect(ui->noteEdit, SIGNAL(focusReceived()), SLOT(focusReceived()), Qt::QueuedConnection);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(qGuiApp, &QGuiApplication::paletteChanged, this, [this](const QPalette &) { updateFirstLineColor(); });
#endif
}

NoteWidget::~NoteWidget()
{
    _autosaveTimer.stop();
    delete ui;
}

QAction *NoteWidget::initAction(const char *icon, const QString &title, const QString &toolTip, const char *hotkey)
{
    QAction *act = new QAction(icon ? QIcon(icon) : QIcon(), title, this);
    act->setToolTip(toolTip);
    act->setShortcut(QKeySequence(QLatin1String(hotkey)));
    act->setShortcutContext(Qt::WindowShortcut);
    return act;
}

void NoteWidget::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void NoteWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && findBar->isVisible()) {
        findBar->hide();
        return;
    }
    QWidget::keyPressEvent(event);
}

void NoteWidget::onFindTriggered()
{
    if (findBar->mode() == TypeAheadFindBar::Find) {
        findBar->searchTriggered(); /* just default */
    } else {
        findBar->setMode(TypeAheadFindBar::Find);
        findBar->open();
    }
}

void NoteWidget::onReplaceTriggered()
{
    if (findBar->mode() == TypeAheadFindBar::Replace) {
        findBar->searchTriggered(); /* just default */
    } else {
        findBar->setMode(TypeAheadFindBar::Replace);
        findBar->open();
    }
}

void NoteWidget::save()
{
    if (_changed) {
        _changed = false;
        emit saveRequested();
        _lastChangeElapsed.restart();
    }
}

void NoteWidget::autosave()
{
    if (!text().isEmpty() && _changed) {
        save();
    } else {
        _autosaveTimer.stop(); // stop until next text change
    }
}

void NoteWidget::focusReceived()
{
    if (!_trashRequested) {
        emit invalidated();
    }
}

void NoteWidget::textChanged()
{
    _changed = true;
    if (!_autosaveTimer.isActive()) {
        _autosaveTimer.start();
        _lastChangeElapsed.restart();
    }
    QTextDocument *doc        = ui->noteEdit->document();
    QTextBlock     firstBlock = doc->begin();

    QString firstLine = firstBlock.text();
    if (firstLine != _firstLine || firstLine.isEmpty()) {
        _firstLine = firstLine;
        emit firstLineChanged();
    }
}

void NoteWidget::setText(QString text)
{
    if (mdModeAct->isVisible()) {
        ui->noteEdit->setPlainText(text);
    } else {
        auto idx = text.indexOf('\n');
        if (idx == -1 || idx + 1 == text.size() || text[idx + 1] == '\n') {
            ui->noteEdit->setMarkdown(text);
        } else {
            ui->noteEdit->setMarkdown(text.left(idx) + '\n' + text.mid(idx));
        }
    }
    _changed = _noteId.isEmpty(); // force saving note if noteId is not set.
    _autosaveTimer.stop();        // timer not required atm
    _lastChangeElapsed.restart();
}

void NoteWidget::setNote(const Note &note)
{
    switch (note.format()) {
    case Note::Html:
        ui->noteEdit->setHtml(note.title() + QLatin1String("<br/>") + note.text());
        ui->noteEdit->setMarkdown(ui->noteEdit->toMarkdown()); // to cleanup html
        mdModeAct->setVisible(false);
        txtModeAct->setVisible(true);
        break;
    case Note::Markdown:
        ui->noteEdit->setMarkdown(note.title() + QLatin1String("\n\n") + note.text());
        mdModeAct->setVisible(false);
        txtModeAct->setVisible(true);
        break;
    case Note::PlainText:
        ui->noteEdit->setPlainText(note.title() + QLatin1Char('\n') + note.text());
        mdModeAct->setVisible(true);
        txtModeAct->setVisible(false);
        break;
    }
    _changed = false;
    _autosaveTimer.stop(); // timer not required atm
    _lastChangeElapsed.restart();
}

QString NoteWidget::text()
{
    if (mdModeAct->isVisible()) {
        return ui->noteEdit->toPlainText().trimmed();
    }
    return ui->noteEdit->toMarkdown().trimmed();
}

bool NoteWidget::isMarkdown() const { return !mdModeAct->isVisible(); }

NoteEdit *NoteWidget::editWidget() const { return ui->noteEdit; }

void NoteWidget::setAcceptRichText(bool state)
{
    _features.setFlag(RichText, state);
    ui->noteEdit->setAcceptRichText(state);
}

void NoteWidget::setSpeechRecognitionProvider(SpeechRecognitionProviderInterface *provider)
{
    speechProvider = provider;
    updateSpeechRecognitionAction();
}

void NoteWidget::setNoteId(const QString &noteId)
{
    QString old = _noteId;
    _noteId     = noteId;
    emit noteIdChanged(old, noteId);
}

void NoteWidget::rehighlight() { _highlighter->rehighlight(); }

void NoteWidget::startSpeechRecognition()
{
    if (!speechProvider || !speechProvider->isSpeechRecognitionReady() || !speechRecorder->isAvailable()) {
        updateSpeechRecognitionAction();
        return;
    }
    if (speechJob) {
        speechJob->cancel();
        speechJob.clear();
    }

    auto caps    = speechProvider->speechRecognitionCapabilities();
    auto maxMs   = caps.maxOneShotDurationMs > 0 ? caps.maxOneShotDurationMs : 60000;
    auto started = speechRecorder->start(maxMs);
    if (!started) {
        QToolTip::showText(speechButton->mapToGlobal(QPoint(0, -speechButton->height())), speechRecorder->errorString(),
                           speechButton);
    }
}

void NoteWidget::finishSpeechRecognition()
{
    if (!speechRecorder || !speechRecorder->isRecording()) {
        return;
    }

    auto audio = speechRecorder->stop();
    QToolTip::hideText();
    if (audio.data.isEmpty() || !speechProvider || !speechProvider->isSpeechRecognitionReady()) {
        updateSpeechRecognitionAction();
        return;
    }

    SpeechRecognitionRequest request;
    request.contextId = speechContextId();
    request.language  = speechRecognitionLanguage();
    request.locale    = request.language.isEmpty() ? QLocale() : QLocale(request.language);

    speechJob = speechProvider->recognizeSpeech(audio, request);
    if (!speechJob) {
        return;
    }
    connect(speechJob, &SpeechRecognitionJob::finished, this, [this](const QString &text) {
        appendRecognizedText(text);
        if (speechJob) {
            speechJob->deleteLater();
            speechJob.clear();
        }
    });
    connect(speechJob, &SpeechRecognitionJob::failed, this, [this](const QString &error) {
        QToolTip::showText(speechButton->mapToGlobal(QPoint(0, -speechButton->height())), error, speechButton);
        if (speechJob) {
            speechJob->deleteLater();
            speechJob.clear();
        }
    });
}

void NoteWidget::cancelSpeechRecognition()
{
    if (speechRecorder) {
        speechRecorder->cancel();
    }
    if (speechJob) {
        speechJob->cancel();
        speechJob.clear();
    }
    QToolTip::hideText();
}

void NoteWidget::updateSpeechRecognitionProgress(qint64 elapsedMs, qint64 maxDurationMs)
{
    if (!speechButton || maxDurationMs <= 0) {
        return;
    }
    auto remainingMs = qMax<qint64>(0, maxDurationMs - elapsedMs);
    auto seconds     = (remainingMs + 999) / 1000;
    QToolTip::showText(speechButton->mapToGlobal(QPoint(0, -speechButton->height())),
                       tr("%n second(s) left", nullptr, int(seconds)), speechButton);
}

void NoteWidget::switchToMarkdown()
{
    ui->noteEdit->blockSignals(true);
    _linkHighlighter->reset();
    ui->noteEdit->setAcceptRichText(true);

    auto txt   = text();
    auto nindx = txt.indexOf(QLatin1Char('\n'));
    if (nindx == -1 || nindx == txt.size() - 1 || txt[nindx + 1] == QLatin1Char('\n')) {
        ui->noteEdit->setMarkdown(txt);
    } else {
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
        ui->noteEdit->setMarkdown(txt.left(nindx) + QLatin1Char('\n') + txt.mid(nindx));
#else
        ui->noteEdit->setMarkdown(txt.left(nindx) + QLatin1Char('\n') + QStringView(txt).mid(nindx));
#endif
    }
    mdModeAct->setVisible(false);
    txtModeAct->setVisible(true);
    ui->noteEdit->setUnconditionalLinks(true);
    ui->noteEdit->blockSignals(false);
}

void NoteWidget::switchToText()
{
    ui->noteEdit->blockSignals(true);
    _linkHighlighter->reset();
    ui->noteEdit->setAcceptRichText(false);

    auto cursor = ui->noteEdit->textCursor();
    cursor.clearSelection();
    cursor.setPosition(0);
    ui->noteEdit->setTextCursor(cursor);

    auto md = ui->noteEdit->toMarkdown().trimmed();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // there is some bug in qt5 requiring this hack
    auto parts = md.split("\n    ```");
    for (int i = 1; i < parts.count() - 1; i += 2) {
        parts[i].replace("\n\n", "\n");
    }
    md = parts.join("\n```");
#endif

    ui->noteEdit->setPlainText(md);

    mdModeAct->setVisible(true);
    txtModeAct->setVisible(false);
    ui->noteEdit->setUnconditionalLinks(false);
    ui->noteEdit->blockSignals(false);
}

void NoteWidget::onCopyClicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text());
}

void NoteWidget::onPrintClicked()
{
    static QPrinter::OutputFormat lastFormat = QPrinter::NativeFormat;
    static QString                lastOutputFilename;
    static QString                lastPrinterName;
    QPrinter                      printer;

    if (!text().isEmpty()) {
        if (lastPrinterName.size()) {
            printer.setPrinterName(lastPrinterName);
        }
        printer.setOutputFileName(lastOutputFilename);
        printer.setOutputFormat(lastFormat);
        QPrintDialog printDialog(&printer, this);

        if (printDialog.exec() == QDialog::Accepted) {
            _highlighter->disableExtension(NoteHighlighter::SpellCheck);
            _highlighter->rehighlight();
            ui->noteEdit->print(&printer);
            _highlighter->enableExtension(NoteHighlighter::SpellCheck);
            _highlighter->rehighlight();
            lastOutputFilename = printer.outputFileName();
            lastFormat         = printer.outputFormat();
            lastPrinterName    = printer.printerName();
        }
    }
}

void NoteWidget::onSaveClicked()
{
    enum class Format { Text, RichText };
    static QMap<Format, QString> allFormats;
    if (allFormats.isEmpty()) {
        allFormats.insert(Format::Text, tr("Text files (*.txt)"));
        allFormats.insert(Format::RichText, tr("HTML files (*.html)"));
    }

    if (_extFileName.isEmpty() || !QFile::exists(_extFileName)) {
        QStringList filters = QStringList() << allFormats[Format::Text];
        if (_features & RichText) {
            filters << allFormats[Format::RichText];
        }
        _extFileName = QFileDialog::getSaveFileName(this, tr("Save Note As"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                    filters.join(";;"), &_extSelecteFilter);
    }
    if (!_extFileName.isEmpty()) {
        QFile f(_extFileName);
        if (f.open(QIODevice::WriteOnly)) {
            QString text;
            Format  format = allFormats.key(_extSelecteFilter);
            if (format == Format::RichText) {
                text = ui->noteEdit->toHtml();
            } else {
                text = ui->noteEdit->toPlainText();
            }
            QByteArray data = text.toLocal8Bit();
#ifdef Q_OS_WIN
            data = data.replace("\n", 1, "\r\n", 2);
#endif
            f.write(data);
            f.close();
        }
    }
}

void NoteWidget::onTrashClicked()
{
    QSettings s;
    if (!text().isEmpty() && s.value("ui.ask-on-delete", true).toBool()) {
        auto mb = new QMessageBox(QMessageBox::Question, tr("Deletion confirmation"),
                                  tr("Are you sure you want to delete this note?"), QMessageBox::Yes | QMessageBox::No);
        mb->setCheckBox(new QCheckBox(tr("Don't ask again")));
        auto res     = mb->exec();
        auto dontAsk = mb->checkBox()->isChecked();
        delete mb;
        if (res != QMessageBox::Yes) {
            return;
        }
        if (dontAsk) {
            s.setValue("ui.ask-on-delete", false);
        }
    }
    _changed        = false;
    _trashRequested = true;
    emit trashRequested();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool NoteWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        updateFirstLineColor();
    }
    return QWidget::event(event);
}
#endif

void NoteWidget::updateFirstLineColor()
{
    QColor hlColor = QSettings().value("ui.title-color", Defaults::firstLineHighlightColor()).value<QColor>();
    QColor merged  = Utils::mergeColors(hlColor, palette().color(QPalette::Text));
    if (merged != firstLineColor) {
        firstLineColor = merged;
        if (_highlighter) {
            _highlighter->rehighlight();
        }
    }
}

void NoteWidget::rereadSettings()
{
    QFont     f;
    QSettings s;
    f.fromString(s.value("ui.default-font").toString());
    setFont(f);

    updateFirstLineColor();
}

void NoteWidget::updateSpeechRecognitionAction()
{
    if (!speechButton || !speechRecorder) {
        return;
    }
    auto language  = speechRecognitionLanguage();
    bool available = speechProvider && speechProvider->isSpeechRecognitionReady() && speechRecorder->isAvailable()
        && (!language.isEmpty() || speechProvider->speechRecognitionCapabilities().languages.isEmpty());
    speechButton->setVisible(available);
    speechButton->setEnabled(available);
    if (available) {
        speechButton->setToolTip(language.isEmpty() ? tr("Hold to dictate text")
                                                    : tr("Hold to dictate text (%1)").arg(language));
    }
}

void NoteWidget::appendRecognizedText(const QString &text)
{
    auto trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    auto cursor = ui->noteEdit->textCursor();
    if (!cursor.atBlockStart() && !cursor.document()->isEmpty()) {
        cursor.insertText(QLatin1String(" "));
    }
    cursor.insertText(trimmed);
    ui->noteEdit->setTextCursor(cursor);
}

QString NoteWidget::speechRecognitionLanguage() const
{
    if (!speechProvider) {
        return QString();
    }

    auto caps          = speechProvider->speechRecognitionCapabilities();
    auto supportedList = caps.languages;
    for (auto &language : supportedList) {
        language = normalizeSpeechRecognitionLanguage(language);
    }
    supportedList.removeAll(QString());
    supportedList.removeDuplicates();

    auto systemLanguage = normalizeSpeechRecognitionLanguage(QLocale().name());
    if (supportedList.isEmpty()) {
        return systemLanguage;
    }

    if (supportedList.contains(systemLanguage, Qt::CaseInsensitive)) {
        return systemLanguage;
    }

    auto languageOnly = systemLanguage.section(QLatin1Char('-'), 0, 0);
    for (const auto &supported : supportedList) {
        if (supported.section(QLatin1Char('-'), 0, 0).compare(languageOnly, Qt::CaseInsensitive) == 0) {
            return supported;
        }
    }

    auto preferred = normalizeSpeechRecognitionLanguage(caps.preferredLanguage);
    if (!preferred.isEmpty() && supportedList.contains(preferred, Qt::CaseInsensitive)) {
        return preferred;
    }

    return supportedList.value(0);
}

QString NoteWidget::normalizeSpeechRecognitionLanguage(const QString &language) const
{
    auto normalized = language.trimmed();
    normalized.replace(QLatin1Char('_'), QLatin1Char('-'));
    if (normalized.isEmpty()) {
        return QString();
    }

    auto parts = normalized.split(QLatin1Char('-'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return QString();
    }

    parts[0] = parts[0].toLower();
    if (parts.size() > 1) {
        parts[1] = parts[1].toUpper();
    }
    return parts.join(QLatin1Char('-'));
}

QString NoteWidget::speechContextId() const
{
    if (!_noteId.isEmpty()) {
        auto key = _storageId.toUtf8() + '\0' + _noteId.toUtf8();
        return QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex());
    }
    if (localSpeechContextId.isEmpty()) {
        const_cast<NoteWidget *>(this)->localSpeechContextId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    return localSpeechContextId;
}

} // namespace QtNote
