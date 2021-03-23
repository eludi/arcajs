#include "window.h"
#include "graphics.h"
#include "audio.h"
#include "console.h"
#include "resources.h"
#include "jsBindings.h"
#include "value.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>

const char* appVersion = "v0.20210323a";

static void showError(const char* msg, ...) {
	char formattedMsg[1024];
	va_list argptr;
	va_start(argptr, msg);
	vsnprintf(formattedMsg, 1024, msg, argptr);
	va_end(argptr);
	fprintf(stderr, "%s\n", formattedMsg);

	if(!WindowIsOpen()) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "arcajs Error", formattedMsg, NULL);
		return;
	}
	ConsoleError(formattedMsg);
	Value* options = Value_new(VALUE_MAP, NULL);
	Value_set(options, "title", Value_str("arcajs ERROR"));
	Value_set(options, "background", Value_int(0xaa0055ff));
	Value_set(options, "color", Value_int(0xffFFffFF));
	Value_set(options, "lineBreakAt", Value_int(WindowWidth()/12-4));
	DialogMessageBox(formattedMsg, NULL, options);
	Value_delete(options, 1);
}

static int isModifier(int sym) {
	switch(sym) {
	case SDLK_LALT:
	case SDLK_RALT:
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		return 1;
	}
	return 0;
}

static const char* keySymToKey(int sym) {
	if(sym>='a' && sym<='z') {
		static char s[2] = { 0, 0 };
		s[0] = sym;
		return s;
	}
	switch(sym) {
	case SDLK_LALT:
		return "Alt";
	case SDLK_RALT:
		return "AltGraph";
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		return "Shift";
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		return "Control";
	case SDLK_DOWN:
		return "ArrowDown";
	case SDLK_UP:
		return "ArrowUp";
	case SDLK_LEFT:
		return "ArrowLeft";
	case SDLK_RIGHT:
		return "ArrowRight";
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return "Enter";
	case SDLK_SPACE:
		return " ";
	case SDLK_KP_BACKSPACE:
		return "Backspace";
	case SDLK_KP_TAB:
		return "Tab";
	default:
		return SDL_GetKeyName(sym);
	}
}

static int64_t touchIds[10];
static int touchCount = 0;

static int getTouchId(int64_t finger) {
	for(int i=0; i<touchCount; ++i)
		if(touchIds[i]==finger)
			return i;
	if(touchCount>9)
		return -1;
	touchIds[touchCount] = finger;
	return touchCount++;
}

static void clearTouchId(int touchId) {
	touchIds[touchId] = -1;
	while (touchCount>0 && touchIds[touchCount-1]==-1)
		--touchCount;
}

int handleEvents(void* udata) {
	static unsigned mouseButtons = 0;

	Value* events = (Value*)udata;
	Value_delete(events->child, 1);
	events->child = NULL;

	SDL_Event evt;
	int ret = 0;
	while( SDL_PollEvent( &evt ) && ret == 0 ) switch(evt.type) {
	case SDL_MOUSEMOTION: {
		if(evt.motion.which==SDL_TOUCH_MOUSEID)
			continue;

		Value* event = Value_new(VALUE_MAP, NULL);
		Value_set(event, "evt", Value_str("pointer"));
		Value_set(event, "type", Value_str(mouseButtons ? "move" : "hover"));
		Value_set(event, "x", Value_int(evt.motion.x));
		Value_set(event, "y", Value_int(evt.motion.y));
		Value_set(event, "pointerType", Value_str("mouse"));
		Value_append(events, event);
		break;
	}
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN: {
		if(evt.button.which==SDL_TOUCH_MOUSEID)
			continue;

		int id;
		switch(evt.button.button) {
		case SDL_BUTTON_LEFT: id=0; break;
		case SDL_BUTTON_RIGHT: id=1; break;
		case SDL_BUTTON_MIDDLE: id=2; break;
		default: continue;
		}

		const char* type;
		if(evt.type==SDL_MOUSEBUTTONDOWN) {
			type = "start";
			mouseButtons |= (1<<id);
		}
		else {
			type = "end";
			mouseButtons &= ~(1<<id);
		}

		Value* event = Value_new(VALUE_MAP, NULL);
		Value_set(event, "evt", Value_str("pointer"));
		Value_set(event, "type", Value_str(type));
		Value_set(event, "id", Value_int(id));
		Value_set(event, "x", Value_int(evt.button.x));
		Value_set(event, "y", Value_int(evt.button.y));
		Value_set(event, "pointerType", Value_str("mouse"));
		Value_append(events, event);
		break;
	}
	case SDL_FINGERDOWN:
	case SDL_FINGERMOTION:
	case SDL_FINGERUP: {
		int touchId = getTouchId(evt.tfinger.fingerId);
		if(touchId<0)
			continue;

		float x = evt.tfinger.x * WindowWidth();
		float y = evt.tfinger.y * WindowHeight();

		const char* type;
		switch(evt.type) {
		case SDL_FINGERDOWN: type = "start"; break;
		case SDL_FINGERMOTION: type = "move"; break;
		default:
			type = "end";
			clearTouchId(touchId);
		}

		Value* event = Value_new(VALUE_MAP, NULL);
		Value_set(event, "evt", Value_str("pointer"));
		Value_set(event, "type", Value_str(type));
		Value_set(event, "id", Value_int(touchId));
		Value_set(event, "x", Value_int(x+0.5f));
		Value_set(event, "y", Value_int(y+0.5f));
		Value_set(event, "pointerType", Value_str("touch"));
		Value_append(events, event);
		break;
	}
	case SDL_KEYDOWN:
		if(((evt.key.keysym.sym == SDLK_F4) && (evt.key.keysym.mod & KMOD_LALT))
			|| ((evt.key.keysym.sym == SDLK_q) && (evt.key.keysym.mod & KMOD_CTRL)))
			ret = 1;
		// NO break;
	case SDL_KEYUP: {
		SDL_KeyCode sym = evt.key.keysym.sym;
		const char* key = keySymToKey(sym);
		//if(evt.type==SDL_KEYDOWN) printf("keydown %s\n", key);
		if(key) {
			int isShift = (evt.key.keysym.mod & KMOD_SHIFT);
			Value* event = Value_new(VALUE_MAP, NULL);
			Value_set(event, "evt", Value_str("keyboard"));
			Value_set(event, "type", Value_str(evt.type==SDL_KEYDOWN ? "keydown" : "keyup"));
			Value_set(event, "key", Value_str(key));
			Value_set(event, "ctrlKey", Value_bool(evt.key.keysym.mod & KMOD_CTRL));
			Value_set(event, "altKey", Value_bool(evt.key.keysym.mod & KMOD_ALT));
			Value_set(event, "shiftKey", Value_bool(isShift));
			Value_set(event, "metaKey", Value_bool(evt.key.keysym.mod & KMOD_GUI));
			Value_set(event, "repeat", Value_bool(evt.key.repeat));
			Value_append(events, event);

			if(evt.type==SDL_KEYDOWN && WindowTextInputActive() && !isModifier(sym) && (sym<32||sym>127)) {
				Value* event = Value_new(VALUE_MAP, NULL);
				Value_set(event, "evt", Value_str("textinput"));
				switch((int)sym) {
					case 0xdf: if(!isShift) { Value_set(event, "char", Value_str("\xc3\x9f")); break; }
						Value_delete(event, 1); continue;
					case 0xe4: Value_set(event, "char", Value_str(isShift ? "\xc3\x84" : "\xc3\xa4")); break;
					case 0xf6: Value_set(event, "char", Value_str(isShift ? "\xc3\x96" : "\xc3\xb6")); break;
					case 0xfc: Value_set(event, "char", Value_str(isShift ? "\xc3\x9c" : "\xc3\xbc")); break;
					// if your keyboard generates more codes within ISO 8859-1 Latin-1 range, let me know!
				default:
					Value_set(event, "key", Value_str(key));
				}
				Value_append(events, event);
			}
		}
		break;
	}
	case SDL_JOYDEVICEADDED:
	case SDL_JOYDEVICEREMOVED: {
		if(evt.type==SDL_JOYDEVICEADDED) 
			WindowControllerOpen(evt.jdevice.which);
		else
			WindowControllerClose(evt.jdevice.which);

		Value* event = Value_new(VALUE_MAP, NULL);
		Value_set(event, "evt", Value_str("gamepad"));
		Value_set(event, "type", Value_str(evt.type==SDL_JOYDEVICEADDED ? "connected" : "disconnected"));
		Value_set(event, "index", Value_int(evt.jdevice.which));
		Value_append(events, event);
		break;
	}
	case SDL_TEXTINPUT: {
		const char* input = evt.text.text;
		size_t sz = strlen(input);
		if(sz>1)
			break;

		Value* event = Value_new(VALUE_MAP, NULL);
		Value_set(event, "evt", Value_str("textinput"));
		Value_set(event, "char", Value_str(input));
		Value_append(events, event);
		//printf("textinput %s\n", input);
		break;
	}
	case SDL_QUIT:
		ret = 1;
		break;
	}
	return ret;
}

int evalScriptyByName(size_t vm, const char* fname, const char* archiveName) {
	char* script = ResourceGetText(fname);
	if(!script) {
		showError("Could not find \"%s\" in \"%s\", exiting.\n", fname, archiveName);
		return -1;
	}

	int ret = jsvmEval(vm, script, fname);
	free(script);
	return ret;
}

//--- main ---------------------------------------------------------
int main(int argc, char **argv) {
	const char* scriptName = "main.js";
	char* archiveName = argc>1 ? argv[argc-1] : NULL;
	char* windowTitle = NULL;
	char* storageFileName = NULL;
	char* iconName = NULL;
	int isCalledWithScript = 0;

	int winSzX = 640, winSzY = 480, windowFlags = WINDOW_VSYNC;
	int consoleY=0, consoleSzY = winSzY-32;
	for(int i=1; i<argc; ++i) {
		if(strcmp(argv[i],"-f")==0)
			windowFlags |= WINDOW_FULLSCREEN;
		else if(strcmp(argv[i],"-v")==0 && i+1<argc) {
			if(atoi(argv[i+1]))
				windowFlags |= WINDOW_VSYNC;
			else
				windowFlags &= ~WINDOW_VSYNC;
		}
		else if(strcmp(argv[i],"-w")==0 && i+1<argc)
			winSzX = atoi(argv[i+1]);
		else if(strcmp(argv[i],"-h")==0 && i+1<argc)
			winSzY = atoi(argv[i+1]);
	}

	size_t ar = 0;
	if(!archiveName) {
		ar = ResourceArchiveOpen(argv[0]);
		archiveName = ar ? argv[0] : ".";
	}
	else if(archiveName[strlen(archiveName)-1]=='\\')
		archiveName[strlen(archiveName)-1]=0;
	else if(strcmp(ResourceSuffix(archiveName), "js")==0) {
		isCalledWithScript = 1;
		scriptName = archiveName;
		size_t pos = strlen(scriptName)-1;
		while(pos>0) {
			if(scriptName[pos]=='/' || scriptName[pos]=='\\')
				break;
			--pos;
		}
		if(!pos)
			archiveName=".";
		else {
			archiveName[pos] = 0;
			scriptName = &archiveName[pos+1];
		}
		windowTitle = storageFileName = ResourceBaseName(scriptName);
	}
	if(!ar)
		ar = ResourceArchiveOpen(archiveName);
	if(!ar) {
		showError("Could not open data at \"%s\", exiting.\n", archiveName);
		return -1;
	}

	// load initial settings from manifest:
	char** scriptNames = NULL;
	char* manifest = ResourceGetText("manifest.json");
	unsigned audioFrequency = 44100, audioTracks = 8;
	if(manifest && !isCalledWithScript) {
		size_t json = jsonDecode(manifest);
		windowTitle = jsonGetString(json, "name");
		iconName = jsonGetString(json, "icon");
		winSzX = jsonGetNumber(json, "window_width", winSzX);
		winSzY = jsonGetNumber(json, "window_height", winSzY);
		consoleY = jsonGetNumber(json, "console_y", consoleY);
		consoleSzY = jsonGetNumber(json, "console_height", consoleSzY);
		audioFrequency = jsonGetNumber(json, "audio_frequency", audioFrequency);
		audioTracks = jsonGetNumber(json, "audio_tracks", audioTracks);
		scriptNames = jsonGetStringArray(json, "scripts");

		char* display = jsonGetString(json, "display");
		if(display && strcmp(display, "fullscreen")==0)
			windowFlags |= WINDOW_FULLSCREEN;
		free(display);

		jsonClose(json);
		free(manifest);
		manifest = NULL;
	}

	// initialize window and video:
	if(WindowOpen(winSzX, winSzY, windowFlags)!=0) {
		showError("Setting video mode failed.\n");
		ResourceArchiveClose();
		return -2;
	}
	winSzX = WindowWidth(), winSzY = WindowHeight();
	if(!windowTitle)
		windowTitle = ResourceBaseName(archiveName);
	if(windowTitle && strlen(windowTitle) && windowTitle[0]!='.')
		WindowTitle(windowTitle);
	WindowResizable(0);

	const char* storagePath = SDL_GetPrefPath("eludi", "arcajs");
	if(storageFileName) {
		storageFileName = (char*)malloc(strlen(storagePath)+strlen(windowTitle)+6);
		storageFileName[0]=0;
		strcat(strcat(strcat(storageFileName, storagePath), windowTitle),".json");
	}
	else {
		char* storageBaseName = ResourceBaseName(archiveName);
		storageFileName = (char*)malloc(strlen(storagePath)+strlen(storageBaseName)+6);
		storageFileName[0]=0;
		strcat(strcat(strcat(storageFileName, storagePath), storageBaseName),".json");
		free(storageBaseName);
	}
	SDL_free((void*)storagePath);

	Value* events = Value_new(VALUE_LIST, NULL);
	WindowEventHandler(handleEvents, events);
	gfxInit(WindowRenderer());
	if(consoleSzY)
		ConsoleCreate(0,0,consoleY, winSzX, consoleSzY);

	if(iconName) { // show splash screen:
		size_t icon = ResourceGetImage(iconName, 1.0f, 1);
		if(icon) {
			WindowShowPointer(0);
			int iconW, iconH;
			float textW;
			gfxColorRGBA(255,255,255,255);
			gfxImageDimensions(icon, &iconW, &iconH);
			gfxDrawImage(icon, (winSzX-iconW)/2.0f, (winSzY-iconH)/2.0f);
			gfxMeasureText(0, windowTitle, &textW, NULL, NULL, NULL);
			gfxFillText(0, (winSzX-textW)/2.0f, (winSzY+iconH)/2.0f+8, windowTitle);
		}
	}
	WindowUpdate();
	free(windowTitle);
	windowTitle = NULL;

	AudioOpen(audioFrequency, audioTracks);
	for(size_t i=0, end = WindowNumControllers(); i<end; ++i)
		WindowControllerOpen(i);

	size_t vm = jsvmInit(storageFileName);
	free(storageFileName);
	if(!vm) {
		ResourceArchiveClose();
		return -3;
	}

	// load and execute scripts:
	if(scriptNames) {
		for(char** fname=scriptNames; *fname!=NULL; ++fname) {
			if(evalScriptyByName(vm, *fname, archiveName)!=0) {
				if(jsvmLastError(vm))
					showError("%s", jsvmLastError(vm));
				ResourceArchiveClose();
				return -4;
			}
			free(*fname);
		}
		free(scriptNames);
		scriptNames = NULL;
	}
	else if(evalScriptyByName(vm, scriptName, archiveName)!=0) {
		if(jsvmLastError(vm))
			showError("%s", jsvmLastError(vm));
		ResourceArchiveClose();
		return -5;
	}
	WindowUpdateTimestamp();
	jsvmDispatchEvent(vm, "load", NULL);

	// main loop:
	Value* argUpdate = Value_float(0.0);
	argUpdate->next = Value_float(0.0);
	while(WindowIsOpen()) {
		double now = WindowTimestamp();
		jsvmUpdateEventListeners(vm);
		jsvmAsyncCalls(vm, now);
		argUpdate->f = WindowDeltaT();
		argUpdate->next->f = now;
		jsvmDispatchGamepadEvents(vm);
		jsvmDispatchEvent(vm, "update", argUpdate);
		jsvmDispatchDrawEvent(vm);
		if(consoleSzY)
			ConsoleDraw();
		if(WindowUpdate()!=0) // swap buffers
			break;

		for(Value* evt = events->child; evt!=NULL; evt = evt->next)
			jsvmDispatchEvent(vm, Value_get(evt, "evt")->str, evt);
		if(jsvmLastError(vm)) {
			showError("JavaScript ERROR: %s\n", jsvmLastError(vm));
			break;
		}
	}

	// cleanup:
	jsvmDispatchEvent(vm, "close", NULL);
	printf("Cleaning up..."); fflush(stdout);
	printf(" audio..."); fflush(stdout);
	AudioClose();
	printf(" graphics..."); fflush(stdout);
	gfxClose();
	ConsoleDelete();
	printf(" window..."); fflush(stdout);
	if(WindowIsOpen())
		WindowClose();
	printf(" scripting..."); fflush(stdout);
	Value_delete(argUpdate, 1);
	jsvmClose(vm);
	printf(" resources..."); fflush(stdout);
	ResourceArchiveClose();
	printf(" done.\n");
	return 0;
}
