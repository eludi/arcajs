#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <SDL.h>

#define NUM_JOYSTICKS_MAX 8

typedef struct {
	SDL_Window *window;
	/// pointer to SDL renderer
	SDL_Renderer* renderer;
	/// alternative handle of OpenGL context
	SDL_GLContext context;

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

	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
		fprintf(stderr,"SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
		return 1;
	}

	if(windowFlags & WINDOW_GL) {
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
#ifdef GRAPHICS_API_OPENGL_ES2
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(GRAPHICS_API_OPENGL_33)
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
#endif
		sdlFlags |= SDL_WINDOW_OPENGL;
	}
	if(windowFlags & WINDOW_FULLSCREEN) {
		sdlFlags |= SDL_WINDOW_FULLSCREEN;
		wnd.window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			dm.w*wnd.pixelRatio, dm.h*wnd.pixelRatio, sdlFlags);
		wnd.fullscreen = 1;
	}
	else {
		if(windowFlags & WINDOW_RESIZABLE)
			sdlFlags |= SDL_WINDOW_RESIZABLE;
		wnd.window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			sizeX*wnd.pixelRatio, sizeY*wnd.pixelRatio, sdlFlags);
		wnd.fullscreen = 0;
	}
	if (!wnd.window) {
		fprintf(stderr,"ERROR: could not create window: %s\n", SDL_GetError());
		return 1;
	}
	SDL_SetWindowDisplayMode(wnd.window, &dm);

	//printf("%s ", SDL_GetCurrentVideoDriver());
	SDL_GetWindowSize(wnd.window, &wnd.szX, &wnd.szY);
	if(windowFlags & WINDOW_GL) {
		wnd.context = SDL_GL_CreateContext(wnd.window);
		wnd.renderer = NULL;
		SDL_GL_SetSwapInterval(1);
	}
	else {
		wnd.context = 0;
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		uint32_t renderFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
		if(wnd.vsync)
			renderFlags |= SDL_RENDERER_PRESENTVSYNC;
		wnd.renderer = SDL_CreateRenderer(wnd.window, -1, renderFlags);
		SDL_SetRenderDrawBlendMode(wnd.renderer, SDL_BLENDMODE_BLEND);
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
	else if(wnd.context) {
		SDL_GL_DeleteContext(wnd.context);
		wnd.context = 0;
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
	if(SDL_WasInit(SDL_INIT_JOYSTICK))
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
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
		case SDL_WINDOWEVENT_SIZE_CHANGED:
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
	if(wnd.context)
		SDL_GL_SwapWindow(wnd.window);
	else if(!wnd.renderer)
		return -1;

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

int WindowIsFullscreen() {
	return wnd.fullscreen;
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

void WindowDimensions(int width, int height) {
	wnd.szX = width;
	wnd.szY = height;
}

float WindowPixelRatio() {
	return wnd.pixelRatio;
}

int WindowIsOpen() {
	return wnd.renderer!=0 || wnd.context!=0;
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
}

uint32_t WindowGetClearColor() {
	return wnd.clearColor;
}


typedef struct {
	SDL_Joystick* pJoy;
	SDL_GameController* pGamepad;
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
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER)<0) {
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

int WindowControllerOpen(size_t id, int useJoystickApi) {
	if(id>=NUM_JOYSTICKS_MAX)
		return -1;
	if(!SDL_WasInit(SDL_INIT_JOYSTICK))
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER)<0) {
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
	if(joysticks[id].nButtons > 8*sizeof(uint32_t))
		joysticks[id].nButtons = 8*sizeof(uint32_t);
	joysticks[id].buttons = joysticks[id].buttonsPrev = 0;
	joysticks[id].nAxes = SDL_JoystickNumAxes(pJoy);
	joysticks[id].nHats = SDL_JoystickNumHats(pJoy);
	int nAxesTotal = joysticks[id].nAxes+2*joysticks[id].nHats;
	joysticks[id].axes = (float*)malloc(sizeof(float)*nAxesTotal);
	joysticks[id].axesPrev = (float*)malloc(sizeof(float)*nAxesTotal);

	if(!SDL_IsGameController(id) || useJoystickApi)
		joysticks[id].pGamepad = NULL;
	else {
		joysticks[id].pGamepad = SDL_GameControllerOpen(id);
		if(!joysticks[id].pGamepad) {
			fprintf(stderr,"WindowControllerOpen(%u) ERROR: cannot initialize controller: %s\n", (unsigned)id, SDL_GetError());
			SDL_ClearError();
			return -1;
		}
		if(joysticks[id].nButtons>11)
			joysticks[id].nButtons=11;
		SDL_ClearError();
	}

	//printf("open joystick %u \"%s\" axes:%i hats:%i buttons:%i\n", (unsigned)id,
	//	SDL_JoystickName(pJoy), joysticks[id].nAxes, joysticks[id].nHats, joysticks[id].nButtons);
	return 0;
}

const char * WindowControllerName(size_t id) {
	return SDL_JoystickNameForIndex(id);
}

void WindowControllerClose(size_t id) {
	if(id < NUM_JOYSTICKS_MAX && joysticks[id].pJoy) {
		if(joysticks[id].pGamepad) {
			SDL_GameControllerClose(joysticks[id].pGamepad);
			joysticks[id].pGamepad = NULL;
		}
		SDL_JoystickClose(joysticks[id].pJoy);
		joysticks[id].pJoy = NULL;
		free(joysticks[id].axes);
		free(joysticks[id].axesPrev);
	}
}

static void WindowGamepadState(JoyData* jd, float** axes, uint32_t* buttons) {
	SDL_GameController* gp = jd->pGamepad;
	if(axes) {
		if(jd->nAxes>1 && SDL_GameControllerHasButton(gp, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
			jd->axes[0] = SDL_GameControllerGetButton(gp, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? -1.0f
				: SDL_GameControllerGetButton(gp, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? 1.0f : 0.0f;
			jd->axes[1] = SDL_GameControllerGetButton(gp, SDL_CONTROLLER_BUTTON_DPAD_UP) ? -1.0f
				: SDL_GameControllerGetButton(gp, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ? 1.0f : 0.0f;
		}
		if(jd->nAxes>3 && SDL_GameControllerHasAxis(gp, SDL_CONTROLLER_AXIS_LEFTX)) {
			int16_t value = SDL_GameControllerGetAxis(gp, SDL_CONTROLLER_AXIS_LEFTX);
			jd->axes[2] = value<-32767 ? -1.0f : ((float)value)/32767.0f;
			value = SDL_GameControllerGetAxis(gp, SDL_CONTROLLER_AXIS_LEFTY);
			jd->axes[3] = value<-32767 ? -1.0f : ((float)value)/32767.0f;
		}
		if(jd->nAxes>4 && SDL_GameControllerHasAxis(gp, SDL_CONTROLLER_AXIS_TRIGGERLEFT)) {
			float value = ((float)SDL_GameControllerGetAxis(gp, SDL_CONTROLLER_AXIS_TRIGGERLEFT) - INT16_MAX/2.0f)*2.0f;
			jd->axes[4] = value<-32767.0f ? -1.0f : value/32767.0f;
		}
		if(jd->nAxes>6 && SDL_GameControllerHasAxis(gp, SDL_CONTROLLER_AXIS_RIGHTX)) {
			int16_t value = SDL_GameControllerGetAxis(gp, SDL_CONTROLLER_AXIS_RIGHTX);
			jd->axes[5] = value<-32767 ? -1.0f : ((float)value)/32767.0f;
			value = SDL_GameControllerGetAxis(gp, SDL_CONTROLLER_AXIS_RIGHTY);
			jd->axes[6] = value<-32767 ? -1.0f : ((float)value)/32767.0f;
		}
		if(jd->nAxes>7 && SDL_GameControllerHasAxis(gp, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)) {
			float value = ((float)SDL_GameControllerGetAxis(gp, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) - INT16_MAX/2.0f)*2.0f;
			jd->axes[7] = value<-32767.0f ? -1.0f : value/32767.0f;
		}
		*axes = jd->axes;
	}

	if(buttons) {
		*buttons=0;
		static int btn[] = {
			SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
			SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
			SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_GUIDE,
			SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK
		};
		for(int i=0; i<jd->nButtons && i<11; ++i)
			if(SDL_GameControllerHasButton(gp, btn[i]) && SDL_GameControllerGetButton(gp, btn[i]))
				*buttons |= (1<<i);
		jd->buttons = *buttons;
	}
}

static void WindowControllerState(JoyData* jd, float** axes, uint32_t* buttons) {
	if(jd->pGamepad) {
		WindowGamepadState(jd, axes, buttons);
		return;
	}
	if(axes) {
		for(int i=0; i<jd->nHats; ++i) {
			uint8_t hat=SDL_JoystickGetHat(jd->pJoy, i);
			jd->axes[2*i] = (hat&SDL_HAT_LEFT) ? -1.0f : (hat&SDL_HAT_RIGHT) ? 1.0f : 0.0f;
			jd->axes[2*i+1] = (hat&SDL_HAT_UP) ? -1.0f : (hat&SDL_HAT_DOWN)  ? 1.0f : 0.0f;
		}
		for(int i=0; i<jd->nAxes; ++i) {
			int16_t value = SDL_JoystickGetAxis(jd->pJoy,i);
			jd->axes[2*jd->nHats + i] = value<-32767 ? -1.0f : ((float)value)/32767.0f;
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
	void(*btnCb)(size_t id, uint8_t button, float value, void* udata))
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
				float value = round(axes[i]/resolution);
				if(value<resolution && value>-resolution)
					value = axes[i] = 0.0f;
				float valuePrev = round(jd->axesPrev[i]/resolution);
				if(value!=valuePrev)
					(*axisCb)(id, i, axes[i], udata);
			}
		}

		if(btnCb && jd->buttons!=jd->buttonsPrev) {
			uint32_t bitmask = 1;
			for(uint8_t i=0; i<jd->nButtons; ++i, bitmask<<=1) {
				uint32_t btn = jd->buttons & bitmask;
				uint32_t btnPrev = jd->buttonsPrev & bitmask;
				if(btn<btnPrev)
					(*btnCb)(id, i, 0.0f, udata);
				else if(btn>btnPrev)
					(*btnCb)(id, i, 1.0f, udata);
			}
		}
		jd->buttonsPrev = jd->buttons;
		memcpy(jd->axesPrev, axes, sizeof(float)*nAxesTotal);
	}
}
