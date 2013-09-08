#ifndef CONFIG_H
#define CONFIG_H

#if defined __cplusplus

#include <QtGlobal>

#ifdef Q_OS_WIN
#define WINVER _WIN32_WINNT_WIN2K
#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>
#include <WinUser.h>
#endif

#endif

#define QTNOTE_VERSION (QTNOTE_VERSION_PRODUCT << 24 | QTNOTE_VERSION_MAJOR << 16 | QTNOTE_VERSION_MINOR << 8 | QTNOTE_VERSION_PATCH)

#endif // CONFIG_H
