#ifndef HIGHLIGHTEREXT_H
#define HIGHLIGHTEREXT_H

#include <QSharedPointer>
#include <QObject>

#include "qtnote_export.h"

class QString;

namespace QtNote {

class NoteHighlighter;

class QTNOTE_EXPORT HighlighterExtension : public QObject
{
    Q_OBJECT

public:
	typedef QSharedPointer<HighlighterExtension> Ptr;

	virtual ~HighlighterExtension() {}

	virtual void highlight(NoteHighlighter *, const QString &text) = 0;
    virtual void invalidate() { emit invalidated(); }

signals:
    void invalidated();
};

} // namespace QtNote

#endif // HIGHLIGHTEREXT_H
