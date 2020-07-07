# application events

The global app object provides several application events. Callback
functions for them can be registered using the app.on function:

```javascript
app.on('pointer', function(evt) {
    //handle mouse or touch event
});
```

## load event

The load event is triggered when the script code is initially interpreted and
all resource files are loaded and ready to be used.

### Callback function parameters:

- none

## update event

The update event is triggered at frequent regular intervals, current everytime
before the screen in refreshed, usually 60 times per second. It  should be used
to update the inner state of your application based on the progress of time.

### Callback function parameters:

- {number} deltaT - the time passed since the last update event in seconds
- {number} now - the time passed since application start in seconds

## draw event

The draw event is triggered at at the same frequency before the
screen in refreshed, usually 60 times per second, directly after the update
event. Any drawing calls must be initiated here.

### Callback function parameters:

- {object} gfx - the graphics object providing the drawing routines

## resize event

The resize event is triggered when the application window is resized. This can
currently only happen in the arcajs browser runtime.

### Callback function parameters:

- {number} width - new window width in (logical) pixels
- {number} height - new window height in (logical) pixels

## pointer event

The pointer event unifies mouse and touch events. It reports 4 diffeernt types
of events: start, move, end, and hover (the latter mouse only).

### Callback function parameters:

- {object} evt - an event structure combining event type, event x and y position,
  id (that is button or touch id), and pointerType (mouse or touch)

## keyboard event

The keyboard event is triggered when a key is pressed (keydown) or released
(keyup).

### Callback function parameters:

- {object} evt - an event structure containing event type, key name, current
  state of modifier keys and number of repetitions

## gamepad event

The gamepad event is triggered whenever a game controller is connected or
disconnected.

### Callback function parameters:

- {object} evt - contains event type (connect/disconnect) and gamepad index

## close event

The close event is triggered when the application closes.

### Callback function parameters:

- none
