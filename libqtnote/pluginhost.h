#ifndef QTNOTE_PLUGINHOST_H
#define QTNOTE_PLUGINHOST_H

#include "pluginhostinterface.h"

#include <QObject>

class QTextEdit;

namespace QtNote {

class PluginHost : public QObject, public PluginHostInterface
{
    Q_OBJECT
public:
    explicit PluginHost(QObject *parent = nullptr);
    QString utilsCuttedDots(const QString &str, int n) override;
    NoteManager* noteManager() override;
    QString qtnoteDataDir() override;
    void rehighlight() override;
    NoteEdit *noteTextWidget(QWidget *w) override;
    void addHighlightExtension(QWidget *w, std::shared_ptr<HighlighterExtension> ext, int type) override;

signals:
    void rehightlight_requested();
};

} // namespace QtNote

#endif // QTNOTE_PLUGINHOST_H
