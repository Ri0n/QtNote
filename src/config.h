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

#endif // CONFIG_H
