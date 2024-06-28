/**
	UDP datagram client/server module
*/
#include <stdio.h>	//printf
#include <string.h> //memset
#include <stdlib.h>
#if defined __WIN32__ || defined WIN32
#  include <winsock.h>
#  define socklen_t int
#  define SHUT_RDWR SD_BOTH
#  define close closesocket
#  define WINDOWS_LEAN_AND_MEAN
#  include <windows.h>
#  define _EXPORT __declspec(dllexport)
#else
#  include <unistd.h>
#  include <arpa/inet.h>
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <netdb.h>
#  define _EXPORT
#endif

#include <SDL_thread.h>
#include "../external/duk_config.h"
#include "../external/duktape.h"

void printError(const char* s) {
#if defined __WIN32__ || defined WIN32
	fprintf(stderr, "%s ERROR: %i\n",s, WSAGetLastError());
#else
	perror(s);
#endif
}

int socketHasData(int sd, unsigned long milliWait) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	struct timeval tv;
	tv.tv_sec  = milliWait/1000;
	tv.tv_usec = milliWait*1000;
	int rc = select(sd+1, &fds, NULL, NULL, &tv);
	return (rc>= 0 && FD_ISSET(sd, &fds)) ? 1 : 0;
}

int socketCreateServer(uint16_t port) {
	//create a UDP socket
	int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd == -1) {
		printError("init");
		return sd;
	}

	struct sockaddr_in si_me;
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket to port
	if( bind(sd, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
		printError("bind");
		close(sd);
		return -2;
	}
	return sd;
}

int socketCreateClient(const char* host, uint16_t port) {
	//create a UDP socket
	int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd == -1) {
		printError("init");
		return sd;
	}
    // convert address to long:
    uint32_t hostAddr;
    struct hostent* pHostInfo = gethostbyname(host);
    memcpy(&hostAddr, pHostInfo->h_addr, pHostInfo->h_length);

    // fill address struct:
	struct sockaddr_in si_me;
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = hostAddr;

    // connect to host:
    if(connect(sd, (struct sockaddr*)&si_me, sizeof(si_me)) == -1) {
		printError("connect");
		close(sd);
		return -2;
    }
    return sd;
}

int socketRecv(int sd, char*buf, uint16_t bufSz) {
	if(sd<0)
		return 0;

	struct sockaddr_in si_other;
	socklen_t slen = sizeof(si_other);
	memset(buf,'\0', bufSz);

	// try to receive some data, this is a blocking call
	int bytesRecv = recvfrom(sd, buf, bufSz, 0, (struct sockaddr*)&si_other, &slen);
	if(bytesRecv<0)
		printError("recvfrom");
	return bytesRecv;
}


int socketRead(int sd, char* buf, uint16_t bufSz, uint16_t msTimeout) {
	if(sd==0) // stdin
		return !socketHasData(sd, msTimeout) ? 0 : read(sd, buf, bufSz);
	return (sd<0 || !socketHasData(sd, msTimeout)) ? 0 : socketRecv(sd, buf, bufSz);
}

int socketWrite(int sd, const char * buffer, uint16_t bufSz) {
	if(sd<0)
		return sd;
	return sd==0 ? write(1, buffer, bufSz) : send(sd, buffer, bufSz, 0);
}

void socketClose(int sd) {
	close(sd);
}


static duk_ret_t dk_socketRead(duk_context *ctx) {
	duk_push_this(ctx);
	duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("sd"));
	int sd = duk_get_int(ctx, -1);
	if(sd<0)
		return 0;

	uint16_t msTimeout = duk_is_number(ctx, 0) ? duk_to_uint16(ctx, 0) : 5;
	char buf[UINT16_MAX];
	int numBytesRead = socketRead(sd, buf, UINT16_MAX, msTimeout);
	if(numBytesRead<=0)
		return 0;
	memcpy(duk_push_fixed_buffer(ctx, numBytesRead), buf, numBytesRead);
	duk_push_buffer_object(ctx, -1, 0, numBytesRead, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t dk_socketWrite(duk_context *ctx) {
	duk_push_this(ctx);
	duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("sd"));
	int sd = duk_get_int(ctx, -1);
	if(sd<0)
		return duk_error(ctx, DUK_ERR_ERROR, "socket not open");

	duk_size_t len;
	const char* msg;
	if(duk_is_buffer_data(ctx,0))
		msg = duk_get_buffer_data(ctx, 0, &len);
	else {
		msg = duk_to_string(ctx, 0);
		len = duk_get_length(ctx, 0);
	}
	if(len>UINT16_MAX)
		return duk_error(ctx, DUK_ERR_ERROR, "message too long");

	int numBytesWritten = socketWrite(sd, msg, len);
	if(numBytesWritten!=len)
		return duk_error(ctx, DUK_ERR_ERROR, "write failed");
	return 0;
}

static duk_ret_t dk_socketClose(duk_context *ctx) {
	duk_push_this(ctx);
	duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("sd"));
	int sd = duk_get_int(ctx, -1);

	if(sd>=0) {
		socketClose(sd);
		duk_push_int(ctx, 0);
		duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("sd"));
	}
	return 0;
}

duk_ret_t dk_socketCreate(duk_context *ctx) {
	int sd = -1;
	if(duk_is_string(ctx, 0) && strcmp(duk_get_string(ctx, 0), "stdio")==0)
		sd = 0;
	else {
		uint16_t port = duk_to_uint16(ctx, 0);
		const char* host = duk_is_string(ctx, 1) ? duk_get_string(ctx, 1) : NULL;
		sd = host ? socketCreateClient(host, port) : socketCreateServer(port);
		if(sd<=0)
			return host ? duk_error(ctx, DUK_ERR_ERROR, "failed to connect to host %s:%u", host, port)
				: duk_error(ctx, DUK_ERR_ERROR, "failed to open socket at port %u", port);
	}

	duk_push_object(ctx);
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("Socket_prototype"));
	duk_set_prototype(ctx, -2);

	duk_push_int(ctx, sd);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("sd"));

	return 1;
}

void _EXPORT dgram_exports(duk_context *ctx) {
#if defined __WIN32__ || defined WIN32
	WSADATA wsda;
	WSAStartup(0x0101,&wsda);
#endif

	duk_push_object(ctx);
	duk_push_c_function(ctx, dk_socketClose, 0);
	duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, dk_socketClose, 0);
	duk_put_prop_literal(ctx, -2, "close");
	duk_push_c_function(ctx, dk_socketRead, 1);
	duk_put_prop_string(ctx, -2, "read");
	duk_push_c_function(ctx, dk_socketWrite, 1);
	duk_put_prop_string(ctx, -2, "write");
	duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("Socket_prototype"));

	duk_push_object(ctx);
	duk_push_c_function(ctx, dk_socketCreate, 2);
	duk_put_prop_string(ctx, -2, "createSocket");
}

void _EXPORT dgram_unload() {
#if defined __WIN32__ || defined WIN32 || defined _WIN32
	WSACleanup();
#endif
	//printf(" dgram..."); fflush(stdout);
}
