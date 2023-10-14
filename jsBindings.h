#pragma once
#include <stddef.h>
#include <stdint.h>
#include "value.h"

extern size_t jsvmInit(const char* storageFileName, const Value* args);
extern void jsvmClose(size_t vm);
extern int jsvmEval(size_t vm, const char* src, const char* fname);
extern int jsvmEvalScript(size_t vm, const char* fname);
extern void jsvmDispatchEvent(size_t vm, const char* event, const Value* data);
extern void jsvmDispatchGamepadEvents(size_t vm);
extern void jsvmDispatchDrawEvent(size_t vm);
extern void jsvmUpdateEventListeners(size_t vm);
extern void jsvmAsyncCalls(size_t vm, double timestamp);
extern const char* jsvmLastError(size_t vm);
/// imports javascript bindings from a shared dynamic library
extern int jsvmRequire(size_t vm, const char* dllName);

extern size_t jsonDecode(const char* str);
extern void jsonClose(size_t json);
extern char* jsonGetString(size_t json, const char* key); // todo: replace key by path
extern char** jsonGetStringArray(size_t json, const char* key);
extern double jsonGetNumber(size_t json, const char* key, double defaultValue);
