#include "../httpRequest.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL_thread.h>
#include <SDL_timer.h>

// gcc -o httpTest httpTest.c ../httpRequest.c -lcurl OR -lwininet

int SDLCALL TestThread(void *udata) {
	uint32_t delay = *(uint32_t*)udata;
	printf("Sleeping for %u ms... ", delay);
	SDL_Delay(delay);
	printf("waking up after %u ms!\n", delay);
	return 0;
}

//------------------------------------------------------------------

int main(int argc, char** argv) {
	const char* url = argc>1 ? argv[1] : "https://eludi.net/stats";
	const char* urlPost = "https://eludi.net/echo/lot";

	char *protocol, *hostname, *path;
	int port;


    SDL_Thread *thread;

    printf("Simple SDL_CreateThread test:\n");

    /* Simply create a thread */
	uint32_t delay = 1000;
    thread = SDL_CreateThread(TestThread, "TestThread", &delay);

    if (NULL == thread) {
        printf("SDL_CreateThread failed: %s\n", SDL_GetError());
    } else {
	    int threadReturnValue;
        SDL_WaitThread(thread, &threadReturnValue);
        printf("Thread returned value: %d\n", threadReturnValue);
    }


	parseUrl("http://192.168.0.2:8888/servlet/abc?a=b&c=d", &protocol, &hostname, &port, &path);
	printf("{\"protocol\":\"%s\", \"hostname\":\"%s\", \"port\":%d, \"path\":\"%s\"}\n", protocol, hostname, port, path);
	free(protocol); free(hostname); free(path);

	parseUrl(url, &protocol, &hostname, &port, &path);
	printf("{\"protocol\":\"%s\", \"hostname\":\"%s\", \"port\":%d, \"path\":\"%s\"}\n", protocol, hostname, port, path);
	free(protocol); free(hostname); free(path);

	printf("\nHTTP GET %s...", url); fflush(stdout);
	char* resp = NULL;
	int ret = httpGet(url, &resp, 0);
	printf(" -> %i %s\n", ret, resp);
	free(resp);

	printf("\nHTTP POST %s...", urlPost); fflush(stdout);
	char* msg = "hello=world";
	ret = httpPost(urlPost, msg, &resp, 0);
	printf(" -> %i %s\n", ret, resp);
	free(resp);

	printf("\nREADY.\n");
	fflush(stdout);
	return ret;
}
