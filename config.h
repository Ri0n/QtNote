#ifndef CONFIG_H
#define CONFIG_H

#if defined __cplusplus

#include <QtGlobal>

#ifdef Q_OS_WIN
#ifndef WINVER
#define WINVER _WIN32_WINNT_VISTA
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <Windows.h>
#include <WinUser.h>
#endif

#endif

#define str_helper(s) #s
#define stringify(s) str_helper(s)

#define QTNOTE_VERSION (QTNOTE_VERSION_MAJOR << 16 | QTNOTE_VERSION_MINOR << 8 | QTNOTE_VERSION_PATCH)
#define QTNOTE_VERSION_STR stringify(QTNOTE_VERSION_MAJOR) "." stringify(QTNOTE_VERSION_MINOR) "." stringify(QTNOTE_VERSION_PATCH)

#endif // CONFIG_H
