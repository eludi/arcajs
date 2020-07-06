#include "httpRequest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined __WIN32__ || defined WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <WinInet.h>
#else
#  include <curl/curl.h>
#endif

//------------------------------------------------------------------

typedef struct {
	char* data;
	size_t len, capacity;
} StringBuf;

StringBuf* StringBuf_create() {
	StringBuf* sb = (StringBuf*)malloc(sizeof(StringBuf));
	sb->capacity = 16;
	sb->data = (char*)malloc(sb->capacity);
	sb->len = 0;
	sb->data[sb->len]=0;
	return sb;
}

void StringBuf_delete(StringBuf* sb, int keepData) {
	if(!keepData)
		free(sb->data);
	free(sb);
}

void StringBuf_append(StringBuf* sb, const char* data, size_t len) {
	if(sb->len + len >= sb->capacity) {
		while(sb->len + len >= sb->capacity)
			sb->capacity *= 2;
		sb->data = (char*)realloc(sb->data, sb->capacity);
	}
	memcpy(sb->data+sb->len, data, len);
	sb->len += len;
	sb->data[sb->len]=0;
}

void StringBuf_set(StringBuf* sb, const char* data) {
	sb->len = 0;
	StringBuf_append(sb, data, strlen(data));
}

//------------------------------------------------------------------

int parseUrl(const char* url, char** protocol, char** hostname, int* port, char** path) {
	*protocol = *hostname = *path = NULL;
	char pathBuf[2048], host[100], prot[16];
	//sscanf(url, "%15[^:]://%99[^/]%1023[^?]%1023[^\n]", prot, host, pathname, search);
	int numMatches = sscanf(url, "%15[^:]://%99[^/]%2047[^\n]", prot, host, pathBuf);
	if(numMatches<2) {
		fprintf(stderr, "fully qualified url expected: protocol://host[:port][/path]\n");
		return -1;
	}

	*hostname = (char*)malloc(strlen(host)+1);
	if(sscanf(host, "%99[^:]:%99d", *hostname, port)<2)
		*port = -1;

	size_t len = strlen(prot);
	*protocol = (char*)malloc(len+2);
	memcpy(*protocol, prot, len);
	(*protocol)[len]=':';
	(*protocol)[len+1]=0;

	*path = strdup((numMatches==2) ? "/" : pathBuf);
	return 0;
}

#if defined __WIN32__ || defined WIN32
static int httpGetResponseCode(HINTERNET req) {
	DWORD status, size = sizeof(DWORD), index = 0;

	if (!HttpQueryInfo(req, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &status, &size, &index))
		return 0;
	return status;
}

#if 0
static char* httpGetContentType(HINTERNET req) {
	DWORD bufsz = 64;
	static char buffer[64];
	strcpy(buffer,"Content-Type");
	if(!HttpQueryInfo(req, HTTP_QUERY_CUSTOM, buffer, &bufsz, NULL))
		buffer[0]=0;
	else
		buffer[bufsz]=0;
	return buffer;
}
#endif

static HINTERNET winInetRequest(const char* url, const char* method, HINTERNET* session, HINTERNET* conn) {
	*session = *conn = NULL;
	char *protocol = NULL, *hostname = NULL, *path = NULL;
	int port;
	if(parseUrl(url, &protocol, &hostname, &port, &path)!=0) {
		free(protocol); free(hostname); free(path);
		return NULL;
	}
	int isHttps = 0;
	if(strcmp(protocol,"https:")==0) {
		isHttps = 1;
		if(port<0)
			port = INTERNET_DEFAULT_HTTPS_PORT;
	}
	else if(strcmp(protocol,"http:")==0) {
		if(port<0)
			port = INTERNET_DEFAULT_HTTP_PORT;
	}
	else {
		fprintf(stderr, "protocol %s not supported\n", protocol);
		free(protocol); free(hostname); free(path);
		return NULL;
	}
	free(protocol);

	*session = InternetOpen(TEXT("arcajs"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	HINTERNET req = NULL;
	if (*session) {
		*conn = InternetConnect(*session, hostname, port, NULL,NULL, INTERNET_SERVICE_HTTP, 0, 1);
		if (!*conn) {
			InternetCloseHandle(*session);
			*session = NULL;
		}
		else {
			DWORD flags = INTERNET_FLAG_KEEP_CONNECTION;
			if(isHttps)
				flags |= INTERNET_FLAG_SECURE;
			req = HttpOpenRequest(*conn, method, path, NULL, NULL, 0, flags, 1);
			if (!req) {
				InternetCloseHandle(*conn);
				InternetCloseHandle(*session);
				*session = *conn = NULL;
			}
		}
	}
	free(hostname);
	free(path);
	return req;
}

static void winInetReadResponse(HINTERNET req, StringBuf* sb) {
	char buffer[2048];
	for(;;) { // read response:
		DWORD numBytesRead = 0;
		BOOL isRead = InternetReadFile(req, buffer, 2048-1, &numBytesRead);
		if (isRead == FALSE || numBytesRead == 0)
			break;
		buffer[numBytesRead] = 0;
		StringBuf_append(sb, buffer, numBytesRead);
	}
}

#else
size_t writeFunction(void *ptr, size_t size, size_t nmemb, StringBuf* sb) {
	StringBuf_append(sb, (char*) ptr, size * nmemb);
	return size * nmemb;
}
#endif // WIN32

int httpGet(const char* url, char** resp) {
	*resp = NULL;
#if defined __WIN32__ || defined WIN32
	HINTERNET session, conn, req = winInetRequest(url, "GET", &session, &conn);
	if (!req)
		return -1;

	StringBuf* sb = StringBuf_create();
	if(HttpSendRequest(req, /*headers*/NULL, /*strlen(headers)*/0, /*body*/NULL, /*strlen(body)*/0))
		winInetReadResponse(req, sb);
	int ret = httpGetResponseCode(req);
	//printf("\t-> %i, %s\n", ret, httpGetContentType(req));
	*resp = sb->data;
	StringBuf_delete(sb, 1);

	InternetCloseHandle(req);
	InternetCloseHandle(conn);
	InternetCloseHandle(session);
	return ret;

#else
	CURL *curl = curl_easy_init();
	if(!curl)
		return -1;

	StringBuf* sb = StringBuf_create();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, sb);
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);

	int ret = curl_easy_perform(curl);
	if(ret != CURLE_OK) {
		StringBuf_set(sb, curl_easy_strerror(ret));
		ret = -ret;
	}
	else {
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		ret = (int)response_code;
		char* contentType = "";
		curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
	}
	curl_easy_cleanup(curl);
	*resp = sb->data;
	StringBuf_delete(sb, 1);
	return ret;
#endif
}

int httpPost(const char* url, const char* data, char** resp) {
	if(resp)
		*resp = NULL;
#if defined __WIN32__ || defined WIN32
	HINTERNET session, conn, req = winInetRequest(url, "POST", &session, &conn);
	if (!req)
		return -1;

	StringBuf* sb = StringBuf_create();
	if(HttpSendRequest(req, /*headers*/NULL, /*strlen(headers)*/0, (void*)data, strlen(data)) && resp)
		winInetReadResponse(req, sb);
	int ret = httpGetResponseCode(req);
	if(resp)
		*resp = sb->data;
	StringBuf_delete(sb, 1);

	InternetCloseHandle(req);
	InternetCloseHandle(conn);
	InternetCloseHandle(session);
	return ret;

#else
	CURL *curl = curl_easy_init();
	if(!curl)
		return -1;

	StringBuf* sb = StringBuf_create();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, sb);
	curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	int ret = curl_easy_perform(curl);
	if(ret != CURLE_OK)
		ret = -ret;
	else {
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		ret = (int)response_code;
	}
	curl_easy_cleanup(curl);
	if(resp)
		*resp = sb->data;
	StringBuf_delete(sb, 1);
	return ret;
#endif
}
