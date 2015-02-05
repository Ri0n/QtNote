#include <QColorDialog>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QStyleOption>
#include <QPainter>

#include "colorbutton.h"

namespace QtNote {

ColorButton::ColorButton(QWidget *parent, Qt::WindowFlags f) :
	QWidget(parent, f)
{
	setMinimumSize(20, 16);
	setColor(QColor(Qt::white));
}

void ColorButton::setColor(const QColor &color)
{
	_color = color;
	QString objName = objectName();
	QString w("QWidget");
	if (!objName.isEmpty()) {
		w += ("#" + objName);
	}
	QString style = QString("%1 { border:1px solid black;background-color:%2 }").arg(w, color.name());
	setStyleSheet(style);
}

void ColorButton::mousePressEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton) {
		QColorDialog d(_color, parentWidget());
		if (d.exec() == QDialog::Accepted) {
			setColor(d.selectedColor());
		}
		return;
	}
	QWidget::mousePressEvent(ev);
}

void ColorButton::paintEvent(QPaintEvent *pe)
{
	Q_UNUSED(pe);
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

} // namespace QtNote
