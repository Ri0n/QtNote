#include <QSyntaxHighlighter>

#include "spellchecker.h"
#include "engineinterface.h"

namespace QtNote {

SpellCheckHighlighter::SpellCheckHighlighter(SpellEngineInterface *sei, NoteDialogEdit *nde) :
   QSyntaxHighlighter(nde),
   sei(sei)
{

}

void SpellCheckHighlighter::highlightBlock(const QString &text)
{
	QTextCharFormat myClassFormat;
	myClassFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
	QString pattern = "\\b[A-Za-z]{2,}\\b";

	QRegExp expression(pattern);
	int index = text.indexOf(expression);
	while (index >= 0) {
		int length = expression.matchedLength();
		if (sei->spell(expression.capturedTexts()[0]) == 0) {
			setFormat(index, length, myClassFormat);
		}
		index = text.indexOf(expression, index + length);
	}
}

} // namespace QtNote
