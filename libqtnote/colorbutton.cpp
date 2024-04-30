#include <QColorDialog>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

#include "colorbutton.h"
#include "utils.h"

namespace QtNote {

ColorButton::ColorButton(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f) { setMinimumSize(20, 16); }

void ColorButton::setColor(QPalette::ColorRole role, const QColor &color)
{
    _role           = role;
    _color          = color;
    QColor  merged  = Utils::mergeColors(color, parentWidget()->palette().color(role));
    QString objName = objectName();
    QString w("QWidget");
    if (!objName.isEmpty()) {
        w += ("#" + objName);
    }
    QString style = QString("%1 { border:1px solid black;background-color:%2 }").arg(w, merged.name());
    setStyleSheet(style);
}

void ColorButton::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        QColorDialog d(parentWidget());
        d.setOption(QColorDialog::ShowAlphaChannel);
        d.setCurrentColor(_color);
        if (d.exec() == QDialog::Accepted) {
            setColor(_role, d.selectedColor());
        }
        return;
    }
    QWidget::mousePressEvent(ev);
}

void ColorButton::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe);
    QStyleOption opt;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    opt.init(this);
#else
    opt.initFrom(this);
#endif
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

} // namespace QtNote
