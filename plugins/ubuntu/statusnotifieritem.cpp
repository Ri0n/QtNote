#include "statusnotifieritem.h"

namespace QtNote {

class StatusNotifierItemPrivate
{
public:

};

StatusNotifierItem::StatusNotifierItem(QObject *parent) :
	QObject(parent),
	d(new StatusNotifierItemPrivate)
{

}

StatusNotifierItem::~StatusNotifierItem()
{
	delete d;
}

} // namespace QtNote
