#include "qmlnoteeditor.h"

#include <QQmlContext>
#include <QQuickWidget>
#include <QVBoxLayout>

#include "noteblockmodel.h"

namespace QtNote {
QmlNoteEditor::QmlNoteEditor(QWidget *parent) : QWidget(parent), model_(new NoteBlockModel(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    quick_ = new QQuickWidget(this);
    quick_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_->setClearColor(palette().color(QPalette::Base));
    quick_->rootContext()->setContextProperty(QStringLiteral("noteBlockModel"), model_);
    quick_->setSource(QUrl(QStringLiteral("qrc:/qml/NoteBlockEditor.qml")));
    quick_->installEventFilter(this);
    layout->addWidget(quick_);

    connect(model_, &NoteBlockModel::contentsChanged, this, &QmlNoteEditor::contentsChanged);
}

void QmlNoteEditor::load(const QString &contents, Note::Format format)
{
    model_->load(contents, format != Note::PlainText);
}

QString QmlNoteEditor::contents() const { return model_->contents(); }
bool    QmlNoteEditor::isMarkdown() const { return model_->markdown(); }

bool QmlNoteEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == quick_) {
        if (event->type() == QEvent::FocusIn)
            emit focusReceived();
        else if (event->type() == QEvent::FocusOut)
            emit focusLost();
    }
    return QWidget::eventFilter(watched, event);
}
} // namespace QtNote
