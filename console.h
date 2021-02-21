#pragma once

#include <stddef.h>

extern void ConsoleCreate(size_t font, float x, float y, float w, float h);
extern void ConsoleDelete();
extern int  ConsoleVisible();
extern void ConsoleShow();
extern void ConsoleHide();
extern void ConsoleLog(const char* msg);
extern void ConsoleWarn(const char* msg);
extern void ConsoleError(const char* msg);
extern void ConsoleDraw();
extern void ConsoleDraw_gl();

#include "value.h"
extern int DialogMessageBox(const char* msg, char* prompt, Value* options);
