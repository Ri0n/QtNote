#include <QClipboard>
#include <QTextFragment>
#include <QPrinter>
#include <QPrintDialog>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QMessageBox>
#include <QToolButton>
#include <QPalette>
#include <QMenu>
#include <QStyle>
#if QT_VERSION >= 0x050400
# include <QGuiApplication>
#endif

#include "notewidget.h"
#include "ui_notewidget.h"
#include "typeaheadfind.h"
#include "notehighlighter.h"
#include "utils.h"
#include "defaults.h"

namespace QtNote {

static HighlighterExtension::Ptr firstLineHighlighter;
static QColor firstLineColor;

class FirstLineHighlighter : public HighlighterExtension
{
public:
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

class CurrentLinkHighlighter : public HighlighterExtension
{
    NoteWidget *noteWidget;
    QTextBlock currentBlock;
    int currentPos;
    int currentLength;

    Q_OBJECT

public:
    CurrentLinkHighlighter(NoteWidget *nw) :
        noteWidget(nw)
    {
        connect(noteWidget->editWidget(), SIGNAL(linkHovered()), SLOT(onLinkHovered()));
        connect(noteWidget->editWidget(), SIGNAL(linkUnhovered()), SLOT(onLinkUnhovered()));
    }

    void rehighlightLine(const QTextBlock &block = QTextBlock(), int pos = 0, int length = 0)
    {
        QTextBlock previousBlock = currentBlock;
        currentBlock = block;
        currentPos = pos;
        currentLength = length;

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

public slots:
    void onLinkHovered()
    {
        auto &hlp = noteWidget->editWidget()->hoveredLinkPosition();
        rehighlightLine(hlp.block, hlp.pos, hlp.length);
    }

    void onLinkUnhovered()
    {
        rehighlightLine();
    }
};

struct ActData {
    const char* icon;
    const char* text;
    const char* toolTip;
    const char* shortcut;
};

static struct MakeVSHappy {
	ActData save =    ActData{ "", QT_TR_NOOP("Save"), QT_TR_NOOP("Save note to file"), "Ctrl+S" };
	ActData copy =    ActData{ ":/icons/copy", QT_TR_NOOP("Copy"), QT_TR_NOOP("Copy note to clipboard"), "Ctrl+Shift+C" };
	ActData print =   ActData{ ":/icons/print", QT_TR_NOOP("Print"), QT_TR_NOOP("Print note"), "Ctrl+P" };
	ActData find =    ActData{ ":/icons/find", QT_TR_NOOP("Find"), QT_TR_NOOP("Find text in note"), "Ctrl+F" };
	ActData replace = ActData{ ":/icons/replace-text", QT_TR_NOOP("Replace"), QT_TR_NOOP("Replace text in note"), "Ctrl+R" };
	ActData trash =   ActData{ ":/icons/trash", QT_TR_NOOP("Delete"), QT_TR_NOOP("Delete note"), "Ctrl+D" };
} staticActData;

NoteWidget::NoteWidget(const QString &storageId, const QString &noteId) :
    ui(new Ui::NoteWidget),
    _storageId(storageId),
    _noteId(noteId)
{
    ui->setupUi(this);

    setFocusProxy(ui->noteEdit);
    //ui->saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    QHBoxLayout *hb3a = new QHBoxLayout();
    findBar = new TypeAheadFindBar(ui->noteEdit, QString::null, this);
    hb3a->addWidget(findBar);
    ui->noteLayout->addLayout(hb3a);

    _autosaveTimer.setInterval(10000);
    _lastChangeElapsed.start();
    connect(&_autosaveTimer, SIGNAL(timeout()), SLOT(autosave()));
    //connect(parent, SIGNAL(destroyed()), SLOT(close()));

    QToolBar *tbar = new QToolBar(this);
    ui->toolbarLayout->addWidget(tbar);

    QAction *act = initAction(staticActData.save);
    act->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onSaveClicked()));

    act = initAction(staticActData.copy);
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onCopyClicked()));

    act = initAction(staticActData.print);
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onPrintClicked()));

    tbar->addSeparator();

    act = initAction(staticActData.find);
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onFindTriggered()));
    QToolButton *findButton=
        dynamic_cast<QToolButton*>(tbar->widgetForAction(act));
    findButton->setPopupMode(QToolButton::InstantPopup);

    act = initAction(staticActData.replace);
    //tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onReplaceTriggered()));
    findButton->addAction(act);

    tbar->addSeparator();

    act = initAction(staticActData.trash);
    tbar->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(onTrashClicked()));

    connect(ui->noteEdit, SIGNAL(textChanged()), SLOT(textChanged()));

    ui->noteEdit->setText(""); // to force update event

    updateFirstLineColor();
    if (firstLineHighlighter.isNull()) {
        firstLineHighlighter = HighlighterExtension::Ptr(new FirstLineHighlighter());
    }
    _highlighter = new NoteHighlighter(ui->noteEdit);
    _highlighter->addExtension(firstLineHighlighter, NoteHighlighter::Title);

    _linkHighlighter = HighlighterExtension::Ptr(new CurrentLinkHighlighter(this));
    _highlighter->addExtension(_linkHighlighter, NoteHighlighter::Other, NoteHighlighter::Normal);

    connect(ui->noteEdit, SIGNAL(focusLost()), SLOT(save()));
    connect(ui->noteEdit, SIGNAL(focusReceived()), SLOT(focusReceived()), Qt::QueuedConnection);
#if QT_VERSION >= 0x050400
    connect(qGuiApp, &QGuiApplication::paletteChanged,
            [this](const QPalette &) { updateFirstLineColor(); });
#endif
}

NoteWidget::~NoteWidget()
{
    _autosaveTimer.stop();
    delete ui;
}

QAction* NoteWidget::initAction(const ActData &actData)
{
    QAction *act = new QAction(QIcon(actData.icon), tr(actData.text), this);
    act->setToolTip(actData.toolTip);
    act->setShortcut(QKeySequence(QLatin1String(actData.shortcut)));
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

void NoteWidget::keyPressEvent(QKeyEvent * event)
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
     if (_changed) { _changed = false; emit saveRequested(); _lastChangeElapsed.restart(); }
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
    QTextDocument *doc = ui->noteEdit->document();
    QTextBlock firstBlock = doc->begin();

    QString firstLine = firstBlock.text();
    if (firstLine != _firstLine || firstLine.isEmpty()) {
        _firstLine = firstLine;
        emit firstLineChanged();
    }
}

void NoteWidget::setText(QString text)
{
    ui->noteEdit->setPlainText(text);
    _changed = _noteId.isEmpty(); // force saving note if noteId is not set.
    _autosaveTimer.stop(); // timer not required atm
    _lastChangeElapsed.restart();
}

QString NoteWidget::text()
{
    return ui->noteEdit->toPlainText().trimmed();
}

NoteEdit* NoteWidget::editWidget() const
{
    return ui->noteEdit;
}

void NoteWidget::setAcceptRichText(bool state)
{
    ui->noteEdit->setAcceptRichText(state);
}

void NoteWidget::setNoteId(const QString &noteId)
{
    QString old = _noteId;
    _noteId = noteId;
    emit noteIdChanged(old, noteId);
}

void NoteWidget::onCopyClicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text());
}

void NoteWidget::onPrintClicked()
{
    static QPrinter::OutputFormat lastFormat = QPrinter::NativeFormat;
    static QString lastOutputFilename;
    static QString lastPrinterName;
    QPrinter printer;

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
            lastFormat = printer.outputFormat();
            lastPrinterName = printer.printerName();
        }
    }
}

void NoteWidget::onSaveClicked()
{
    enum Format {
        Text,
        RichText
    };
    static QMap<Format,QString> allFormats;
    if (allFormats.isEmpty()) {
        allFormats.insert(Text, tr("Text files (*.txt)"));
        allFormats.insert(RichText, tr("HTML files (*.html)"));
    }

    if (_extFileName.isEmpty() || !QFile::exists(_extFileName)) {
        QStringList filters = QStringList() << allFormats[Text];
        if (_features && RichText) {
            filters << allFormats[RichText];
        }
        _extFileName = QFileDialog::getSaveFileName(this, tr("Save Note As"),
#if QT_VERSION < 0x050000
            QDesktopServices::storageLocation(QDesktopServices::DesktopLocation),
#else
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
#endif
            filters.join(";;"), &_extSelecteFilter);
    }
    if (!_extFileName.isEmpty()) {
        QFile f(_extFileName);
        if (f.open(QIODevice::WriteOnly)) {
            QString text;
            Format format = allFormats.key(_extSelecteFilter);
            if (format == RichText) {
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
    if (!text().isEmpty() && s.value("ui.ask-on-delete", true).toBool() &&
        QMessageBox::question(0, tr("Deletion confirmation"),
                              tr("Are you sure want to delete this note?"),
                              QMessageBox::Yes | QMessageBox::No) !=
        QMessageBox::Yes) {
        return;
    }
    _changed = false;
    _trashRequested  = true;
    emit trashRequested();
}

void NoteWidget::updateFirstLineColor()
{
    QColor hlColor = QSettings().value("ui.title-color", Defaults::firstLineHighlightColor()).value<QColor>();
    QColor merged = Utils::mergeColors(hlColor, palette().color(QPalette::Text));
    if (merged != firstLineColor) {
        firstLineColor = merged;
        if (_highlighter) {
            _highlighter->rehighlight();
        }
    }
}

void NoteWidget::rereadSettings()
{
    updateFirstLineColor();
}

} // namespace QtNote

#include "notewidget.moc"
