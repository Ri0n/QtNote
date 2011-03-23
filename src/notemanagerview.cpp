#include <QMenu>
#include <QContextMenuEvent>
#include "notemanagerview.h"
#include "notemanagermodel.h"
#include "notemanager.h"

NoteManagerView::NoteManagerView(QWidget *parent) :
    QTreeView(parent)
{
}

void NoteManagerView::contextMenuEvent(QContextMenuEvent *e)
{
	QModelIndex index = currentIndex();
	if (index.isValid() && index.parent().isValid()) { // note
		e->accept();
		QMenu m;
		m.addAction(QIcon(":/icons/trash"), tr("Delete"), this, SLOT(removeSelected()));
		m.exec(QCursor::pos());
	}
}

void NoteManagerView::removeSelected()
{
	QModelIndexList indexes = selectedIndexes();
	foreach (QModelIndex index, indexes) {
		NoteStorage *storage = NoteManager::instance()->storage(index.data(NoteManagerModel::StorageId).toString());
		QString noteId = index.data(NoteManagerModel::NoteId).toString();
		if (storage && !noteId.isEmpty()) {
			storage->deleteNote(noteId);
		}
	}
}
