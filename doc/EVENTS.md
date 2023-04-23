# application events

The global app object provides several application events. Individual callback
functions for them can be registered using the app.on function:

```javascript
app.on('pointer', function(evt) {
    // handle mouse or touch event
});
```

Alternatively, several callback functions can be registered or switched at once
via an event handler object:

```javascript
app.on({
  pointer: function(evt) { /*...*/ },
  keyboard: function(evt) { /*...*/ },
  update: function(deltaT, now) { /*...*/ },
  draw: function(gfx) { /*...*/ },
  // ...
});
```

## load event

The load event is triggered once when the script code is initially interpreted
and all resource files are loaded and ready to be used.

### Callback function parameters:

- none

## enter event

The entered event is triggered after a new event handler object is registered
before the next frame.

### Callback function parameters:

- none

## leave event

The entered event is triggered before switching to a new event handler object.

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

- {object} evt - an event structure combining event type, event x and y
  position, id (that is button or touch id), and pointerType (mouse or touch)

## keyboard event

The keyboard event is triggered when a key is pressed (keydown) or released
(keyup).

### Callback function parameters:

- {object} evt - an event structure containing event type, key name, location,
  current state of modifier keys, and number of repetitions

## textinput event

The experimental text input event is triggered when a key having a printable
ISO-Latin-1 value is pressed. Currently it does not support virtual on-screen
keyboards in the browser runtime.

### Callback function parameters:

- {object} evt - an event structure containing event type, char string value
  for printable ASCII characters, or a key value for non-printable (navigation)
  keys

## textinsert event

The experimental text insert event is triggered when text is dropped or pasted
into the application window.

### Callback function parameters:

- {object} evt - an event structure containing event type ('drop' or 'paste') 
  and data (string)

## gamepad event

The gamepad event is triggered whenever a game controller is connected or
disconnected, or a button or axis significantly changes its value.

### Callback function parameters:

- {object} evt - contains event type (connect/disconnect/axis/button) and
  gamepad index. For axis and button events, also an axis/button index and a
  value is provided. For connect events, also the number of axes and buttons,
  and a device name are reported.

## visibilitychange event

The visibility change event is triggered the application window is hidden or restored.

### Callback function parameters:

- {object} evt - contains boolean property visible (true or false)
 
## close event

The close event is triggered when the application closes.

### Callback function parameters:

- none

## custom event

A custom event may be triggered by calling app.emit('custom'[, params]) .

### Callback function parameters:

- {object} params - contains the passed event parameters
