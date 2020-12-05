#ifndef DEINTEGRATIONINTERFACE_H
#define DEINTEGRATIONINTERFACE_H

class QWidget;

namespace QtNote {

class DEIntegrationInterface {
public:
    virtual void activateWidget(QWidget *w) = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::DEIntegrationInterface, "com.rion-soft.QtNote.DEIntegrationInterface/1.0")

#endif
