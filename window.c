#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <SDL.h>
#include <SDL_opengl.h>

#define NUM_JOYSTICKS_MAX 8

#ifndef _NO_GL
extern void gfxGlClearRGB(unsigned char r, unsigned char g, unsigned char b);
extern void gfxGlFlush();
#endif

typedef struct {
	SDL_Window *window;
	/// pointer to SDL renderer
	SDL_Renderer* renderer;
	/// OpenGL context
	SDL_GLContext glctx;

	/// window size x
	int szX;
	/// window size y
	int szY;
	/// window fullscreen mode
	char fullscreen;
	/// vertical sync on or off
	char vsync;
	/// clear color
	uint32_t clearColor;
	/// pixel ratio, for supporting high DPI displays
	float pixelRatio;

	/// timestamp of current frame
	double timestamp;
	/// time difference between last frames
	double deltaT;
	/// stores current input text
	char inputText[256];
	/// stores current input text length
	unsigned char inputTextSz;
	/// flag indicating a finished input text event
	unsigned char inputTextComplete;
	/// input axes
	float inputAxis[6];
	/// input buttons
	unsigned int inputButtons;
	/// custom event handler
	int (*eventHandler)(void*);
	/// custom event handler user data
	void* eventHandlerUserData;
} Window;

static Window wnd;

//--- window handling --------------------------------------------

static float getPixelRatio(int displayIndex) {
	const float sysDefaultDpi =
#ifdef __APPLE__
		72.0f;
#else
		96.0f;
#endif
	float dpi;
	if(SDL_GetDisplayDPI(displayIndex, NULL, &dpi, NULL) != 0)
		return 1.0f;
	return dpi/sysDefaultDpi;
}

int WindowOpen(int sizeX, int sizeY, WindowFlags windowFlags) {
	wnd.szX = wnd.szY = 0;
	wnd.renderer = 0;
	wnd.glctx = 0;

	WindowClearColor(0);
	wnd.inputTextSz = wnd.inputTextComplete = 0;
	wnd.inputButtons = 0;
	wnd.inputAxis[0]=wnd.inputAxis[1]=wnd.inputAxis[2]=wnd.inputAxis[3]=wnd.inputAxis[4]=wnd.inputAxis[5]=0.0f;
	wnd.eventHandler = NULL;
	wnd.eventHandlerUserData = NULL;
	wnd.vsync = (windowFlags & WINDOW_VSYNC) ? 1 : 0;
	wnd.pixelRatio = getPixelRatio(0);

	if(SDL_Init(SDL_INIT_VIDEO)!=0) {
		fprintf(stderr,"ERROR: cannot initialize SDL video: %s\n",SDL_GetError());
		return 1;
	}
	int32_t sdlFlags = SDL_WINDOW_ALLOW_HIGHDPI;
	const char* title = "arcajs";

	if(windowFlags & WINDOW_GL) {
		sdlFlags |= SDL_WINDOW_OPENGL;
#ifdef GRAPHICS_API_OPENGL_ES2
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#elif defined(GRAPHICS_API_OPENGL_33)
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
	}

	if(windowFlags & WINDOW_FULLSCREEN) {
		SDL_DisplayMode dm;
		if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
			fprintf(stderr,"SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
			return 1;
		}
		sdlFlags |= SDL_WINDOW_FULLSCREEN;
		wnd.window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			dm.w*wnd.pixelRatio, dm.h*wnd.pixelRatio, sdlFlags);
		//SDL_SetRelativeMouseMode(SDL_TRUE);
		wnd.fullscreen = 1;
	}
	else {
		wnd.window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			sizeX*wnd.pixelRatio, sizeY*wnd.pixelRatio, sdlFlags);
		wnd.fullscreen = 0;
	}
	if (!wnd.window) {
		fprintf(stderr,"ERROR: could not create window: %s\n", SDL_GetError());
		return 1;
	}

	//printf("%s ", SDL_GetCurrentVideoDriver());
	SDL_GetWindowSize(wnd.window, &wnd.szX, &wnd.szY);
	if(!(sdlFlags & SDL_WINDOW_OPENGL)) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		uint32_t renderFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
		if(wnd.vsync)
			renderFlags |= SDL_RENDERER_PRESENTVSYNC;
		wnd.renderer = SDL_CreateRenderer(wnd.window, -1, renderFlags);
		SDL_SetRenderDrawBlendMode(wnd.renderer, SDL_BLENDMODE_BLEND);

		//SDL_RendererInfo renderInfo;
		//if(SDL_GetRendererInfo(wnd.renderer, &renderInfo)==0)
		//	printf("%s %u ",renderInfo.name, renderInfo.flags);
	}
	else {
		wnd.glctx = SDL_GL_CreateContext(wnd.window);
		if(wnd.vsync)
			SDL_GL_SetSwapInterval(1);
	}

	SDL_StopTextInput();
	WindowUpdateTimestamp();
	return 0;
}

void* WindowRenderer() {
	return wnd.renderer;
}

void WindowClose() {
	if(wnd.renderer) {
		SDL_DestroyRenderer(wnd.renderer);
		wnd.renderer = NULL;
	}
	if(wnd.glctx) {
		SDL_GL_DeleteContext(wnd.glctx);
		wnd.glctx = 0;
	}
	if(!wnd.window)
		return;
	SDL_DestroyWindow(wnd.window);
	if(SDL_WasInit(SDL_INIT_VIDEO))
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	wnd.window = NULL;
	wnd.szX=0;
	wnd.szY=0;
	for(size_t i=0; i<NUM_JOYSTICKS_MAX; ++i)
		WindowControllerClose(i);
	SDL_Quit();
}

static int WindowHandleEvents() {
	if(wnd.inputTextComplete) {
		wnd.inputText[0]=0;
		wnd.inputTextSz=0;
		wnd.inputTextComplete = 0;
	}

	wnd.inputAxis[5]=0.0f; // reset mouse wheel axis
	SDL_Event evt;
	while( SDL_PollEvent( &evt ) ) switch(evt.type) {
	case SDL_KEYDOWN:
		switch(evt.key.keysym.sym) {
		case SDLK_RETURN:
		case SDLK_KP_ENTER: wnd.inputTextComplete=1; break;
		case SDLK_BACKSPACE: if(wnd.inputTextSz) wnd.inputText[--wnd.inputTextSz]=0; break;
		case SDLK_LEFT: wnd.inputAxis[0]=-1.0f; break;
		case SDLK_RIGHT: wnd.inputAxis[0]=1.0f; break;
		case SDLK_UP: wnd.inputAxis[1]=1.0f; break;
		case SDLK_DOWN: wnd.inputAxis[1]=-1.0f; break;
		case SDLK_PAGEUP: wnd.inputAxis[2]=1.0f; break;
		case SDLK_PAGEDOWN: wnd.inputAxis[2]=-1.0f; break;
		case SDLK_TAB: break;
		case SDLK_ESCAPE: wnd.inputButtons |= (1<<27); break;
		case SDLK_F1: wnd.inputButtons |= (1<<0); break;
		case SDLK_F2: wnd.inputButtons |= (1<<1); break;
		case SDLK_F3: wnd.inputButtons |= (1<<2); break;
		case SDLK_F4: wnd.inputButtons |= (1<<3); break;
		case SDLK_F5: wnd.inputButtons |= (1<<4); break;
		case SDLK_F6: wnd.inputButtons |= (1<<5); break;
		case SDLK_F7: wnd.inputButtons |= (1<<6); break;
		case SDLK_F8: wnd.inputButtons |= (1<<7); break;
		case SDLK_F9: wnd.inputButtons |= (1<<8); break;
		case SDLK_F10: wnd.inputButtons |= (1<<9); break;
		case SDLK_F11: wnd.inputButtons |= (1<<10); break;
		case SDLK_F12: wnd.inputButtons |= (1<<11); break;
		default:
			break;
		}
		break;
	case SDL_KEYUP:
		switch(evt.key.keysym.sym) {
		case SDLK_LEFT: wnd.inputAxis[0]=0.0f; break;
		case SDLK_RIGHT: wnd.inputAxis[0]=0.0f; break;
		case SDLK_UP: wnd.inputAxis[1]=0.0f; break;
		case SDLK_DOWN: wnd.inputAxis[1]=0.0f; break;
		case SDLK_PAGEUP: wnd.inputAxis[2]=0.0f; break;
		case SDLK_PAGEDOWN: wnd.inputAxis[2]=0.0f; break;
		case SDLK_F1: wnd.inputButtons &= ~(1<<0); break;
		case SDLK_F2: wnd.inputButtons &= ~(1<<1); break;
		case SDLK_F3: wnd.inputButtons &= ~(1<<2); break;
		case SDLK_F4: wnd.inputButtons &= ~(1<<3);
			if(evt.key.keysym.mod & KMOD_LALT) {
				WindowClose();
				return 1;
			}
			break;
		case SDLK_F5: wnd.inputButtons &= ~(1<<4); break;
		case SDLK_F6: wnd.inputButtons &= ~(1<<5); break;
		case SDLK_F7: wnd.inputButtons &= ~(1<<6); break;
		case SDLK_F8: wnd.inputButtons &= ~(1<<7); break;
		case SDLK_F9: wnd.inputButtons &= ~(1<<8); break;
		case SDLK_F10: wnd.inputButtons &= ~(1<<9); break;
		case SDLK_F11: wnd.inputButtons &= ~(1<<10); break;
		case SDLK_F12: wnd.inputButtons &= ~(1<<11); break;
		default:
			break;
		}
		break;
	case SDL_MOUSEMOTION:
		wnd.inputAxis[3]= ((float)evt.motion.x/(float)wnd.szX-0.5f)*2.0f;
		wnd.inputAxis[4]=-((float)evt.motion.y/(float)wnd.szY-0.5f)*2.0f;
		break;
	case SDL_MOUSEBUTTONDOWN:
		switch(evt.button.button) {
		case SDL_BUTTON_LEFT: wnd.inputButtons |= (1<<0); break;
		case SDL_BUTTON_RIGHT: wnd.inputButtons |= (1<<1); break;
		case SDL_BUTTON_MIDDLE: wnd.inputButtons |= (1<<2); break;
		default: break;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		switch(evt.button.button) {
		case SDL_BUTTON_LEFT: wnd.inputButtons &= ~(1<<0); break;
		case SDL_BUTTON_RIGHT: wnd.inputButtons &= ~(1<<1); break;
		case SDL_BUTTON_MIDDLE: wnd.inputButtons &= ~(1<<2); break;
		default: break;
		}
		break;
	case SDL_TEXTINPUT: {
		const char* input = evt.text.text;
		size_t sz = strlen(input);

		if(wnd.inputTextSz+sz>254)
			break;
		strcat(&wnd.inputText[wnd.inputTextSz], input);
		wnd.inputTextSz += sz;
		wnd.inputText[wnd.inputTextSz] = 0;
		break;
	}
	case SDL_WINDOWEVENT:
		switch(evt.window.event) {
		case SDL_WINDOWEVENT_RESIZED:
			wnd.szX = evt.window.data1;
			wnd.szY = evt.window.data2;
			break;
		}
		break;
	case SDL_QUIT:
		WindowClose();
		return 1;
	}
	return 0;
}

int WindowUpdate() {
	if(wnd.renderer) {
		SDL_RenderPresent(wnd.renderer);
		SDL_SetRenderDrawColor(wnd.renderer, wnd.clearColor >> 24, wnd.clearColor >> 16, wnd.clearColor >> 8,
			SDL_ALPHA_OPAQUE);
		SDL_RenderClear(wnd.renderer);
	}
#ifndef _NO_GL
	else if(wnd.glctx) {
		gfxGlFlush();
		SDL_GL_SwapWindow(wnd.window);
		GLenum gl_error = glGetError();
		if( gl_error != GL_NO_ERROR ) {
			fprintf(stderr, "WindowUpdate OpenGL ERROR: %i\n", gl_error);
			return -1;
		}
		gfxGlClearRGB(wnd.clearColor >> 24, wnd.clearColor >> 16, wnd.clearColor >> 8);
	}
#endif
	else return -1;

	const char* sdl_error = SDL_GetError();
	if(*sdl_error) {
		int ignore = strncmp(sdl_error, "ERROR: NumPoints = 0", 20)==0;
		fprintf(stderr, "WindowUpdate SDL ERROR%s: %s\n", (ignore ? " ignored" : ""), sdl_error);
		SDL_ClearError();
		if(!ignore)
			return -1;
	}
	int ret = wnd.eventHandler ? (*wnd.eventHandler)(wnd.eventHandlerUserData) : WindowHandleEvents();
	if(ret)
		return ret;

	double tPrev = wnd.timestamp;
	if(wnd.vsync)
		SDL_Delay(1);
	wnd.timestamp = (double)SDL_GetTicks()/1000.0;
	wnd.deltaT = wnd.timestamp-tPrev;
	return 0;
}

void WindowMinimize() {
	SDL_MinimizeWindow(wnd.window);
}

void WindowRestore() {
	SDL_RestoreWindow(wnd.window);
}

void WindowResizable(int isResizable) {
	SDL_SetWindowResizable(wnd.window, (SDL_bool)isResizable);
}

void WindowToggleFullScreen() {
	wnd.fullscreen=!wnd.fullscreen;
	SDL_SetWindowFullscreen(wnd.window, wnd.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
}

void WindowShowPointer(int visible) {
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

void WindowTextInputStart() {
	SDL_StartTextInput();
	wnd.inputTextComplete = 0;
}

void WindowTextInputStop() {
	SDL_StopTextInput();
	wnd.inputTextComplete = 1;
}

int WindowTextInput(char str[256]) {
	strncpy(str,wnd.inputText,wnd.inputTextSz+1);
	return wnd.inputTextComplete;
}

int WindowTextInputActive() {
	return SDL_IsTextInputActive();
}

void WindowEventHandler(int(*eventHandler)(), void* udata) {
	wnd.eventHandler = eventHandler;
	wnd.eventHandlerUserData = udata;
}

void* WindowEventData() {
	return wnd.eventHandlerUserData;
}

void WindowInput(float axis[6], unsigned int button[1]) {
	*button = wnd.inputButtons;
	memcpy(axis,wnd.inputAxis,6*sizeof(float));
}

int WindowWidth() {
	return wnd.szX;
}

int WindowHeight() {
	return wnd.szY;
}

float WindowPixelRatio() {
	return wnd.pixelRatio;
}

int WindowIsOpen() {
	return wnd.renderer!=0 || wnd.glctx;
}

void WindowTitle(const char* str) {
	SDL_SetWindowTitle(wnd.window, str);
}

double WindowTimestamp() {
	return wnd.timestamp;
}

void WindowUpdateTimestamp() {
	double tPrev = wnd.timestamp;
	wnd.timestamp = (double)SDL_GetTicks()/1000.0;
	wnd.deltaT = wnd.timestamp-tPrev;
}

double WindowDeltaT() {
	return wnd.deltaT;
}

void WindowSleep(double secs) {
	SDL_Delay(secs*1000);
}

void WindowClearColor(uint32_t color) {
	wnd.clearColor = color;
	if(wnd.renderer) {
		SDL_SetRenderDrawColor(wnd.renderer, wnd.clearColor >> 24, wnd.clearColor >> 16, wnd.clearColor >> 8,
			SDL_ALPHA_OPAQUE);
		SDL_RenderClear(wnd.renderer);
	}
#ifndef _NO_GL
	else if(wnd.glctx)
		gfxGlClearRGB(wnd.clearColor >> 24, wnd.clearColor >> 16, wnd.clearColor >> 8);
#endif
}

uint32_t WindowGetClearColor() {
	return wnd.clearColor;
}


typedef struct {
	SDL_Joystick* pJoy;
	int nButtons;
	int nAxes;
	int nHats;
	float* axes;
	float* axesPrev;
	uint32_t buttons;
	uint32_t buttonsPrev;
} JoyData;

static JoyData joysticks[NUM_JOYSTICKS_MAX];

size_t WindowNumControllers() {
	if(!SDL_WasInit(SDL_INIT_JOYSTICK)) {
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)<0) {
			fprintf(stderr, "WindowNumControllers ERROR: cannot initialize SDL joystick.\n");
			SDL_ClearError();
			return 0;
		}
		memset(joysticks, 0, sizeof(JoyData)*NUM_JOYSTICKS_MAX);
	}
	size_t n = (size_t)SDL_NumJoysticks();
	SDL_ClearError();
	return n < NUM_JOYSTICKS_MAX ? n : NUM_JOYSTICKS_MAX;
}

int WindowControllerOpen(size_t id) {
	if(id>=NUM_JOYSTICKS_MAX)
		return -1;
	if(!SDL_WasInit(SDL_INIT_JOYSTICK))
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)<0) {
			fprintf(stderr, "WindowControllerOpen(%u) ERROR: cannot initialize SDL joystick.\n", (unsigned)id);
			return -1;
		}
	SDL_Joystick* pJoy = SDL_JoystickOpen(id);
	if(!pJoy) {
		fprintf(stderr,"WindowControllerOpen(%u) ERROR: cannot initialize controller: %s\n", (unsigned)id, SDL_GetError());
		SDL_ClearError();
		return -1;
	}
	joysticks[id].pJoy = pJoy;
	joysticks[id].nButtons = SDL_JoystickNumButtons(pJoy);
	if(joysticks[id].nButtons>=sizeof(uint32_t))
		joysticks[id].nButtons = sizeof(uint32_t);
	joysticks[id].buttons = joysticks[id].buttonsPrev = 0;
	joysticks[id].nAxes = SDL_JoystickNumAxes(pJoy);
	joysticks[id].nHats = SDL_JoystickNumHats(pJoy);
	int nAxesTotal = joysticks[id].nAxes+2*joysticks[id].nHats;
	joysticks[id].axes = (float*)malloc(sizeof(float)*nAxesTotal);
	joysticks[id].axesPrev = (float*)malloc(sizeof(float)*nAxesTotal);

	//printf("open joystick %u \"%s\" axes:%i hats:%i buttons:%i\n", (unsigned)id,
	//	SDL_JoystickName(pJoy), joysticks[id].nAxes, joysticks[id].nHats, joysticks[id].nButtons);
	return 0;
}

void WindowControllerClose(size_t id) {
	if(id < NUM_JOYSTICKS_MAX && joysticks[id].pJoy) {
		SDL_JoystickClose(joysticks[id].pJoy);
		joysticks[id].pJoy = NULL;
		free(joysticks[id].axes);
		free(joysticks[id].axesPrev);
	}
}

static void WindowControllerState(JoyData* jd, float** axes, uint32_t* buttons) {
	if(axes) {
		for(int i=0; i<jd->nAxes; ++i) {
			int16_t value = SDL_JoystickGetAxis(jd->pJoy,i);
			jd->axes[i] = value<-32767 ? -1.0f : ((float)value)/32767.0f;
		}
		for(int i=0; i<jd->nHats; ++i) {
			uint8_t hat=SDL_JoystickGetHat(jd->pJoy, i);
			jd->axes[jd->nAxes+2*i] = (hat&SDL_HAT_LEFT) ? -1.0f :  (hat&SDL_HAT_RIGHT) ? 1.0f : 0.0f;
			jd->axes[jd->nAxes+2*i+1] = (hat&SDL_HAT_DOWN) ? -1.0f :  (hat&SDL_HAT_UP) ? 1.0f : 0.0f;
		}
		*axes = jd->axes;
	}

	if(buttons) {
		*buttons=0;
		for(int i=0; i<jd->nButtons; ++i)
			if(SDL_JoystickGetButton(jd->pJoy, i))
				*buttons |= (1<<i);
		jd->buttons = *buttons;
	}
}

int WindowControllerInput(size_t id, int* numAxes, float** axes, int* numButtons, uint32_t* buttons) {
	JoyData* jd = (id < NUM_JOYSTICKS_MAX) ? &joysticks[id] : NULL;
	if(!jd || !jd->pJoy)
		return -1;

	SDL_JoystickUpdate();
	WindowControllerState(jd, axes, buttons);
	if(numAxes)
		*numAxes = jd->nAxes + 2*jd->nHats;
	if(numButtons)
		*numButtons = jd->nButtons;
	return 0;
}

void WindowControllerEvents(float resolution, void* udata,
	void(*axisCb)(size_t id, uint8_t axis, float value, void* udata),
	void(*btnCb)(size_t id, uint8_t button, uint8_t value, void* udata))
{
	if(resolution<=0.0f || resolution>1.0f)
		return;

	SDL_JoystickUpdate();

	float* axes;
	uint32_t buttons;

	for(size_t id=0; id<NUM_JOYSTICKS_MAX; ++id) {
		JoyData* jd = &joysticks[id];
		if(!jd->pJoy)
			continue;

		WindowControllerState(jd, axisCb ? &axes : NULL, btnCb ? &buttons : NULL);

		int nAxesTotal = jd->nAxes + 2*jd->nHats;
		if(axisCb) {
			for(uint8_t i=0; i<nAxesTotal; ++i) {
				float value = round(jd->axes[i]/resolution);
				float valuePrev = round(jd->axesPrev[i]/resolution);
				if(value!=valuePrev)
					(*axisCb)(id, i, jd->axes[i], udata);
			}
		}

		if(btnCb && jd->buttons!=jd->buttonsPrev) {
			uint32_t bitmask = 1;
			for(uint8_t i=0; i<jd->nButtons; ++i, bitmask<<=1) {
				uint32_t btn = jd->buttons & bitmask;
				uint32_t btnPrev = jd->buttonsPrev & bitmask;
				if(btn<btnPrev)
					(*btnCb)(id, i, 0, udata);
				else if(btn>btnPrev)
					(*btnCb)(id, i, 1, udata);
			}
		}
		jd->buttonsPrev = jd->buttons;
		memcpy(jd->axesPrev, jd->axes, sizeof(float)*nAxesTotal);
	}
}
