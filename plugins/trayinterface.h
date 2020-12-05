#ifndef TRAYINTERFACE_H
#define TRAYINTERFACE_H

#include <QObject>

namespace QtNote {

class Main;
class TrayImpl;

class TrayInterface {
public:
    virtual TrayImpl *initTray(Main *qtnote) = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::TrayInterface, "com.rion-soft.QtNote.TrayInterface/1.1")

#endif // TRAYINTERFACE_H
