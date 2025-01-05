# arcajs API

- [app](#module-app)
- [audio](#module-audio)
- [console](#module-console)
- [graphics](#module-graphics)
- [intersects](#module-intersects)

## module console

console input output

### function console.visible

sets or gets console visibility

#### Parameters:

- {boolean} [isVisible] - new console visibility

#### Returns:

- {boolean\|undefined} current console visibility if called without argument

## module app

the single global entry point to the arcajs API. Always available.

### function app.on

registers or removes an event listener callback for an application event.
The individual application events are described in [EVENTS.md](EVENTS.md).

#### Parameters:

- {string\|object} name - event name or object consisting of name:eventHandler function pairs
- {function\|null} callback - function to be executed when the event has happened, set null to remove

### function app.emit

emits an event

#### Parameters:

- {string} name - event name
- {any} [args] - an arbitrary number of additional arguments to be passed to the event handler

### function app.emitAsGamepadEvent

re-emits configurable keyboard events as gamepad events

This helper function allows games to use a unified input handler by translating
configurable keyboard key events to gamepad events.

```javascript
app.on('keyboard', function(evt) {
    // create a virtual gamepad by interpreting
    // WASD as a virtual directional pad and Enter key as primary button:
    app.emitAsGamepadEvent(evt, 0, ['a','d', 'w','s'], ['Enter']);
});
```

#### Parameters:

- {object} keyboardEvent - the keyboard event to be translated and re-emitted
- {number} index - the index of the gamepad
- {array} axes - key names of the keys to be interpreted as gamepad axes. Each pair of keys define an axis.
- {array} [buttons] - key names of the keys to be interpreted as gamepad buttons or objects {key:'keyName', location:index}.

### function app.getResource

returns handle to an image/audio/font or text/json resource or array of handles

#### Parameters:

- {string\|array} name - resource file name or list of resource file names
- {object} [params] - optional additional parameters as key-value pairs such as

  filtering for images, scale for SVG images, or size for font resources

#### Returns:

- {number\|array} resource handle(s)

### function app.createCircleResource

creates a circle image resource

#### Parameters:

- {number} radius - circle radius
- {array\|number} [fillColor=[255,255,255,255]] - fill color (RGBA)
- {number} [strokeWidth=0] - stroke width
- {array\|number} [strokeColor=[0,0,0,0]] - stroke color (RGBA)

#### Returns:

- {number} handle of the created image resource

### function app.createPathResource

creates an image resource from an SVG path description

#### Parameters:

- {number} width - image width
- {number} height - image height
- {string\|array} path - path description
- {array\|number} [fillColor=[255,255,255,255]] - fill color (RGBA)
- {number} [strokeWidth=0] - stroke width
- {array\|number} [strokeColor=[0,0,0,0]] - stroke color (RGBA)

#### Returns:

- {number} handle of the created image resource

### function app.createTileResources

creates tiled image resources based on an existing image resource

#### Parameters:

- {number\|string} parent - image resource handle or name
- {number} tilesX - number of tiles in horizontal direction
- {number} [tilesY=1] - number of tiles in vertical direction
- {number} [border=0] - border around tiles in pixels
- {object} [params] - optional additional parameters as key-value pairs such as filtering or scale. Only effective if parent is a resource file name.

#### Returns:

- {number} handle of the first created tile image resource

### function app.createTileResource

creates image resource based on an existing image resource

#### Parameters:

- {number} parent - image resource handle
- {number} x - relative new image horizontal origin (0.0..1.0)
- {number} y - relative new image vertical origin (0.0..1.0)
- {number} w - relative new image width (0.0..1.0)
- {number} h - relative new image height (0.0..1.0)
- {object} [params] - additional optional parameters: centerX, centerY

#### Returns:

- {number} handle of the created image resource

### function app.createImageFontResource

creates a font based on a texture containing a fixed 16x16 grid of glyphs

#### Parameters:

- {number\|string} img - image resource handle
- {object} [params] - additional optional parameters: border, scale

#### Returns:

- {number} handle of the created font resource

### function app.setImageCenter

sets image origin and rotation center relative to upper left (0\|0) and lower right (1.0\|1.0) corners

#### Parameters:

- {number} img - image resource handle
- {number} cx - relative horizontal image center
- {number} cy - relative vertical image center

### function app.createSVGResource

creates an image resource from an inline SVG string

#### Parameters:

- {string} svg - SVG image description
- {object} [params] - image resource params such as scale factor

#### Returns:

- {number} handle of the created image resource

### function app.createImageResource

creates an image resource from a buffer or from a callback function

#### Parameters:

- {number\|object} width - image width or an object having width, height, depth, and data properties
- {number} [height] - image height
- {buffer\|array\|number\|string} [data\|cb] - RGBA 4-byte per pixel image data or background color if image shall be created via a callback function
- {object\|function} [params] - optional additional parameters as key-value pairs such as filtering  or callback function having a graphics context as parameter

#### Returns:

- {number} handle of the created image resource

### function app.releaseResource

releases a previously uploaded image, audio, or font resource

#### Parameters:

- {number} handle - resource handle
- {string} mediaType - mediaType, either 'image', 'audio', or 'font'

### function app.setBackground

sets window background color

#### Parameters:

- {number\|array\|buffer} r - RGB red component in range 0-255 or color array / array buffer
- {number} [g] - RGB green component in range 0-255
- {number} [b] - RGB blue component in range 0-255

### function app.setTitle

sets window title

#### Parameters:

- {string} title - new title

### function app.resizable

sets window resizability

#### Parameters:

- {bool} isResizable

### function app.fullscreen

toggles window fullscreen or returns fullscreen state

#### Parameters:

- {bool} [fullscreen]

#### Returns:

- {bool} current fullscreen state, if called without parameter

### function app.minimize

minimizes or restores an application window

#### Parameters:

- {bool} minimized

### function app.transformArray

transforms a Float32Array by applying a function on all groups of members

#### Parameters:

- {buffer} arr - Float32Array to be transformed
- {number} stride - number of elements of a single logical record
- {any} [param] - zero or more fixed parameters to be passed to the callback function
- {function} callback - function transforming a single logical record at once, signature function(input, output[, param0,...])

### function app.setPointer

turns mouse pointer visiblity on or off

#### Parameters:

- {Number} state - visible (1) invisible (0)

### function app.vibrate

vibrates the device, if supported by the platform, likely on mobile browsers only

#### Parameters:

- {Number} duration - duration in seconds

### function app.prompt

reads a string from a modal window or popup overlay

#### Parameters:

- {string\|array} message - (multi-line) message to be displayed
- {string} [initialValue] - optional prefilled value
- {object} [options] - display options: font, title, titleFont, color, background, lineBreakAt, icon, button0, button1

#### Returns:

- {string} entered string

### function app.message

displays a modal message window or popup overlay

#### Parameters:

- {string\|array} message - (multi-line) message to be displayed
- {string} [options] - display options: font, title, titleFont, color, background, lineBreakAt, icon, button0, button1

#### Returns:

- {number} index of pressed button

### function app.close

closes window and application

### function app.httpGet

initiates a HTTP GET request

#### Parameters:

- {string} url - requested URL
- {function} [callback] - function to be called when the response is received. The first argument contains the received data, the second argument is the http response status code.

### function app.httpPost

initiates a HTTP POST request sending data to a URL

#### Parameters:

- {string} url - target URL
- {string\|object} data - data to be sent
- {function} [callback] - function to be called when a response is received. The first argument contains the received data, the second argument is the http response status code.

### function app.openURL

opens a URL in a (new) browser window

#### Parameters:

- {string} url - target URL

### function app.parse

parses an XML or (X)HTML, or JSON string as Javascript object

#### Parameters:

- {string} url - target URL

### function app.include

loads JavaScript code from one or more other javascript source files

#### Parameters:

- {string} filename - file name

### function app.require

loads a module written in C or JavaScript (registered via app.exports)

#### Parameters:

- {string} name - module file name without suffix

#### Returns:

- {object\|function} loaded module

### function app.exports

exports a JavaScript module that can be accessed via app.require

#### Parameters:

- {string} id - module identifier to be used by require
- {object\|function} module - exported module

### function app.queryFont

measures text dimensions using a specified font.

#### Parameters:

- {number} font - font resource handle, use 0 for built-in default 12x16 font
- {string} [text] - optional text for width calculation

#### Returns:

- {object} an object having the properties width, height, fontBoundingBoxAscent, fontBoundingBoxDescent

### function app.queryImage

measures image dimensions.

#### Parameters:

- {number} img - image resource handle

#### Returns:

- {object} an object having the properties width and height

### function app.hsl

converts a HSL color defined by hue, saturation, lightness, and optionally opacity to a single RGB color number.

#### Parameters:

- {number} h - hue, value range 0.0..360.0
- {number} s - saturation, value range 0.0..1.0
- {number} l - lightness, value range 0.0..1.0
- {number} [a=1.0] - opacity, value between 0.0 (invisible) and 1.0 (opaque)

#### Returns:

- {number} - RGBA color value

### function app.cssColor

converts any CSS color string to a single RGB(A) color number.

#### Parameters:

- {number\|string} color - color string or number

#### Returns:

- {number} - RGBA color value

### function app.createColorArray

creates an Uint32Array of colors having an appropriate native format for color arrays

#### Parameters:

- {number}[, {number}...] colors - color values as numbers in format #RRGGBBAA (e.g., #00FF00FF for opaque green)

#### Returns:

- {Uint32Array} - color values

### function app.arrayColor

returns a single color number having the appropriate (reverse) byte order for color arrays. May be used for writing or reading individual values of color Uint32Arrays

#### Parameters:

- {number} color - color value as number in format #RRGGBBAA (e.g., #00FF00FF for opaque green)

#### Returns:

- {number} - color value in appropriate byte order for color arrays

### function app.log

writes an info message to application log

#### Parameters:

- {any} value - one or more values to write

### function app.warn

writes a warning message to application log

#### Parameters:

- {any} value - one or more values to write

### function app.error

writes an error message to application log

#### Parameters:

- {any} value - one or more values to write

### Properties:

- {array} app.args - script-relevant command line arguments (or URL parameters), to be passed after a -- as separator as key value pairs, keys start with a -- or -
- {string} app.version - arcajs version
- {string} app.platform - arcajs platform, either 'browser' or 'standalone'
- {string} app.arch - operating system name and architecture, for example Linux_x86_64
- {int} app.numControllers - number of currently connected game controllers
- {number} app.width - window width in logical pixels
- {number} app.height - window height in logical pixels
- {number} app.pixelRatio - ratio physical to logical pixels

## module audio

a collection of basic sound synthesis and replay functions

```javascript
var audio = app.require('audio');
```

### function audio.volume

sets or returns master volume or volume of a currently playing track

#### Parameters:

- {number} [track] - track ID
- {number} [v] - new volume, a number between 0.0 and 1.0

#### Returns:

- {number} the current master volume if called without arguments

### function audio.playing

checks if a track or any track is currently playing

#### Parameters:

- {number} [track] - track ID

#### Returns:

- {boolean} true if the given track (or any track) is playing, otherwise false

### function audio.stop

immediately stops an individual track or all tracks

#### Parameters:

- {number} [track] - track ID

### function audio.suspend

(temporarily) suspends all audio output

### function audio.resume

resumes previously suspended audio output

### function audio.fadeOut

linearly fades out a currently playing track

#### Parameters:

- {number} track - track ID
- {number} deltaT - time from now in seconds until silence

### function audio.replay

immediately plays a buffered PCM sample

#### Parameters:

- {number\|array} sample - sample handle or array of alternative samples (randomly chosen)
- {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
- {number} [detune=0.0] - sample pitch shift in half tones. For example, -12.0 means half replay speed/ one octave less

#### Returns:

- {number} track number playing this sound or UINT_MAX if no track is available

### function audio.loop

immediately and repeatedly plays a buffered PCM sample

#### Parameters:

- {number\|array} sample - sample handle or array of alternative samples (randomly chosen)
- {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
- {number} [detune=0.0] - sample pitch shift in half tones. For example, -12.0 means half replay speed/ one octave less

#### Returns:

- {number} track number playing this sound or UINT_MAX if no track is available

### function audio.sound

immediately plays an oscillator-generated sound

#### Parameters:

- {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
- {number\|string} freq - frequency in Hz or note in form of F#2, A4
- {number} duration - duration in seconds
- {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)

#### Returns:

- {number} track number playing this sound or UINT_MAX if no track is available

### function audio.createSound

creates a complex oscillator-generated sound

#### Parameters:

- {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
- {number\|string} - one or more control points consisting of frequency/time interval/volume/shape

#### Returns:

- {number} a handle identifying this sound for later replay

### function audio.createSoundBuffer

creates a complex oscillator-generated sound buffer

#### Parameters:

- {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
- {number\|string} - one or more control points consisting of frequency/time interval/volume/shape

#### Returns:

- {Float32Array} a PCM sound buffer

### function audio.melody

immediately plays an FM-generated melody based on a compact string notation

#### Parameters:

- {string} melody - melody notated as a series of wave form descriptions and notes
- {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)

#### Returns:

- {number} track number playing this sound or UINT_MAX if no track is available

### function audio.uploadPCM

uploads PCM data from an array of floating point numbers and returns a handle for later playback

#### Parameters:

- {array\|Float32Array\|object} data - array of PCM sample values in range -1.0..1.0, or an object having data, channels, and offset attributes
- {number} [channels=1] - number of channels, 1=mono, 2=stereo

#### Returns:

- {number} sample handle to be used in audio

### function audio.note2freq

translates a musical note (e.g., A4 , Bb5 C#3) to the corresponding frequency

#### Parameters:

- {string\|number} note - musical note pitch as string or as numeric frequency

#### Returns:

- {number} - corresponding frequency

### function audio.sampleBuffer

provides access to a sample's buffer

#### Parameters:

- {number} sample - sample handle

#### Returns:

- {Float32Array} - float32 buffer object containing the PCM samples

### function audio.clampBuffer

clamps a sample buffer' value range to given minimum and maximum values

#### Parameters:

- {Float32Array} buffer - sample buffer to be truncated
- {number} [minValue=-1.0] - minimum value
- {number} [maxValue=+1.0] - maximum value

### function audio.mixToBuffer

mixes a mono PCM sample to an existing stereo buffer. Both need to have the same sample
rate as the current audio device.

#### Parameters:

- {Float32Array} target - target stereo buffer
- {number\|Float32Array} source - source mono sample or buffer
- {number} [startTime=0.0] - start time offset
- {number} [volume=1.0] - maximum value
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)

### Properties:

- {number} audio.sampleRate - audio device sample rate in Hz
- {number} audio.tracks - number of parallel audio tracks

## module graphics

drawing functions, only available within the  draw event callback function

```javascript
app.on('draw', function(gfx) {
    gfx.color(255, 0, 0);
    gfx.fillRect(50, 50, 200, 100);
    //...
});
```

### function gfx.color

sets the current drawing color

#### Parameters:

- {number} r - RGB red component in range 0..255 or unified uint32 RGBA color
- {number} [g] - RGB green component in range 0..255 or opacity if first argument is a unified uint32 RGBA color
- {number} [b] - RGB blue component in range 0..255
- {number} [a=255] - opacity between 0 (invisible) and 255 (opaque)

#### Returns:

- {object} - this gfx object

### function gfx.lineWidth

sets or returns current drawing line width in pixels.

#### Parameters:

- {number} [w] - line width in pixels

#### Returns:

- {object\|number} - this gfx object or line width in pixels if called without arguments

### function gfx.blend

sets current drawing blend mode.

#### Parameters:

- {number} [mode] - blend mode, one of the gfx.BLEND_xyz constants

#### Returns:

- {object\|number} - this gfx object or current blend mode, if called without parameter

### function gfx.clipRect

sets viewport/clipping rectangle (in screen coordinates) or turns clipping off if called without parameters or with false as sole parameter

#### Parameters:

- {number\|boolean} [x] - X ordinate or false for turning clipping off
- {number} [y] - Y ordinate
- {number} [w] - width
- {number} [h] - height

### function gfx.transform

sets the current transformation

#### Parameters:

- {number\|object} x - horizontal translation or an object having {x:0,y:0,z:0,rotX:0,rotY:0,rotZ:0,sc:1.0} members of type number
- {number} [y] - vertical translation
- {number} [rot=0] - rotation angle in radians
- {number} [sc=1] - scale factor

#### Returns:

- {object} this gfx object for chained calls

### function gfx.save

saves current rendering state, can be nested up to 7 times

#### Returns:

- {object} this gfx object for chained calls

### function gfx.restore

restores the last stored rendering state

#### Returns:

- {object} this gfx object for chained calls

### function gfx.reset

restores the initial rendering state, pops also all stored states

#### Returns:

- {object} this gfx object for chained calls

### function gfx.drawRect

draws a rectangular boundary line identified by a left upper coordinate, width, and height.

#### Parameters:

- {number} x - X ordinate
- {number} y - Y ordinate
- {number} w - width
- {number} h - height

### function gfx.fillRect

fills a rectangular screen area identified by a left upper coordinate, width, and height.

#### Parameters:

- {number} x - X ordinate
- {number} y - Y ordinate
- {number} w - width
- {number} h - height

### function gfx.drawLine

draws a line between two coordinates.

#### Parameters:

- {number} x1 - X ordinate first point
- {number} y1 - Y ordinate first point
- {number} x2 - X ordinate second point
- {number} y2 - Y ordinate second point

### function gfx.drawLineStrip

draws a series of connected lines using the current color and line width.

#### Parameters:

- {array\|Float32Array} arr - array of vertex ordinates

### function gfx.drawLineLoop

draws a closed series of connected lines using the current color and line width.

#### Parameters:

- {array\|Float32Array} arr - array of vertex ordinates

### function gfx.drawPoints

draws a series points using an optionally defined image as point sprite and the current line width.

#### Parameters:

- {array\|Float32Array} arr - array of vertex ordinates
- {number} [img=gfx.IMG_CIRCLE] - point sprite image handle

### function gfx.fillTriangle

draws a single filled triangle using the current color.

#### Parameters:

- {number} x1 - X ordinate first point
- {number} y1 - Y ordinate first point
- {number} x2 - X ordinate second point
- {number} y2 - Y ordinate second point
- {number} x3 - X ordinate third point
- {number} y3 - Y ordinate third point

### function gfx.fillTriangles

draws filled triangles.

#### Parameters:

- {array\|Float32Array} arr - array of vertex ordinates
- {array\|Uint32Array} [colors] - optional array of vertex colors

### function gfx.drawImage

gfx.drawImage(img[,x, y, angle, scale, flip])

draws an image at a given target position, optionally scaled and flipped

#### Parameters:

- {number} img - image handle
- {number} [x=0] - destination X position
- {number} [y=0] - destination Y position
- {number} [angle=0] - rotation angle in radians
- {number} [scale=1] - scale factor
- {number} [flip=gfx.FLIP_NONE] - flip image in X (gfx.FLIP_X), Y (gfx.FLIP_Y), or in both (gfx.FLIP_XY) directions

### function gfx.stretchImage

gfx.stretchImage(img,x1,y1,w,h)

draws a stretched image controlled by a corner and width and height

#### Parameters:

- {number} img - image handle
- {number} x1 - X ordinate upper left corner
- {number} y1 - Y ordinate upper left corner
- {number} w - width
- {number} h - height

### function gfx.fillText

writes text using a specified font.

#### Parameters:

- {number} x - X ordinate
- {number} y - Y ordinate
- {string} text - text
- {number} [font=0] - font handle
- {number} [align=gfx.ALIGN_LEFT_TOP] - horizontal and vertical alignment, one of the gfx.ALIGN_xyz constants

### function gfx.drawTiles

draws a rectangular grid of multiple tiled images

#### Parameters:

- {number} imgBase - base image handle
- {number} tilesX - number of tiles in horizontal direction
- {number} tilesY - number of tiles in vertical direction
- {Uint32Array} imgOffsets - array of image handle offsets
- {Uint32Array} [colors] - optional tile color array
- {number} [stride=tilesX] - number of array elements to proceed to next row

### function gfx.drawImages

draws multiple images based on array data

#### Parameters:

- {number} imgBase - base image handle
- {number} stride - number of array elements to proceed to next image data record
- {number} components - components contained in array, a bitwise combination of gfx.COMP_xyz constants
- {Float32Array} arr - array containing at least the positions of the images to be drawn and optionally further components

### function gfx.drawSprite

draws a sprite object based on various object attributes

#### Parameters:

- {object} s - sprite object. The following attributes are interpreted:

- {number} image - image handle
- {number} x,y,[rot=0],[sc=1] - position
- {number} [color=0xFFffFFff] - color and opacity
- {number} [flip=0] - horizontal (1) and vertical (2) flip flags

### Constants:

- {number} gfx.ALIGN_LEFT
- {number} gfx.ALIGN_CENTER
- {number} gfx.ALIGN_RIGHT
- {number} gfx.ALIGN_TOP
- {number} gfx.ALIGN_MIDDLE
- {number} gfx.ALIGN_BOTTOM
- {number} gfx.ALIGN_LEFT_TOP
- {number} gfx.ALIGN_CENTER_TOP
- {number} gfx.ALIGN_RIGHT_TOP
- {number} gfx.ALIGN_LEFT_MIDDLE
- {number} gfx.ALIGN_CENTER_MIDDLE
- {number} gfx.ALIGN_RIGHT_MIDDLE
- {number} gfx.ALIGN_LEFT_BOTTOM
- {number} gfx.ALIGN_CENTER_BOTTOM
- {number} gfx.ALIGN_RIGHT_BOTTOM
- {number} gfx.FLIP_NONE
- {number} gfx.FLIP_X
- {number} gfx.FLIP_Y
- {number} gfx.FLIP_XY
- {number} gfx.BLEND_NONE
- {number} gfx.BLEND_ALPHA
- {number} gfx.BLEND_ADD
- {number} gfx.BLEND_MOD
- {number} gfx.BLEND_MUL
- {number} gfx.IMG_CIRCLE
- {number} gfx.IMG_SQUARE
- {number} gfx.COMP_IMG_OFFSET
- {number} gfx.COMP_ROT
- {number} gfx.COMP_SCALE
- {number} gfx.COMP_COLOR_R
- {number} gfx.COMP_COLOR_G
- {number} gfx.COMP_COLOR_B
- {number} gfx.COMP_COLOR_A
- {number} gfx.COMP_COLOR_RGB
- {number} gfx.COMP_COLOR_RGBA

## module intersects

a collection of intersection/collision test functions

```javascript
var intersects = app.require('intersects');
```

### function intersects.pointPoint

Test if two points have the same position

#### Parameters:

- {number} x1 - test point1 X ordinate
- {number} y1 - test point1 Y ordinate
- {number} x2 - test point2 X ordinate
- {number} y2 - test point2 Y ordinate

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.pointCircle

Test if a points lies within a circle

#### Parameters:

- {number} x - test point X ordinate
- {number} y - test point Y ordinate
- {number} cx - circle center X ordinate
- {number} cy - circle center Y ordinate
- {number} r - circle radius

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.pointAlignedRect

Test if a points lies within an axis-aligned rectangle

#### Parameters:

- {number} x - test point X ordinate
- {number} y - test point Y ordinate
- {number} x1 - rectangle minimum X ordinate
- {number} y1 - rectangle minimum Y ordinate
- {number} x2 - rectangle maximum X ordinate
- {number} y2 - rectangle maximum Y ordinate

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.pointPolygon

Test if point(x,y) is within a convex polygon

Based on https://stackoverflow.com/a/34689268 .

#### Parameters:

- {number} x - test point X ordinate
- {number} y - test point Y ordinate
- {array\|ArrayBuffer} polygon - polygon ordinates

#### Returns:

- {boolean} true if point is within polygon

### function intersects.circleCircle

Test if two circles intersect

#### Parameters:

- {number} x1 - circle 1 center X ordinate
- {number} y1 - circle 1 center Y ordinate
- {number} r1 - circle 1 radius
- {number} x2 - circle 2 center X ordinate
- {number} y2 - circle 2 center Y ordinate
- {number} r2 - circle 2 radius

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.circleAlignedRect

Test if a circle and an axis-aligned rectangle intersect

#### Parameters:

- {number} x - circle center X ordinate
- {number} y - circle center Y ordinate
- {number} r - circle radius
- {number} x1 - rectangle minimum X ordinate
- {number} y1 - rectangle minimum Y ordinate
- {number} x2 - rectangle maximum X ordinate
- {number} y2 - rectangle maximum Y ordinate

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.circlePolygon

Test if a circle and a convex polygon intersect

#### Parameters:

- {number} x - circle center X ordinate
- {number} y - circle center Y ordinate
- {number} r - circle radius
- {array\|ArrayBuffer} polygon - polygon ordinates

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.circleTriangles

Test if a circle and a list of optionally transformed triangles intersect

#### Parameters:

- {number} cx - circle center X ordinate
- {number} cy - circle center Y ordinate
- {number} cr - circle radius
- {array\|ArrayBuffer} triangles - triangle ordinates
- {number} [tx=0] - triangles x translation
- {number} [ty=0] - triangles y translation
- {number} [trot=0] - triangles rotation

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.alignedRectAlignedRect

Test if two axis-aligned rectangles intersect

#### Parameters:

- {number} x1 - rectangle 1 minimum X ordinate
- {number} y1 - rectangle 1 minimum Y ordinate
- {number} x2 - rectangle 1 maximum X ordinate
- {number} y2 - rectangle 1 maximum Y ordinate
- {number} x3 - rectangle 2 minimum X ordinate
- {number} y3 - rectangle 2 minimum Y ordinate
- {number} x4 - rectangle 2 maximum X ordinate
- {number} y4 - rectangle 2 maximum Y ordinate

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.alignedRectPolygon

Test if an axis-aligned rectangle and a convex polygon intersect

#### Parameters:

- {number} x1 - rectangle minimum X ordinate
- {number} y1 - rectangle minimum Y ordinate
- {number} x2 - rectangle maximum X ordinate
- {number} y2 - rectangle maximum Y ordinate
- {array\|ArrayBuffer} polygon - polygon ordinates

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.polygonPolygon

Test if two convex polygons intersect

#### Parameters:

- {array\|ArrayBuffer} polygon1 - polygon 1 ordinates
- {array\|ArrayBuffer} polygon2 - polygon 2 ordinates

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.polygonTriangles

Test if a convex polygon and a triangle list intersect, both optionally transformed

#### Parameters:

- {array\|ArrayBuffer} polygon - polygon ordinates
- {number} x1 - polygon x translation
- {number} y1 - polygon y translation
- {number} rot1 - polygon rotation
- {array\|ArrayBuffer} triangles - triangle ordinates
- {number} [x2=0] - triangles x translation
- {number} [y2=0] - triangles y translation
- {number} [rot2=0] - triangles rotation

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.trianglesTriangles

Test if two triangle lists intersect, both optionally transformed

#### Parameters:

- {array\|ArrayBuffer} tr1 - first triangle ordinates
- {number} x1 - first triangles x translation
- {number} y1 - first triangles y translation
- {number} rot1 - first triangles rotation
- {array\|ArrayBuffer} second tr2 - second triangle ordinates
- {number} [x2=0] - second triangles x translation
- {number} [y2=0] - second triangles y translation
- {number} [rot2=0] - triangles rotation

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.sprites

Test if two sprite objects intersect

The test is based on the following object attributes:
- position (x,y,rot) currently NO scale
- bounding radius (radius) or rectangle (w,h) with optional center (cx,cy)
- optionally convex hull (shape) or triangle list (triangles)

#### Parameters:

- {object} s1 - first sprite
- {object} s2 - second sprite

#### Returns:

- {boolean} true if the two objects intersect

### function intersects.arrays

Test if two geometries intersect

Depending on the number of ordinates, the arrays are interpreted either as point (2), circle (3),
axis-aligned rectangle (4), or polygon (6+).

#### Parameters:

- {array\|ArrayBuffer} array1 - first test geometry ordinates
- {array\|ArrayBuffer} array2 - second test geometry ordinates

#### Returns:

- {boolean} true if the two objects intersect

## module Worker

implements a minimal subset of the Web Workers API

Calls supported by the main context are the Worker constructor and its postMessage and onmessage methods.

The worker itself may communicate with the main context via the postMessage() function and an onmessage callback.
In addition, it may import additional javascript sources via the importScripts() function.
Apart from that, only few selected APIs are accessible by arcajs workers: console, setTimeout, clearTimeout.

For further details please refer to the [Web Workers API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API)
documentation at MDN.
