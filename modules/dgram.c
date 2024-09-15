/**
	UDP datagram client/server module
*/
#include <stdio.h>	//printf
#include <string.h> //memset
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#if defined __WIN32__ || defined WIN32
#  include <winsock.h>
#  include <io.h>
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

static void printError(const char* s) {
#if defined __WIN32__ || defined WIN32
	fprintf(stderr, "%s ERROR: %i\n",s, WSAGetLastError());
#else
	perror(s);
#endif
}

typedef struct {
	int sd;
	bool isServer, allowRemoteSocketChange;
	struct sockaddr_in saddr;
	socklen_t saddr_len;
} SocketData;

static SocketData* sockets=NULL;
static uint32_t numSockets=0, numSocketsMax=0;

bool socketHasData(int sd, unsigned long milliWait) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	struct timeval tv;
	tv.tv_sec  = milliWait/1000;
	tv.tv_usec = milliWait*1000;
	int rc = select(sd+1, &fds, NULL, NULL, &tv);
	return (rc>= 0 && FD_ISSET(sd, &fds)) ? true : false;
}

int socketCreateServer(uint16_t port, bool allowRemoteSocketChange) {
	// create a UDP socket:
	int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd == -1) {
		printError("init");
		return sd;
	}

	struct sockaddr_in si_me;
	memset(&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind socket to port:
	if( bind(sd, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
		printError("bind");
		close(sd);
		return -2;
	}

	// store socket data:
	if(numSocketsMax==0) {
		numSocketsMax=4;
		sockets = (SocketData*)malloc(numSocketsMax*sizeof(SocketData));
	}
	else if(numSockets == numSocketsMax) {
		numSocketsMax *= 2;
		sockets = (SocketData*)realloc(sockets, numSocketsMax*sizeof(SocketData));
	}
	sockets[numSockets].sd = sd;
	sockets[numSockets].isServer = true;
	sockets[numSockets].allowRemoteSocketChange = allowRemoteSocketChange;
	sockets[numSockets].saddr_len = 0;
	++numSockets;
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

static int socketRecv(int sd, char* buf, uint16_t bufSz) {
	if(sd<0)
		return 0;

	struct sockaddr_in si_other;
	socklen_t slen = sizeof(si_other);
	memset(buf,'\0', bufSz);

	// try to receive some data, this is a blocking call
	int bytesRecv = recvfrom(sd, buf, bufSz, 0, (struct sockaddr*)&si_other, &slen);
	if(bytesRecv<0)
		printError("recvfrom");

	for(uint32_t i=0; i<numSockets; ++i)
		if(sockets[i].sd == sd) {
			if(!sockets[i].saddr_len) {
				sockets[i].saddr = si_other;
				sockets[i].saddr_len = slen;
			}
			else { // check if same, and, if no, trigger callback?
				if(!sockets[i].allowRemoteSocketChange
					&& (slen != sockets[i].saddr_len
					|| sockets[i].saddr.sin_family != si_other.sin_family
					|| sockets[i].saddr.sin_port != si_other.sin_port
					|| sockets[i].saddr.sin_addr.s_addr != si_other.sin_addr.s_addr))
				{
					bytesRecv = -1;
					fprintf(stderr, "socketRead WARNING: remote socket has changed\n");
					break;
				}
				sockets[i].saddr = si_other;
				sockets[i].saddr_len = slen;
			}
			break;
		}

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
	if(sd==0)
		return write(1, buffer, bufSz);
	for(uint32_t i=0; i<numSockets; ++i)
		if(sockets[i].sd == sd) {
			if(sockets[i].saddr_len)
				return sendto(sd, buffer, bufSz, 0, (struct sockaddr*)&sockets[i].saddr, sockets[i].saddr_len);
			break;
		}

	return send(sd, buffer, bufSz, 0);
}

void socketClose(int sd) {
	close(sd);
	for(uint32_t i=0; i<numSockets; ++i) {
		if(sockets[i].sd != sd)
			continue;
		sockets[i].sd = 0;
		sockets[i].saddr_len = 0;
		while(numSockets && sockets[numSockets-1].sd == 0)
			--numSockets;
		break;
	}
}


#ifndef _NO_DUKTAPE

#include "../external/duk_config.h"
#include "../external/duktape.h"

static duk_ret_t try_json_decode(duk_context *ctx, void* udata) {
	(void)(udata);
	duk_json_decode(ctx, -1);
	return 1;
}

static duk_ret_t dk_socketRead(duk_context *ctx) {
	duk_push_this(ctx);
	duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("sd"));
	int sd = duk_get_int(ctx, -1);
	if(sd<0)
		return 0;

	uint16_t msTimeout = duk_is_number(ctx, 0) ? (duk_get_number(ctx, 0)*1000) : 5;
	int mode = 0; // raw
	if(duk_is_number(ctx, 1))
		mode = duk_get_int(ctx, 1);
	else if(duk_is_string(ctx,1)) {
		const char* modeStr = duk_get_string(ctx, 1);
		if(strcmp(modeStr, "string")==0)
			mode = 1;
		else if (strcmp(modeStr, "json")==0 || strcmp(modeStr, "object")==0)
			mode = 2;
	}
	char buf[UINT16_MAX];
	int numBytesRead = socketRead(sd, buf, UINT16_MAX, msTimeout);
	if(numBytesRead<=0)
		return 0;
	if(mode>0) {
		duk_push_lstring(ctx, buf, numBytesRead);
		if(mode == 2 && duk_safe_call(ctx, try_json_decode, NULL, 1 /*nargs*/, 1 /*nrets*/) != DUK_EXEC_SUCCESS) {
			fprintf(stderr, "failed to json parse response [%.*s] len %i", numBytesRead, buf, numBytesRead);
			return 0; // ignore error
		}
	}
	else {
		memcpy(duk_push_fixed_buffer(ctx, numBytesRead), buf, numBytesRead);
		duk_push_buffer_object(ctx, -1, 0, numBytesRead, DUK_BUFOBJ_ARRAYBUFFER);
	}
	return 1;
}

static const char* getMsg(duk_context *ctx, duk_idx_t idx, duk_size_t* len) {
	const char* msg = NULL;
	if(duk_is_buffer_data(ctx,idx))
		msg = duk_get_buffer_data(ctx, idx, len);
	else if(duk_is_object(ctx, idx)) {
		msg = duk_json_encode(ctx, idx);
		*len = duk_get_length(ctx, idx);
	}
	else {
		msg = duk_to_string(ctx, idx);
		*len = duk_get_length(ctx, idx);
	}
	return msg;
}

static duk_ret_t dk_socketWrite(duk_context *ctx) {
	duk_push_this(ctx);
	duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("sd"));
	int sd = duk_get_int(ctx, -1);
	if(sd<0)
		return duk_error(ctx, DUK_ERR_ERROR, "socket not open");

	duk_size_t len;
	const char* msg = getMsg(ctx, 0, &len);
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
		duk_bool_t allowRemoteSocketChange = true;
		if(!host && duk_is_boolean(ctx, 1))
			allowRemoteSocketChange = duk_get_boolean(ctx, 1);
		sd = host ? socketCreateClient(host, port) : socketCreateServer(port, allowRemoteSocketChange);
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

duk_ret_t dk_socketBroadcast(duk_context *ctx) {
	uint16_t port = duk_to_uint16(ctx, 0);
	duk_size_t len;
	const char* msg = getMsg(ctx, 1, &len);
	if(len>UINT16_MAX)
		return duk_error(ctx, DUK_ERR_ERROR, "message too long");
	uint16_t msTimeout = duk_is_number(ctx, 2) ? (duk_get_number(ctx, 2)*1000) : 5;

	//create a UDP socket
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd == -1) {
		printError("init");
		return duk_error(ctx, DUK_ERR_ERROR, "failed to open broadcast socket at port %u", port);
	}
#if defined __WIN32__ || defined WIN32
	char broadcastEnable=1;
#else
	int broadcastEnable=1;
#endif
	if(setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable))!=0) {
		printError("setsockopt broadcast");
		return duk_error(ctx, DUK_ERR_ERROR, "failed to set broadcast socket option");
	}

	struct sockaddr_in si_me;
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	// send message
    if(sendto(sd, msg, len, 0, (struct sockaddr *)&si_me, sizeof(struct sockaddr_in)) < 0) {
        printError("sendto");
		return duk_error(ctx, DUK_ERR_ERROR, "failed to send broadcast message");
	}

	// collect responses:
	char buf[UINT16_MAX];
	memset(buf, 0, UINT16_MAX);
	struct sockaddr_in si_other;
	socklen_t slen = sizeof(si_other);

	duk_idx_t objIdx = duk_push_object(ctx);

	while(socketHasData(sd, msTimeout)) {
		// try to receive some data, this is a blocking call
		int bytesRecv = recvfrom(sd, buf, UINT16_MAX, 0, (struct sockaddr*)&si_other, &slen);
		if(bytesRecv<0) {
			printError("recvfrom");
			return duk_error(ctx, DUK_ERR_ERROR, "receive failed");
		}
		if(bytesRecv < UINT16_MAX)
			buf[bytesRecv] = 0;
		//printf("Received response from %s: %s\n", inet_ntoa(si_other.sin_addr), buf);

		duk_push_lstring(ctx, buf, bytesRecv);
		if(duk_safe_call(ctx, try_json_decode, NULL, 1 /*nargs*/, 1 /*nrets*/) != DUK_EXEC_SUCCESS)
			duk_push_lstring(ctx, buf, bytesRecv);
		duk_put_prop_string(ctx, objIdx, inet_ntoa(si_other.sin_addr));
	}

	close(sd);
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
	duk_push_c_function(ctx, dk_socketRead, 2);
	duk_put_prop_string(ctx, -2, "read");
	duk_push_c_function(ctx, dk_socketWrite, 1);
	duk_put_prop_string(ctx, -2, "write");
	duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("Socket_prototype"));

	duk_push_object(ctx);
	duk_push_c_function(ctx, dk_socketCreate, 2);
	duk_put_prop_string(ctx, -2, "createSocket");
	duk_push_c_function(ctx, dk_socketBroadcast, 3);
	duk_put_prop_string(ctx, -2, "broadcast");
}

void _EXPORT dgram_unload() {
#if defined __WIN32__ || defined WIN32
	WSACleanup();
#endif
	//printf(" dgram..."); fflush(stdout);
}
#endif // _NO_DUKTAPE