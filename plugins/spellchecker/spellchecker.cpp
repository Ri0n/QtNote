#include "spellchecker.h"

namespace QtNote {

SpellChecker::SpellChecker(NoteDialogEdit *parent) :
	QObject(parent),
	nde(parent)
{
	connect(parent->document(), SIGNAL(contentsChange(int,int,int)), SLOT(contentsChange(int,int,int)));
}

void SpellChecker::contentsChange(int position, int charsRemoved, int charsAdded)
{

}

} // namespace QtNote
