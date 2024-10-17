#pragma once
#include <stdarg.h>  // Required for va_list

extern void LogInfo(const char *format, ...);
extern void LogWarn(const char *format, ...);
extern void LogError(const char *format, ...);
