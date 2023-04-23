#pragma once

#include <stddef.h>

extern int parseUrl(const char* url, char** protocol, char** hostname, int* port, char** path);
extern int httpGet(const char* url, char** resp, size_t* respsz);
extern int httpPost(const char* url, const char* data, char** resp, size_t* respsz);
