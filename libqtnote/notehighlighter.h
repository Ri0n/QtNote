#ifndef NOTEHIGHLIGHTER_H
#define NOTEHIGHLIGHTER_H

#include <QList>
#include <QPointer>
#include <QSyntaxHighlighter>

#include "highlighterext.h"
#include "qtnote_export.h"

namespace QtNote {

class NoteEdit;

class QTNOTE_EXPORT NoteHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    enum ExtType { Title, SpellCheck, Other };

    enum Priority { Low, Normal, High };

    struct ExtItem {
        bool                                active;
        ExtType                             type;
        Priority                            priority;
        std::weak_ptr<HighlighterExtension> ext;
    };

    struct Format {
        int             start;
        int             count;
        QTextCharFormat format;
    };

    NoteHighlighter(NoteEdit *nde);
    void highlightBlock(const QString &text);

    // virtual for plugins
    virtual void addExtension(std::shared_ptr<HighlighterExtension> extension, ExtType type = Other,
                              Priority prio = Normal);

    void disableExtension(ExtType type);
    void enableExtension(ExtType type);

    inline QTextBlock currentBlock() const { return QSyntaxHighlighter::currentBlock(); }
    virtual void      addFormat(int start, int count, const QTextCharFormat &format); // virtual for plugins

signals:

public slots:

protected:
    friend class HighlighterExtension;
    QList<ExtItem> extensions;
    QList<Format>  formats;
};

} // namespace QtNote

#endif // NOTEHIGHLIGHTER_H
