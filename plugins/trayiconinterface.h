class QWidget;

class TrayIconInterface
{
public:
    virtual void activateNote(QWidget *w) = 0;
};

Q_DECLARE_INTERFACE(TrayIconInterface,
                     "com.rion-soft.QtNote.TrayIconInterface/1.0")
