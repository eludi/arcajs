#include "console.h"
#include "graphics.h"
#include "graphicsUtils.h"
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
	console->bgColor = 0x00000000;
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
		gfxFillText(0, console->x, y, &console->buf[(i+offset)%bufsz]);
		y += console->charH;
	}
}

//------------------------------------------------------------------
#include "window.h"
#include "value.h"
#include "resources.h"
#include <math.h>

static void DialogButton(float pos[4], const char* label, size_t font, uint32_t color, uint32_t background, int selected) {
	gfxColor(color);
	if(!selected)
		gfxDrawRect(pos[0],pos[1],pos[2],pos[3]);
	else{
		gfxFillRect(pos[0],pos[1],pos[2],pos[3]);
		gfxColor(background | 0xff);
	}
	gfxFillTextAlign(font, pos[0]+pos[2]/2, pos[1]+pos[3]/2, label, GFX_ALIGN_CENTER_MIDDLE);
}

static int isInside(float pos[4], float x, float y) {
	return x>=pos[0] && y>=pos[1] && x<pos[0]+pos[2] && y<pos[1]+pos[3];
}

int DialogMessageBox(const char* msg, char* prompt, Value* options) {
	Value* events = (Value*)WindowEventData();
	Value* savedEvents = Value_new(VALUE_LIST, NULL);
	Value_append(savedEvents, events->child);
	events->child = NULL;

	int isTextInput = WindowTextInputActive();
	if(prompt && !isTextInput)
		WindowTextInputStart();

	const char* title = 0;
	size_t font=0, fontTitle=0, icon=0;
	uint32_t bgColor=WindowGetClearColor(), fgColor = 0xeeeeeeff;
	const char* button0 = "OK", *button1 = prompt ? "Cancel" : NULL;
	size_t len=0, lenMax = prompt ? 255 : 0, lineBreakAt=0;
	if(prompt)
		len = strlen(prompt);

	if(options) {
		//Value_print(options, stdout);
		const Value* v = Value_get(options, "font");
		if(v && v->type==VALUE_INT)
			font = v->i;

		v = Value_get(options, "title");
		if(v && v->type==VALUE_STRING)
			title = v->str;
		if(title) {
			v = Value_get(options, "titleFont");
			if(v && v->type==VALUE_INT)
				fontTitle = v->i;
		}

		v = Value_get(options, "color");
		if(v && v->type==VALUE_INT)
			fgColor = v->i;

		v = Value_get(options, "background");
		if(v && v->type==VALUE_INT)
			bgColor = v->i;

		v = Value_get(options, "lineBreakAt");
		if(v && v->type==VALUE_INT)
			lineBreakAt = v->i;

		v = Value_get(options, "icon");
		if(v && v->type==VALUE_INT)
			icon = v->i;

		v = Value_get(options, "button0");
		if(v && v->type==VALUE_STRING)
			button0 = v->str;
		v = Value_get(options, "button1");
		if(v && v->type==VALUE_STRING)
			button1 = v->str;
	}

	int winSzX = WindowWidth(), winSzY = WindowHeight();
	float sz, fontTitleH;
	int iconSzX=0, iconSzY=0;
	gfxMeasureText(font, NULL, NULL, &sz, NULL, NULL);
	gfxMeasureText(fontTitle, NULL, NULL, &fontTitleH, NULL, NULL);
	if(icon)
		gfxImageDimensions(icon, &iconSzX, &iconSzY);

	size_t msglen = strlen(msg);
	char* substr = malloc(msglen+1);
	substr[msglen]=0;

	int ret=0, button0Selected=0, button1Selected=0;
	float button0Pos[] = { 0.75*winSzX, winSzY-2.5*sz, 0.25*winSzX-sz, 1.5*sz };
	float button1Pos[] = { 0.50*winSzX, winSzY-2.5*sz, 0.25*winSzX-sz, 1.5*sz };

	gfxStateReset();

	int done = 0;
	while(WindowIsOpen() && !done) {
		double now = WindowTimestamp();
		// draw:
		gfxColor(bgColor);
		gfxFillRect(0,0, WindowWidth(), winSzY);
		if(icon) {
			gfxColor(0xffffffff);
			gfxDrawImage(icon, winSzX-iconSzX-2*sz, 2*sz, 0,1,0);
		}
		gfxColor(fgColor);
		float y = 2*sz;
		if(title) {
			gfxFillText(fontTitle, 2*sz, y, title);
			y += 2*sz + fontTitleH;
		}

		for(size_t pos=0, count = strcspn(msg,"\n"); pos<msglen && count; pos += count+1, count=strcspn(msg+pos,"\n")) {
			if(count>msglen-pos)
				count = msglen-pos;
			int needsBreak = lineBreakAt>0 && count>lineBreakAt;
			if(needsBreak) {
				size_t lastPos = pos+lineBreakAt-1;
				while(lastPos>pos && msg[lastPos]!=' ' && msg[lastPos]!='\t')
					--lastPos;
				count = (lastPos==pos) ? lineBreakAt : lastPos-pos;

			}
			strncpy(substr, msg+pos, count);
			substr[count]=0;
			//printf("%u %u [%s]\n", (unsigned)pos, (unsigned)count, substr);
			gfxFillText(font, 2*sz, y, substr);
			y += 1.25*sz;
			if(needsBreak && count==lineBreakAt)
				--count;
		}
		if(prompt) {
			gfxFillText(font, 2*sz, y, prompt);
			if(fmod(now, 1.0)>0.33) { // cursor
				float bufW;
				gfxMeasureText(font, prompt, &bufW, NULL, NULL, NULL);
				gfxFillRect(2*sz+bufW, y, sz*0.66, sz);
			}
		}

		if(button0)
			DialogButton(button0Pos, button0, font, fgColor, bgColor, button0Selected);
		if(button1)
			DialogButton(button1Pos, button1, font, fgColor, bgColor, button1Selected);

		if(WindowUpdate()>0) { // swap buffers
			WindowClose();
			break;
		}

		// handle events:
		for(Value* evt = events->child; evt!=NULL; evt = evt->next) {
			if(prompt && strcmp(Value_get(evt, "evt")->str, "textinput")==0) {
				//Value_print(evt, stdout);
				const Value* v = Value_get(evt, "key");
				if(v && v->type==VALUE_STRING) {
					if(strcmp(v->str, "Enter")==0)
						done = 1;
					else if(strcmp(v->str, "Escape")==0) {
						ret = done = 1;
					}
					else if(strcmp(v->str, "Backspace")==0 && len) {
						if((unsigned char)prompt[--len]>127 && len)
							--len;
						prompt[len] = 0;
					}
				}
				v = Value_get(evt, "char");
				if(v && v->str) {
					for(char* pch = v->str; *pch!=0 && len<lenMax; ++pch)
						prompt[len++] = *pch;
					prompt[len] = 0;
				}
			}
			else if(!prompt && strcmp(Value_get(evt, "evt")->str, "keyboard")==0
				&& strcmp(Value_get(evt, "type")->str, "keydown")==0)
			{
				//Value_print(evt, stdout);
				const Value* v = Value_get(evt, "key");
				if(v && v->type==VALUE_STRING) {
					if(strcmp(v->str, "Enter")==0) {
						done = 1;
						ret = button1Selected;
					}
					else if(strcmp(v->str, "Escape")==0)
						ret = done = 1;
				}
			}
			else if(strcmp(Value_get(evt, "evt")->str, "pointer")==0) {
				int x = Value_get(evt, "x")->i, y = Value_get(evt, "y")->i;
				button0Selected = isInside(button0Pos, x, y);
				button1Selected = button0Selected ? 0 : isInside(button1Pos, x, y);
				if((button0Selected || button1Selected) && (strcmp(Value_get(evt, "type")->str, "start")==0)) {
					done = 1;
					ret = button1Selected;
				}
			}
		}
	}
	free(substr);
	if(prompt && !isTextInput)
		WindowTextInputStop();
	events->child = savedEvents->child;
	savedEvents->child = NULL;
	Value_delete(savedEvents, 1);
	return ret;
}
