#ifndef HIGHLIGHTEREXT_H
#define HIGHLIGHTEREXT_H

#include <QObject>
#include <QSharedPointer>
#include <memory>

#include "qtnote_export.h"

class QString;

namespace QtNote {

class NoteHighlighter;

class QTNOTE_EXPORT HighlighterExtension : public std::enable_shared_from_this<HighlighterExtension> {
public:
    virtual void highlight(NoteHighlighter *, const QString &text) = 0;
    virtual void reset() = 0; // resets any state to initial. e.g. because of replacing note contents
};

} // namespace QtNote

#endif // HIGHLIGHTEREXT_H
