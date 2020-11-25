#include "console.h"
#include "graphics.h"
#include "graphicsGL.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/// structure for managing a console
typedef struct {
	/// font resource handle
	size_t font;
	/// x position ordinate
	float x;
	/// y position ordinate
	float y;
	/// dimensions
	float w, h;
	/// text color
	uint32_t fgColor;
	/// background color
	uint32_t bgColor;
	/// buffer dimensions
	uint16_t bufW, bufH;
	/// single character dimensions
	uint16_t charW, charH;
	/// line buffer
	char* buf;
	/// total line counter
	size_t lines;
	/// visibility
	uint8_t visible;
} Console;

static Console* console = NULL;

void ConsoleCreate(size_t font, float x, float y, float w, float h) {
	if(console)
		ConsoleDelete();
	console = (Console*)malloc(sizeof(Console));
	memset(console, 0, sizeof(Console));
	console->font = font;
	console->x = x;
	console->y = y;
	console->w = w;
	console->h = h;
	console->fgColor = 0xffffffff;
	console->bgColor = 0x00000055;
	console->charW = 12;
	console->charH = 16;
	console->bufW = w/console->charW+1;
	console->bufH = h/console->charH;
	char* buf = console->buf
		= (char*)malloc(sizeof(char) * console->bufH * console->bufW);
	memset(buf, 0, console->bufH * console->bufW);
}

void ConsoleDelete() {
	if(!console)
		return;
	free(console->buf);
	free(console);
	console = NULL;
}

void ConsoleShow() {
	if(console)
		console->visible = 1;
}

void ConsoleHide() {
	if(console)
		console->visible = 0;
}

int ConsoleVisible() {
	return console && console->visible;
}

void ConsoleLog(const char* msg) {
	if(!console) {
		printf("%s\n", msg);
		fflush(stdout);
		return;
	}
	size_t len = strlen(msg);
	if(len>=console->bufW)
		len = console->bufW-1;
	size_t pos = (console->lines++ * console->bufW) % (console->bufW*console->bufH);
	strncpy(&console->buf[pos], msg, len);
	console->buf[pos+len] = 0;
}

void ConsoleWarn(const char* msg) {
	ConsoleLog(msg);
}

void ConsoleError(const char* msg) {
	ConsoleShow();
	ConsoleLog(msg);
}

void ConsoleDraw() {
	if(!console || !console->visible)
		return;
	gfxColor(console->bgColor);
	gfxFillRect(console->x, console->y, console->w, console->h);
	gfxColor(console->fgColor);
	float y = console->y;
	const size_t bufsz = console->bufH * console->bufW;
	for(size_t i=0, end=bufsz; i<end; i+=console->bufW) {
		size_t offset = 0;
		if(console->lines > console->bufH)
			offset = (console->lines % console->bufH) * console->bufW;
		gfxFillText(console->font, console->x, y, &console->buf[(i+offset)%bufsz]);
		y += console->charH;
	}
}

void ConsoleDraw_gl() {
	if(!console || !console->visible)
		return;
	gfxGlColor(console->bgColor);
	gfxGlFillRect(console->x, console->y, console->w, console->h);
	gfxGlColor(console->fgColor);
	float y = console->y;
	for(size_t i=0, end=console->bufH * console->bufW; i<end; i+=console->bufW) {
		gfxGlFillText(console->font, console->x, y, &console->buf[i]);
		y += console->charH;
	}
}
