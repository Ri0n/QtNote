#include "trayiconutils.h"

#include <QGuiApplication>
#include <QIcon>
#include <QSystemTrayIcon>

#ifdef Q_OS_WIN
#include <QColor>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif

namespace QtNote {

namespace {
    constexpr auto TrayIconName         = "qtnote";
    constexpr auto SymbolicTrayIconName = "qtnote-symbolic";
    constexpr auto ColorTrayIconPath    = ":/icons/trayicon";
    constexpr auto SymbolicTrayIconPath = ":/icons/trayicon-symbolic";

#ifdef Q_OS_WIN
    bool isDarkColorScheme()
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        const auto scheme = QGuiApplication::styleHints()->colorScheme();
        if (scheme == Qt::ColorScheme::Dark)
            return true;
        if (scheme == Qt::ColorScheme::Light)
            return false;
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
#else
        return false;
#endif
    }

    QPixmap tintedSymbolicPixmap(int size, const QColor &color)
    {
        QPixmap pixmap = QIcon(SymbolicTrayIconPath).pixmap(size, size);
        if (pixmap.isNull())
            return pixmap;

        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), color);
        return pixmap;
    }

    QIcon windowsTrayIcon()
    {
        QIcon        icon;
        const QColor color = isDarkColorScheme() ? Qt::white : Qt::black;

        for (int size : { 16, 20, 24, 32, 48 }) {
            const QPixmap pixmap = tintedSymbolicPixmap(size, color);
            if (!pixmap.isNull())
                icon.addPixmap(pixmap);
        }

        return icon.isNull() ? QIcon(ColorTrayIconPath) : icon;
    }
#endif
} // namespace

QString TrayIconUtils::themedTrayIconName() { return QLatin1String(SymbolicTrayIconName); }

QIcon TrayIconUtils::themedTrayIcon()
{
#ifdef Q_OS_WIN
    return windowsTrayIcon();
#else
    return QIcon::fromTheme(SymbolicTrayIconName, QIcon::fromTheme(TrayIconName, QIcon(ColorTrayIconPath)));
#endif
}

void TrayIconUtils::setupSystemTrayIcon(QSystemTrayIcon *trayIcon)
{
    if (!trayIcon)
        return;

    trayIcon->setIcon(themedTrayIcon());

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, trayIcon,
                     [trayIcon] { trayIcon->setIcon(themedTrayIcon()); });
#endif
}

} // namespace QtNote
