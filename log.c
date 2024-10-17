#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <SDL_log.h>

void LogInfo(const char *format, ...) {
	va_list args;
    va_start(args, format);
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, format, args);
	va_end(args);
}

void LogWarn(const char *format, ...) {
	va_list args;
    va_start(args, format);
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, format, args);
	va_end(args);
}

void LogError(const char *format, ...) {
	va_list args;
    va_start(args, format);
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, format, args);
	va_end(args);
}
