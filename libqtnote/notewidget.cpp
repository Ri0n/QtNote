#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGuiApplication>
#include <QImageReader>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QPrintDialog>
#include <QPrinter>
#include <QResizeEvent>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStyle>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QToolButton>
#include <QToolTip>
#include <QUuid>

#include "defaults.h"
#include "desktopeditorplatformbackend.h"
#include "desktopnoteeditorhost.h"
#include "iconutils.h"
#include "localmediastore.h"
#include "note.h"
#include "noteblockmodel.h"
#include "noteeditor.h"
#include "notehighlighter.h"
#include "notestorage.h"
#include "notewidget.h"
#include "speechaudiorecorder.h"
#include "speechrecognitionprovider.h"
#include "typeaheadfind.h"
#include "ui_notewidget.h"
#include "utils.h"

namespace QtNote {

static std::shared_ptr<HighlighterExtension> firstLineHighlighter;
static QColor                                firstLineColor;

namespace {
    QImage decodedMediaImage(const QByteArray &encoded, QString *error = nullptr)
    {
        QBuffer buffer;
        buffer.setData(encoded);
        if (!buffer.open(QIODevice::ReadOnly)) {
            if (error)
                *error = buffer.errorString();
            return {};
        }

        QImageReader reader(&buffer);
        reader.setAutoTransform(true);
        auto image = reader.read();
        if (image.isNull() && error)
            *error = reader.errorString();
        return image;
    }

    QSize mediaDisplaySize(const QSize &imageSize, int availableWidth)
    {
        return imageSize.scaled(qMin(qMax(1, availableWidth), imageSize.width()), imageSize.height(),
                                Qt::KeepAspectRatio);
    }

    QString displayText(const QTextBlock &block)
    {
        QString result;
        for (auto fragment = block.begin(); !fragment.atEnd(); ++fragment) {
            const auto textFragment = fragment.fragment();
            if (!textFragment.isValid())
                continue;
            const auto charFormat = textFragment.charFormat();
            if (!charFormat.isImageFormat()) {
                result += textFragment.text();
                continue;
            }
            const auto imageFormat = charFormat.toImageFormat();
            auto       label       = imageFormat.property(QTextFormat::ImageAltText).toString();
            if (label.isEmpty())
                label = imageFormat.property(QTextFormat::ImageTitle).toString();
            if (label.isEmpty())
                label = QFileInfo(QUrl(imageFormat.name()).path()).fileName();
            result += label;
        }
        return result.trimmed();
    }

    QAction *initAction(const QIcon icon, const QString &title, const QString &toolTip, const char *hotkey,
                        QWidget *parent)
    {
        QAction *act = new QAction(icon, title, parent);
        act->setToolTip(toolTip);
        act->setShortcut(QKeySequence(QLatin1String(hotkey)));
        act->setShortcutContext(Qt::WindowShortcut);
        return act;
    }
}

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
            auto pointSize = tb.charFormat().font().pointSizeF();
            if (pointSize <= 0)
                pointSize = tb.document()->defaultFont().pointSizeF();
            if (pointSize > 0)
                titleHighlightFormat.setFontPointSize(pointSize * 1.5);
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

NoteWidget::NoteWidget(const Note &note, const QUuid &draftId) : ui(new Ui::NoteWidget)
{
    ui->setupUi(this);

    editor = new NoteEditor(note, draftId, this);

    qmlEditor = new DesktopNoteEditorHost(editor, this);
    ui->noteLayout->insertWidget(1, qmlEditor);
    ui->noteEdit->hide(); // compatibility adapter for the legacy plugin API

    if (!editor->note().isNull() && editor->note().storage()) {
        auto storageFormats = editor->note().storage()->availableFormats();
        setAcceptRichText(storageFormats.contains(Note::Markdown) || storageFormats.contains(Note::Html));
    }

    setFocusProxy(qmlEditor);
    // ui->saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    QHBoxLayout *hb3a = new QHBoxLayout();
    findBar           = new TypeAheadFindBar(ui->noteEdit, QString(), this);
    hb3a->addWidget(findBar);
    ui->noteLayout->addLayout(hb3a);

    _autosaveTimer.setInterval(10000);
    _lastChangeElapsed.start();
    connect(&_autosaveTimer, SIGNAL(timeout()), SLOT(autosave()));
    _mediaResizeTimer.setSingleShot(true);
    _mediaResizeTimer.setInterval(50);
    connect(&_mediaResizeTimer, &QTimer::timeout, this, &NoteWidget::resizeMediaToViewport);
    connect(ui->noteEdit, &NoteEdit::imagePasteRequested, this, &NoteWidget::insertClipboardImage);
    // connect(parent, SIGNAL(destroyed()), SLOT(close()));

    QToolBar *tbar = new QToolBar(this);
    ui->toolbarLayout->addWidget(tbar);

    QAction *exportAction = initAction(nullptr, tr("Export"), tr("Export note to file"), "Ctrl+S");
    exportAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(exportAction, SIGNAL(triggered()), SLOT(onSaveClicked()));

    speechRecorder  = new SpeechAudioRecorder(this);
    auto speechIcon = QIcon::fromTheme(QLatin1String("audio-input-microphone"));
    if (speechIcon.isNull()) {
        speechIcon = IconUtils::symbolicIcon(QLatin1String(":/svg/microphone"));
    }
    speechIdleIcon = speechIcon;
    speechAction   = QtNote::initAction(speechIcon, tr("Mic"), tr("Hold to dictate text"), "", this);
    speechAction->setVisible(false);
    tbar->addAction(speechAction);
    speechButton = qobject_cast<QToolButton *>(tbar->widgetForAction(speechAction));
    if (speechButton) {
        speechButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        speechButton->setAutoRaise(true);
        connect(speechButton, &QToolButton::pressed, this, &NoteWidget::startSpeechRecognition);
        connect(speechButton, &QToolButton::released, this, &NoteWidget::finishSpeechRecognition);
    }
    connect(speechRecorder, &SpeechAudioRecorder::elapsedChanged, this, &NoteWidget::updateSpeechRecognitionProgress);
    connect(speechRecorder, &SpeechAudioRecorder::maxDurationReached, this, &NoteWidget::finishSpeechRecognition);
    connect(speechRecorder, &SpeechAudioRecorder::failed, this,
            [this](const QString &error) { showSpeechRecognitionError(error); });

    tbar->addSeparator();

    QAction *act = initAction(":/icons/copy", tr("Copy"), tr("Copy note to clipboard"), "Ctrl+Shift+C");
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onCopyClicked()));

    act = initAction(":/icons/find", tr("Find"), tr("Find text in note"), "Ctrl+F");
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onFindTriggered()));

    QToolButton *findButton = dynamic_cast<QToolButton *>(tbar->widgetForAction(act));
    findButton->setPopupMode(QToolButton::InstantPopup);

    act = initAction(":/icons/replace-text", tr("Replace"), tr("Replace text in note"), "Ctrl+R");
    // tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onReplaceTriggered()));
    findButton->addAction(act);

    auto pinIcon = QIcon::fromTheme(QLatin1String("view-pin-symbolic"));
    if (pinIcon.isNull())
        pinIcon = QIcon::fromTheme(QLatin1String("window-pin"));
    if (pinIcon.isNull())
        pinIcon = IconUtils::symbolicIcon(QLatin1String(":/svg/pin"));
    pinAction = QtNote::initAction(pinIcon, tr("Pin"), tr("Pin note to the desktop"), "", this);
    pinAction->setVisible(false);
    pinAction->setEnabled(!editor->noteId().isEmpty());
    tbar->addAction(pinAction);
    connect(pinAction, &QAction::triggered, this, &NoteWidget::pinRequested);

    act = initAction(":/icons/trash", tr("Delete"), tr("Delete note"), "Ctrl+D");
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onTrashClicked()));

    auto *toolbarSpacer = new QWidget(tbar);
    toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tbar->addWidget(toolbarSpacer);

    auto *printAction = initAction(":/icons/print", tr("Print"), tr("Print note"), "Ctrl+P");
    tbar->addAction(printAction);
    connect(printAction, SIGNAL(triggered()), SLOT(onPrintClicked()));
    tbar->addAction(exportAction);

    QSettings s;
    auto      fs = s.value("ui.default-font").toString();
    if (!fs.isEmpty()) {
        QFont defaultFont;
        defaultFont.fromString(fs);
        setFont(defaultFont);
    }

    connect(ui->noteEdit, SIGNAL(textChanged()), SLOT(textChanged()));
    connect(editor, &NoteEditor::textChanged, this, &NoteWidget::textChanged);
    connect(editor, &NoteEditor::formatChanged, this, [this] { syncEditorMode(editor->isMarkdown()); });
    connect(qmlEditor, &DesktopNoteEditorHost::focusLost, this, &NoteWidget::save);
    connect(qmlEditor, &DesktopNoteEditorHost::focusReceived, this, &NoteWidget::focusReceived, Qt::QueuedConnection);

    ui->noteEdit->setText(""); // to force update event

    updateFirstLineColor();
    if (!firstLineHighlighter) {
        firstLineHighlighter = std::make_shared<FirstLineHighlighter>();
    }
    _highlighter = new NoteHighlighter(ui->noteEdit->document());
    addHighlightExtension(firstLineHighlighter, NoteHighlighter::Title);

    _linkHighlighter = std::make_shared<CurrentLinkHighlighter>(this);
    std::dynamic_pointer_cast<CurrentLinkHighlighter>(_linkHighlighter)->init();
    _highlighter->addExtension(_linkHighlighter, NoteHighlighter::Other, NoteHighlighter::Normal);

    connect(ui->noteEdit, SIGNAL(focusLost()), SLOT(save()));
    connect(ui->noteEdit, SIGNAL(focusReceived()), SLOT(focusReceived()), Qt::QueuedConnection);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(qGuiApp, &QGuiApplication::paletteChanged, this, [this](const QPalette &) { updateFirstLineColor(); });
#endif
    initFromNote();
}

NoteWidget::~NoteWidget()
{
    _autosaveTimer.stop();
    delete ui;
}

QAction *NoteWidget::initAction(const char *icon, const QString &title, const QString &toolTip, const char *hotkey)
{
    return QtNote::initAction(icon ? QIcon(icon) : QIcon(), title, toolTip, hotkey, this);
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
    qmlEditor->flushPendingEditorChanges();
    if (!editor->isDirty())
        return;
    if (!editor->save())
        return;
    editor->breakHistoryMerge();
    _lastChangeElapsed.restart();
}

bool NoteWidget::prepareToClose()
{
    qmlEditor->flushPendingEditorChanges();
    return editor->close();
}

void NoteWidget::discardDraft() { editor->discardDraft(); }

void NoteWidget::autosave()
{
    if (!text().isEmpty() && editor->isDirty())
        save();
    else
        _autosaveTimer.stop();
}

void NoteWidget::focusReceived()
{
    if (_trashRequested || editor->isDirty())
        return;
    if (editor->reloadNewerDraft())
        textChanged();
}

void NoteWidget::textChanged()
{
    if (sender() == ui->noteEdit) {
        const QString legacyText = editor->isMarkdown() ? ui->noteEdit->toMarkdown() : ui->noteEdit->toPlainText();
        editor->setText(legacyText);
    } else {
        const QSignalBlocker blocker(ui->noteEdit);
        if (editor->isMarkdown())
            ui->noteEdit->setMarkdown(editor->text());
        else
            ui->noteEdit->setPlainText(editor->text());
    }

    if (pinAction)
        pinAction->setEnabled(!text().isEmpty());
    if (editor->isDirty() && !_autosaveTimer.isActive()) {
        _autosaveTimer.start();
        _lastChangeElapsed.restart();
    }
    const QString firstLine = text().section(QLatin1Char('\n'), 0, 0).trimmed();
    if (firstLine != _firstLine || firstLine.isEmpty()) {
        _firstLine = firstLine;
        emit firstLineChanged();
    }
}

void NoteWidget::setText(QString text)
{
    editor->loadDocument(text, editor->format(), NoteEditor::LoadPolicy::ResetHistory);
    _autosaveTimer.stop();
    _lastChangeElapsed.restart();
}

void NoteWidget::initFromNote()
{
    const auto current = editor->note();
    if (current.isNull()) {
        setContents(QString(), QString(), Note::PlainText);
        return;
    }
    setContents(current.title(), current.text(), current.format());
}

void NoteWidget::setContents(const QString &title, const QString &body, Note::Format format, ContentLoadPolicy policy)
{
    ui->noteEdit->blockSignals(true);
    _linkHighlighter->reset();

    const auto editorLoadPolicy = policy == ContentLoadPolicy::RecordFormatConversion
        ? NoteEditor::LoadPolicy::RecordFormatConversion
        : NoteEditor::LoadPolicy::ResetHistory;

    QString      contents;
    Note::Format targetFormat = format;
    switch (format) {
    case Note::Html:
        ui->noteEdit->setHtml(title + QLatin1String("<br/>") + body);
        ui->noteEdit->setMarkdown(ui->noteEdit->toMarkdown());
        contents     = ui->noteEdit->toMarkdown();
        targetFormat = Note::Markdown;
        break;
    case Note::Markdown:
        contents = title + QLatin1String("\n\n") + body;
        ui->noteEdit->setMarkdown(contents);
        break;
    case Note::PlainText:
        contents = title + QLatin1Char('\n') + body;
        ui->noteEdit->setPlainText(contents);
        break;
    }

    editor->setMedia(editor->note().media());
    editor->loadDocument(contents, targetFormat, editorLoadPolicy);
    syncEditorMode(targetFormat != Note::PlainText);
    loadMediaResources();
    resizeMediaToViewport();
    _autosaveTimer.stop();
    _lastChangeElapsed.restart();
    ui->noteEdit->blockSignals(false);
    textChanged();
}

void NoteWidget::loadMediaResources()
{
    for (const auto &reference : editor->note().media()) {
        if (!reference.mediaType.startsWith(QLatin1String("image/")))
            continue;
        const auto loaded = LocalMediaStore::instance()->data(reference.blobId);
        if (!loaded)
            continue;
        const auto preview = decodedMediaImage(loaded.value);
        if (!preview.isNull())
            ui->noteEdit->document()->addResource(QTextDocument::ImageResource, QUrl(reference.uri()), preview);
    }
    ui->noteEdit->document()->markContentsDirty(0, ui->noteEdit->document()->characterCount());
}

void NoteWidget::resizeMediaToViewport()
{
    auto                *document       = ui->noteEdit->document();
    const int            availableWidth = qMax(1, ui->noteEdit->viewport()->width() - 16);
    const bool           wasModified    = document->isModified();
    const QSignalBlocker editBlocker(ui->noteEdit);

    for (auto block = document->begin(); block.isValid(); block = block.next()) {
        for (auto fragment = block.begin(); !fragment.atEnd(); ++fragment) {
            const auto textFragment = fragment.fragment();
            if (!textFragment.isValid() || !textFragment.charFormat().isImageFormat())
                continue;
            auto       format   = textFragment.charFormat().toImageFormat();
            const auto resource = document->resource(QTextDocument::ImageResource, QUrl(format.name()));
            const auto image    = resource.value<QImage>();
            if (image.isNull())
                continue;
            const QSize target = mediaDisplaySize(image.size(), availableWidth);
            QTextCursor cursor(document);
            cursor.setPosition(textFragment.position());
            cursor.setPosition(textFragment.position() + textFragment.length(), QTextCursor::KeepAnchor);
            if (qRound(format.width()) != target.width() || qRound(format.height()) != target.height()) {
                format.setWidth(target.width());
                format.setHeight(target.height());
                cursor.mergeCharFormat(format);
            }
        }
    }
    document->setModified(wasModified);
}

void NoteWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    _mediaResizeTimer.start();
}

void NoteWidget::insertImage() { qmlEditor->platformBackend()->insertImage(); }

void NoteWidget::insertClipboardImage(const QImage &image)
{
    const auto name = QStringLiteral("Clipboard_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    insertRasterImage(image, name);
}

void NoteWidget::insertDroppedImage(const QImage &image, int row)
{
    const auto name = QStringLiteral("Dropped_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    insertRasterImage(image, name, row);
}

void NoteWidget::insertRasterImage(const QImage &image, const QString &name, int row)
{
    if (!canInsertImages() || image.isNull())
        return;

    QByteArray encoded;
    QBuffer    buffer(&encoded);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        QMessageBox::warning(this, tr("Insert image"), tr("Could not encode the clipboard image."));
        return;
    }
    const auto imported = LocalMediaStore::instance()->importData(encoded, name, QStringLiteral("image/png"));
    if (!imported) {
        QMessageBox::warning(this, tr("Insert image"), imported.error);
        return;
    }
    insertImportedImage(imported.value, row);
}

void NoteWidget::insertDroppedImageFiles(const QStringList &fileNames, int row)
{
    if (!canInsertImages())
        return;
    QList<MediaReference> references;
    for (const QString &fileName : fileNames) {
        const auto imported = LocalMediaStore::instance()->importFile(fileName);
        if (!imported) {
            QMessageBox::warning(this, tr("Insert image"), imported.error);
            return;
        }
        references.append(imported.value);
    }
    insertImportedImages(references, row, QStringLiteral("drop-images"));
}

bool NoteWidget::canInsertImages() const
{
    const auto storage = editor->note().storage();
    return storage && storage->supportsMedia() && isMarkdown();
}

bool NoteWidget::insertImportedImage(const MediaReference &reference, int row)
{
    return insertImportedImages({ reference }, row, QStringLiteral("insert-image"));
}

bool NoteWidget::insertImportedImages(const QList<MediaReference> &references, int row, const QString &historyKind)
{
    if (references.isEmpty() || !qmlEditor || !isMarkdown())
        return false;

    editor->beginHistoryTransaction(historyKind);
    auto media = editor->media();
    media.append(references);
    editor->setMedia(media);

    int insertionRow = row < 0 ? qmlEditor->model()->rowCount() : qBound(0, row, qmlEditor->model()->rowCount());
    for (const MediaReference &reference : references)
        qmlEditor->model()->insertImage(insertionRow++, reference.uri(), reference.originalName);
    editor->endHistoryTransaction();
    return true;
}

QString NoteWidget::text() { return editor ? editor->text().trimmed() : QString(); }

bool NoteWidget::isMarkdown() const { return editor && editor->isMarkdown(); }

Note NoteWidget::note() const { return editor->note(); }

QString NoteWidget::storageId() const { return editor->storageId(); }

QString NoteWidget::noteId() const { return editor->noteId(); }

QUuid NoteWidget::draftId() const { return editor->draftId(); }

bool NoteWidget::hasPersistedDraft() const { return editor->hasPersistedDraft(); }

NoteEdit *NoteWidget::editWidget() const { return ui->noteEdit; }

void NoteWidget::findText(const QString &text, bool focusFindBar) { findBar->search(text, focusFindBar); }

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

void NoteWidget::setStickyNotesAvailable(bool available)
{
    if (pinAction) {
        pinAction->setVisible(available);
        pinAction->setEnabled(!editor->noteId().isEmpty() || !text().isEmpty());
    }
}

void NoteWidget::rehighlight()
{
    _highlighter->rehighlight();
    qmlEditor->platformBackend()->rehighlight();
}

void NoteWidget::addHighlightExtension(const std::shared_ptr<HighlighterExtension> &extension, int type)
{
    _highlighter->addExtension(extension, NoteHighlighter::ExtType(type));
    qmlEditor->platformBackend()->addHighlightExtension(extension, type);
}

void NoteWidget::startSpeechRecognition()
{
    if (speechRecognitionBusy) {
        return;
    }
    if (!speechProvider || !speechProvider->isSpeechRecognitionReady() || !speechRecorder->isAvailable()) {
        updateSpeechRecognitionAction();
        return;
    }
    if (speechJob) {
        speechJob->cancel();
        speechJob->deleteLater();
        speechJob.clear();
    }

    auto caps    = speechProvider->speechRecognitionCapabilities();
    auto maxMs   = caps.maxOneShotDurationMs > 0 ? caps.maxOneShotDurationMs : 60000;
    auto started = speechRecorder->start(maxMs);
    if (!started) {
        showSpeechRecognitionError(speechRecorder->errorString());
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
        showSpeechRecognitionError(tr("Speech recognition provider did not start a recognition job"));
        return;
    }
    speechRecognitionBusy = true;
    updateSpeechRecognitionAction();
    if (speechButton) {
        QToolTip::showText(speechButton->mapToGlobal(QPoint(0, -speechButton->height())), tr("Recognizing speech..."),
                           speechButton, QRect(), 3000);
    }
    auto job = speechJob;
    connect(job, &SpeechRecognitionJob::finished, this, [this, job](const QString &text) {
        appendRecognizedText(text);
        if (speechJob == job) {
            speechJob.clear();
        }
        speechRecognitionBusy = false;
        updateSpeechRecognitionAction();
        job->deleteLater();
    });
    connect(job, &SpeechRecognitionJob::failed, this, [this, job](const QString &error) {
        showSpeechRecognitionError(error);
        if (speechJob == job) {
            speechJob.clear();
        }
        speechRecognitionBusy = false;
        updateSpeechRecognitionAction();
        job->deleteLater();
    });
}

void NoteWidget::cancelSpeechRecognition()
{
    if (speechRecorder) {
        speechRecorder->cancel();
    }
    if (speechJob) {
        speechJob->cancel();
        speechJob->deleteLater();
        speechJob.clear();
    }
    speechRecognitionBusy = false;
    updateSpeechRecognitionAction();
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

void NoteWidget::showSpeechRecognitionError(const QString &error)
{
    if (error.isEmpty()) {
        return;
    }
    if (speechButton) {
        QToolTip::showText(speechButton->mapToGlobal(QPoint(0, -speechButton->height())), error, speechButton, QRect(),
                           6000);
    }
}

void NoteWidget::switchToMarkdown() { editor->setMarkdown(true); }

void NoteWidget::switchToText() { editor->setMarkdown(false); }

void NoteWidget::syncEditorMode(bool markdown)
{
    ui->noteEdit->setAcceptRichText(markdown);
    ui->noteEdit->setUnconditionalLinks(markdown);
    ui->noteEdit->setImagePasteEnabled(editor->canInsertImages());
}

void NoteWidget::insertTableBlock()
{
    if (!editor->isMarkdown())
        editor->setMarkdown(true);
    qmlEditor->insertTable();
}

void NoteWidget::insertListBlock(int type)
{
    if (!editor->isMarkdown())
        editor->setMarkdown(true);
    qmlEditor->insertList(type);
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
            QTextDocument document;
            if (isMarkdown())
                document.setMarkdown(text(), QTextDocument::MarkdownDialectGitHub);
            else
                document.setPlainText(text());
            document.print(&printer);
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
                QTextDocument document;
                if (isMarkdown())
                    document.setMarkdown(this->text(), QTextDocument::MarkdownDialectGitHub);
                else
                    document.setPlainText(this->text());
                text = document.toHtml();
            } else {
                text = this->text();
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
        if (qmlEditor)
            qmlEditor->platformBackend()->rehighlight();
    }
}

void NoteWidget::rereadSettings()
{
    QFont     f;
    QSettings s;
    f.fromString(s.value("ui.default-font").toString());
    setFont(f);

    updateFirstLineColor();
    updateSpeechRecognitionAction();
}

void NoteWidget::updateSpeechRecognitionAction()
{
    if (!speechAction || !speechButton || !speechRecorder) {
        return;
    }
    auto language          = speechRecognitionLanguage();
    auto recorderAvailable = speechRecorder->isAvailable();
    auto providerReady     = speechProvider && speechProvider->isSpeechRecognitionReady();
    auto caps = speechProvider ? speechProvider->speechRecognitionCapabilities() : SpeechRecognitionCapabilities();
    bool languageSupported = !language.isEmpty() || caps.languages.isEmpty();
    bool available         = providerReady && recorderAvailable && languageSupported;
    speechAction->setVisible(available || speechRecognitionBusy);
    speechAction->setEnabled(available && !speechRecognitionBusy);
    speechButton->setEnabled(available && !speechRecognitionBusy);
    if (speechRecognitionBusy) {
        speechAction->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        speechAction->setToolTip(tr("Recognizing speech..."));
    } else {
        speechAction->setIcon(speechIdleIcon);
    }
    if (available && !speechRecognitionBusy) {
        speechAction->setToolTip(language.isEmpty() ? tr("Hold to dictate text")
                                                    : tr("Hold to dictate text (%1)").arg(language));
    }
}

void NoteWidget::appendRecognizedText(const QString &text)
{
    auto trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    qmlEditor->insertText(trimmed);
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
    if (!noteId().isEmpty()) {
        auto key = storageId().toUtf8() + '\0' + noteId().toUtf8();
        return QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex());
    }
    if (localSpeechContextId.isEmpty()) {
        const_cast<NoteWidget *>(this)->localSpeechContextId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    return localSpeechContextId;
}

} // namespace QtNote
