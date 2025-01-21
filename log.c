#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <SDL_log.h>

extern int debug;

void LogInfo(const char *format, ...) {
	if(debug<=0)
		return;
	va_list args;
    va_start(args, format);
#ifdef __ANDROID__
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, format, args);
#else
	char buf[256];
	snprintf(buf, 255, "INFO: %s\n", format);
	buf[255]=0;
	vprintf(buf, args);
#endif
	va_end(args);
}

void LogWarn(const char *format, ...) {
	va_list args;
    va_start(args, format);
#ifdef __ANDROID__
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, format, args);
#else
	char buf[256];
	snprintf(buf, 255, "WARN: %s\n", format);
	buf[255]=0;
	vprintf(buf, args);
#endif
	va_end(args);
}

void LogError(const char *format, ...) {
	va_list args;
    va_start(args, format);
#ifdef __ANDROID__
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, format, args);
#else
	char buf[256];
	snprintf(buf, 255, "ERROR: %s\n", format);
	buf[255]=0;
	vprintf(buf, args);
#endif
	va_end(args);
}
