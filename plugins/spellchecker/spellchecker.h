#ifndef QTNOTE_SPELLCHECKER_H
#define QTNOTE_SPELLCHECKER_H

#include <QObject>

#include "notedialogedit.h"

namespace QtNote {

class SpellChecker : public QObject
{
	Q_OBJECT
public:
	explicit SpellChecker(NoteDialogEdit *parent = 0);

signals:

public slots:

private slots:
	void contentsChange(int position, int charsRemoved, int charsAdded);

private:
	NoteDialogEdit *nde;
};

} // namespace QtNote

#endif // QTNOTE_SPELLCHECKER_H
