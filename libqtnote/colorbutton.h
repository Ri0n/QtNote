#ifndef QTNOTE_COLORBUTTON_H
#define QTNOTE_COLORBUTTON_H

#include <QWidget>

namespace QtNote {

class ColorButton : public QWidget {
    Q_OBJECT
public:
    explicit ColorButton(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void          setColor(QPalette::ColorRole role, const QColor &color);
    inline QColor color() const { return _color; }
signals:

public slots:

protected:
    void mousePressEvent(QMouseEvent *ev);
    void paintEvent(QPaintEvent *);

private:
    QPalette::ColorRole _role;
    QColor              _color;
};

} // namespace QtNote

#endif // QTNOTE_COLORBUTTON_H
