#ifndef NOTEHIGHLIGHTER_H
#define NOTEHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QList>

#include "qtnote_export.h"
#include "highlighterext.h"

namespace QtNote {

class NoteEdit;

class QTNOTE_EXPORT NoteHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT
public:
    enum ExtType {
        Title,
        SpellCheck,
        Other
    };

    struct ExtItem {
        bool active;
        ExtType type;
        QWeakPointer<HighlighterExtension> ext;
    };

	struct Format
	{
		int start;
		int count;
		QTextCharFormat format;
	};

	NoteHighlighter(NoteEdit *nde);
	void highlightBlock(const QString &text);

    inline void addExtension(HighlighterExtension::Ptr extension, ExtType type = Other)
    { extensions.append(ExtItem{true, type, extension}); }

    void disableExtension(ExtType type);
    void enableExtension(ExtType type);

	inline QTextBlock currentBlock() const { return QSyntaxHighlighter::currentBlock(); }
	void addFormat(int start, int count, const QTextCharFormat &format);

signals:

public slots:

protected:
	friend class HighlighterExtension;
    QList<ExtItem> extensions;
	QList<Format> formats;
};

} // namespace QtNote

#endif // NOTEHIGHLIGHTER_H
