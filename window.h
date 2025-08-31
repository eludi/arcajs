#pragma once
#include <stdint.h>
#include <stddef.h>

//--- window management API ----------------------------------------
typedef enum {
    WINDOW_FULLSCREEN = (1<<0),
    WINDOW_GL = (1<<1),
    WINDOW_VSYNC = (1<<2),
    WINDOW_RESIZABLE = (1<<3),
    WINDOW_LANDSCAPE = (1<<4),
    WINDOW_PORTRAIT = (1<<5),
} WindowFlags;

/// opens an SDL window
int WindowOpen(int sizeX, int sizeY, WindowFlags windowFlags);
/// returns true in case the window is open
int WindowIsOpen();
/// draws window and swaps buffers
int WindowUpdate();
/// minimizes window
void WindowMinimize();
/// restores window
void WindowRestore();
/// sets window resize policy
void WindowResizable(int isResizable);
/// sets window size, if resizable
int WindowResize(int width, int height);
/// toggles fullscreen
void WindowToggleFullScreen();
/// returns true in case the window is fullscreen
int WindowIsFullscreen();
/// turns mouse pointer display on/off
void WindowShowPointer(int visible);

/// closes window
void WindowClose();
/// initiates a graceful shutdown by emitting a close event to the main event loop
void WindowEmitClose();
/// sets window title
void WindowTitle(const char *str);
/// returns window width
int WindowWidth();
/// returns window height
int WindowHeight();
/// updates internal window dimensions, only necessary if using a custom event handler
void WindowDimensions(int width, int height);
/// returns pixel ratio, for supporting high DPI displays
float WindowPixelRatio();

/// returns timestamp of last frame
double WindowTimestamp();
/// updates window timestamp
void WindowUpdateTimestamp();
/// returns time difference between last frames
double WindowDeltaT();
/// sleeps for at least n seconds
void WindowSleep(double secs);

/// sets clear color
void WindowClearColor(uint32_t color);
/// gets clear color
uint32_t WindowGetClearColor();
/// returns SDL renderer
void* WindowRenderer();

// mouse/keyboard input handling API
/// starts the processing of text input
void WindowTextInputStart();
/// returns user keyboard input text and true in case a string has been finished
int WindowTextInput(char str[256]);
/// stops the processing of text input
void WindowTextInputStop();
/// returns whether text input events are currently processed
int WindowTextInputActive();
/// returns current user input axes (mouse, arrow, pageup/dn keys) and buttons (mouse, F1..12 keys)
void WindowInput(float axis[6], unsigned int button[1]);
/// registers a custom event handler
void WindowEventHandler(int(*eventHandler)(void*), void* udata);
/// allows accessing data of a custom event handler
void* WindowEventData();

/// returns number of available game controllers / gamepads / joysticks
size_t WindowNumControllers();
/// opens the nth controller
/** if not useJoystickApi is specified, the controller attempts to apply the semantic button/axis mapping of SDL2's GameController API*/
int WindowControllerOpen(size_t id, int useJoystickApi);
/// returns the name of the identified controller
const char* WindowControllerName(size_t id);
/// closes a previously opened controller
void WindowControllerClose(size_t id);
/// returns current input of controller n
/** memory of axes is managed by the framework, do not free it yourself. */
int WindowControllerInput(size_t id, int* numAxes, float** axes, int* numButtons, uint32_t* buttons);
/// creates controller events when a button changes its state or an axis changes beyond a given resolution
void WindowControllerEvents(float resolution, void* udata,
	void(*axisCb)(size_t id, uint8_t axis, float value, void* udata),
	void(*btnCb)(size_t id, uint8_t button, float value, void* udata));
