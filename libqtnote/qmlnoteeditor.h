#ifndef QMLNOTEEDITOR_H
#define QMLNOTEEDITOR_H

#include <QWidget>

#include "note.h"

class QQuickWidget;

namespace QtNote {
class NoteBlockModel;

class QTNOTE_EXPORT QmlNoteEditor : public QWidget {
    Q_OBJECT

public:
    explicit QmlNoteEditor(QWidget *parent = nullptr);

    NoteBlockModel *model() const { return model_; }
    void            load(const QString &contents, Note::Format format);
    QString         contents() const;
    bool            isMarkdown() const;

signals:
    void contentsChanged();
    void focusReceived();
    void focusLost();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    NoteBlockModel *model_ = nullptr;
    QQuickWidget   *quick_ = nullptr;
};
} // namespace QtNote

#endif
