#pragma once
#include <stdint.h>
#include <stdbool.h>

extern bool socketHasData(int sd, unsigned long milliWait);

extern int socketCreateServer(uint16_t port, bool allowRemoteSocketChange);

extern int socketCreateClient(const char* host, uint16_t port);

extern int socketRead(int sd, char* buf, uint16_t bufSz, uint16_t msTimeout);

extern int socketWrite(int sd, const char * buffer, uint16_t bufSz, uint8_t remoteAddrIndex);

extern uint8_t socketLastRemoteAddressIndex(int sd);

extern void socketClose(int sd);
