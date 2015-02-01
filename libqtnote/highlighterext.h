#ifndef HIGHLIGHTEREXT_H
#define HIGHLIGHTEREXT_H

#include <QSharedPointer>

class QString;

namespace QtNote {

class NoteHighlighter;

class HighlighterExtension
{
public:
	typedef QSharedPointer<HighlighterExtension> Ptr;

	virtual ~HighlighterExtension() {}

	virtual void highlight(NoteHighlighter *, const QString &text) = 0;
};

} // namespace QtNote

#endif // HIGHLIGHTEREXT_H
