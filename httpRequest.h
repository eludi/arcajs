#pragma once

extern int parseUrl(const char* url, char** protocol, char** hostname, int* port, char** path);
extern int httpGet(const char* url, char** resp);
extern int httpPost(const char* url, const char* data, char** resp);
