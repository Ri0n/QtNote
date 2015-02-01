#ifndef NOTEHIGHLIGHTER_H
#define NOTEHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QList>

#include "highlighterext.h"

namespace QtNote {

class NoteEdit;

class NoteHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT
public:
	struct Format
	{
		int start;
		int count;
		QTextCharFormat format;
	};

	NoteHighlighter(NoteEdit *nde);
	void highlightBlock(const QString &text);

	inline void addExtension(HighlighterExtension::Ptr extension)
	{ extensions.append(extension); }

	inline QTextBlock currentBlock() const { return QSyntaxHighlighter::currentBlock(); }
	void addFormat(int start, int count, const QTextCharFormat &format);

signals:

public slots:

protected:
	friend class HighlighterExtension;
	QList<QWeakPointer<HighlighterExtension> > extensions;
	QList<Format> formats;
};

} // namespace QtNote

#endif // NOTEHIGHLIGHTER_H
