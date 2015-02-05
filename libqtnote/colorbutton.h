#ifndef QTNOTE_COLORBUTTON_H
#define QTNOTE_COLORBUTTON_H

#include <QWidget>

namespace QtNote {

class ColorButton : public QWidget
{
	Q_OBJECT
public:
	explicit ColorButton(QWidget *parent=0, Qt::WindowFlags f=0);

	void setColor(const QColor &color);
	inline QColor color() const { return _color; }
signals:

public slots:

protected:
	void mousePressEvent(QMouseEvent *ev);
	void paintEvent(QPaintEvent *);
private:
	QColor _color;
};

} // namespace QtNote

#endif // QTNOTE_COLORBUTTON_H
