# arcajs API

- [app](#module-app)
- [audio](#module-audio)
- [console](#module-console)
- [graphics](#module-gfx)
- [intersects](#module-intersects)
- [sprites](#module-sprite)

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
- {array} [buttons] - key names of the keys to be interpreted as gamepad buttons.

### function app.getResource

returns handle to an image/audio/font or text resource or array of handles

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
- {array} [fillColor=[255,255,255,255]] - fill color (RGBA)
- {number} [strokeWidth=0] - stroke width
- {array} [strokeColor=[0,0,0,0]] - stroke color (RGBA)

#### Returns:

- {number} handle of the created image resource

### function app.createPathResource

creates an image resource from an SVG path description

#### Parameters:

- {number} width - image width
- {number} height - image height
- {string\|array} path - path description
- {array} [fillColor=[255,255,255,255]] - fill color (RGBA)
- {number} [strokeWidth=0] - stroke width
- {array} [strokeColor=[0,0,0,0]] - stroke color (RGBA)

#### Returns:

- {number} handle of the created image resource

### function app.createSVGResource

creates an image resource from an inline SVG string

#### Parameters:

- {string} svg - SVG image description
- {object} [params] - image resource params such as scale factor

#### Returns:

- {number} handle of the created image resource

### function app.createImageResource

creates an RGBA image resource from an buffer

#### Parameters:

- {number} width - image width
- {number} height - image height
- {buffer\|array} data - RGBA 4-byte per pixel image data
- {object} [params] - optional additional parameters as key-value pairs such as filtering

#### Returns:

- {number} handle of the created image resource

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

### function app.setPointer

turns mouse pointer visiblity on or off

#### Parameters:

- {Number} state - visible (1) invisible (0)

### function app.vibrate

vibrates the device, if supported by the platform, likely on mobile browsers only

#### Parameters:

- {Number} duration - duration in seconds

### function app.prompt

reads a string from a modal window

#### Parameters:

- {string\|array} message - (multi-line) message to be displayed
- {string} [initialValue] - optional prefilled value
- {string} [options] - display options

#### Returns:

- {string} entered string

### function app.message

displays a modal message window

#### Parameters:

- {string\|array} message - (multi-line) message to be displayed
- {string} [options] - display options

### function app.close

closes window and application

### function app.httpGet

initiates a HTTP GET request

#### Parameters:

- {string} url - requested URL
- {function} [callback] - function to be called when the response is received

### function app.httpPost

initiates a HTTP POST request sending data to a URL

#### Parameters:

- {string} url - target URL
- {string\|object} data - data to be sent
- {function} [callback] - function to be called when a response is received

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

- {number} font - font resource handle, use 0 for built-in default 10x16 pixel font
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

### Properties:

- {string} app.version - arcajs version
- {string} app.platform - arcajs platform, either 'browser' or 'standalone'
- {number} app.width - window width in logical pixels
- {number} app.height - window height in logical pixels
- {number} app.pixelRatio - ratio physical to logical pixels

## module gfx

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

- {number\|array\|buffer} r - RGB red component in range 0..255 or color array/array buffer
- {number} [g] - RGB green component in range 0..255
- {number} [b] - RGB blue component in range 0..255
- {number} [a=255] - opacity between 0 (invisible) and 255 (opaque)

#### Returns:

- {object} - this gfx object

### function gfx.colorf

sets the current drawing color using normalized floating point values

#### Parameters:

- {number} r - RGB red component in range 0.0..1.0
- {number} g - RGB green component in range 0.0..1.0
- {number} b - RGB blue component in range 0.0..1.0
- {number} [a=255] - opacity between 0.0 (invisible) and 1.0 (opaque)

#### Returns:

- {object} - this gfx object

### function gfx.lineWidth

sets current drawing line width in pixels.

#### Parameters:

- {number} [w] - line width in pixels

#### Returns:

- {object\|number} - this gfx object or current line width, if called without width parameter

### function gfx.blend

sets current drawing blend mode.

#### Parameters:

- {number} [mode] - blend mode, one of the gfx.BLEND_xyz constants

#### Returns:

- {object\|number} - this gfx object or current blend mode, if called without parameter

### function gfx.origin

sets drawing origin

#### Parameters:

- {number} ox - horizontal origin
- {number} oy - vertical origin
- {boolean} [isScreen=true] - flag switching between screen and model space

#### Returns:

- {object} - this gfx object

### function gfx.scale

sets drawing scale

#### Parameters:

- {number} sc - scale

#### Returns:

- {object} - this gfx object

### function gfx.clipRect

sets viewport/clipping rectangle (in screen coordinates) or turns clipping off if called without parameters

#### Parameters:

- {number} [x] - X ordinate
- {number} [y] - Y ordinate
- {number} [w] - width
- {number} [h] - height

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

### function gfx.drawPoints

draws an array of individual points using the current color and line width.

#### Parameters:

- {array\|Float32Array} arr - array of vertex ordinates

### function gfx.drawImage

gfx.drawImage(dstX, dstY[, dstW, dstH])(dstX, dstY[, dstW, dstH])
gfx.drawImage(srcX,srcY, srcW, srcH, dstX, dstY, dstW, dstH[, cX, cY, angle, flip])

draws an image or part of an image at a given target position, optionally scaled

#### Parameters:

- {number} img - image handle
- {number} dstX - destination X position
- {number} dstY - destination Y position
- {number} [destW=srcW] - destination width
- {number} [dstH=srcH] - destination height
- {number} [srcX=0] - source origin X in pixels
- {number} [srcY=0] - source origin Y in pixels
- {number} [srcW=imgW] - source width in pixels
- {number} [srcH=imgH] - source height in pixels
- {number} [cX=0] - rotation center X offset in pixels
- {number} [cY=0] - rotation center Y offset in pixels
- {number} [angle=0] - rotation angle in radians
- {number} [flip=gfx.FLIP_NONE] - flip image in X (gfx.FLIP_X), Y (gfx.FLIP_Y), or in both (gfx.FLIP_XY) directions

### function gfx.fillText

writes text using a specified font.

#### Parameters:

- {number} font - font resource handle, use 0 for built-in default 12x16 pixel font
- {number} x - X ordinate
- {number} y - Y ordinate
- {string} text - text
- {number} [align=gfx.ALIGN_LEFT_TOP] - horizontal and vertical alignment, one of the gfx.ALIGN_xyz constants

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

## module audio

a collection of basic sound synthesis and replay functions

```javascript
var audio = app.require('audio');
```

### function audio.volume

sets or returns master volume, a number between 0.0 and 1.0

#### Parameters:

- {number} [v] - sets master volume

#### Returns:

- {number} the current master volume if called without parameter

### function audio.playing

checks if a track or any track is currently playing

#### Parameters:

- {number} [track] - track ID

#### Returns:

- {boolean} true if the given track (or any track) is playing, otherwise false

### function audio.stop

immediate stops an individual track or all tracks

#### Parameters:

- {number} [track] - track ID

### function audio.replay

immediately plays a buffered PCM sample

#### Parameters:

- {number\|array} sample - sample handle or array of alternative samples (randomly chosen)
- {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
- {number} [detune=0.0] - sample pitch shift in half tones. For example, -12.0 means half replay speed/ one octave less

#### Returns:

- {number} track number playing this sound or UINT_MAX if no track is available

### function audio.sound

immediately plays an FM-generated sound

#### Parameters:

- {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
- {number} freq - frequency in Hz
- {number} duration - duration in seconds
- {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)

#### Returns:

- {number} track number playing this sound or UINT_MAX if no track is available

### function audio.melody

immediately plays an FM-generated melody based on a compact string notation

#### Parameters:

- {string} melody - melody notated as a series of wave form descriptions and notes
- {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
- {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)

#### Returns:

- {number} track number playing this sound or UINT_MAX if no track is available

### function audio.sample

creates an audio sample from an array of floating point numbers

#### Parameters:

- {array\|Float32Array} data - array of PCM sample values in range -1.0..1.0

#### Returns:

- {number} sample handle to be used in audio.replay

### Properties:

- {number} audio.sampleRate - audio device sample rate in Hz

## module Sprite

### function Sprite.setColor

sets the sprite's color possibly blending with its pixel colors

#### Parameters:

- {number\|array\|buffer} r - RGB red component in range 0..255 or combined color array/array buffer
- {number} [g] - RGB green component in range 0..255
- {number} [b] - RGB blue component in range 0..255
- {number} [a=255] - opacity between 0 (invisible) and 255 (opaque)

#### Returns:

- {object} - sprite instance

### function Sprite.getColor

returns the sprite's RGBA color

#### Returns:

- {array} color [r,g,b,a], components in range 0..255

### function Sprite.setAlpha

sets the sprite's opacity

#### Parameters:

- {number} opacity - opacity between 0.0 (invisible) and 1.0 (opaque)

#### Returns:

- {object} - sprite instance

### function Sprite.getX

returns the sprite's X ordinate

#### Returns:

- {number} X ordinate

### function Sprite.getY

returns the sprite's Y ordinate

#### Returns:

- {number} Y ordinate

### function Sprite.setX

sets the sprite's horizontal position

#### Parameters:

- {number} value - X ordinate

#### Returns:

- {object} - sprite instance

### function Sprite.setY

sets the sprite's vertical position

#### Parameters:

- {number} value - y ordinate

### function Sprite.setPos

sets the sprite's position

#### Parameters:

- {number} x - X ordinate
- {number} y - Y ordinate
- {number} [rot] - rotation in radians

#### Returns:

- {object} - sprite instance

### function Sprite.setScale

sets the sprite's output dimensions relative to its source dimensions. Use
negative scales for horizontal/vertical mirroring/flipping.

#### Parameters:

- {number} scaleX - horizontal scale
- {number} [scaleY=scaleX] - vertical scale

#### Returns:

- {object} - sprite instance

### function Sprite.getScaleX

returns the sprite's horizontal scale relative to its source width

#### Returns:

- {number} - horizontal size

### function Sprite.getScaleY

returns the sprite's vertical scale relative to its source height

#### Returns:

- {number} - vertical size

### function Sprite.setDim

sets the sprite's absolute output dimensions

#### Parameters:

- {number} w - width in pixels
- {number} h - height in pixels

#### Returns:

- {object} - sprite instance

### function Sprite.getDimX

returns the sprite's output width

#### Returns:

- {number} output width

### function Sprite.getDimY

returns the sprite's output height

#### Returns:

- {number} output height

### function Sprite.setVel

sets the sprite's velocity

#### Parameters:

- {number} velX - horizontal velocity in pixels per second
- {number} velY - vertical velocity in pixels per second
- {number} [velRot] - rotation velocity in radians per second

#### Returns:

- {object} - sprite instance

### function Sprite.setVelX

sets the sprite's horizontal velocity

#### Parameters:

- {number} velX - horizontal velocity in pixels per second

#### Returns:

- {object} - sprite instance

### function Sprite.setVelY

sets the sprite's vertical velocity

#### Parameters:

- {number} velY - vertical velocity in pixels per second

#### Returns:

- {object} - sprite instance

### function Sprite.getVelX

returns the sprite's horizontal velocity

#### Returns:

- {number} - velocity in pixels per second

### function Sprite.getVelY

returns the sprite's vertical velocity

#### Returns:

- {number} - velocity in pixels per second

### function Sprite.getVelRot

returns the sprite's rotation velocity

#### Returns:

- {number} - rotation velocity in radians per second

### function Sprite.setVelRot

sets the sprite's rotation velocity

#### Parameters:

- {number} rot - rotation velocity in radians per second

#### Returns:

- {object} - sprite instance

### function Sprite.setCenter

sets the sprite's center coordinates

#### Parameters:

- {number} cx - center X position, normalized from 0.0..1.0
- {number} cy - center Y position, normalized from 0.0..1.0

#### Returns:

- {object} - sprite instance

### function Sprite.getCenterX

returns the sprite's center X offset from left border

#### Returns:

- {number} X offset

### function Sprite.getCenterY

returns the sprite's center Y offset from top border

#### Returns:

- {number} Y offset

### function Sprite.getRot

returns the sprite's rotation

#### Returns:

- {number} rotation in radians

### function Sprite.setRot

sets the sprite's rotation

#### Parameters:

- {number} rot - rotation in radians

#### Returns:

- {object} - sprite instance

### function Sprite.setSource

sets the sprite's source dimensions

#### Parameters:

- {number} x - source x origin in pixels
- {number} y - source y origin in pixels
- {number} w - source width in pixels
- {number} h - source height in pixels

#### Returns:

- {object} - sprite instance

### function Sprite.setTile

sets the sprite source tile number, given that the sprite's SpriteSet is tiled

#### Parameters:

- {number} tile - tile number

#### Returns:

- {object} - sprite instance

### function Sprite.getRadius

returns the sprite's collision radius

#### Returns:

- {number} - collision radius, -1.0 if disabled

### function Sprite.setRadius

sets the sprite's collision radius

#### Parameters:

- {number} r - collision radius, use -1.0 to disable

#### Returns:

- {object} - sprite instance

### function Sprite.set

sets the sprite's custom attributes

#### Parameters:

- {object} attributes - custom attributes as key-value pairs

#### Returns:

- {object} - sprite instance

### function Sprite.intersects

tests on intersection/collision with another sprite or a point

The test is either based on radius if set (faster), or on possibly rotated bounding rectangle

#### Parameters:

- {Sprite\|number} arg1 - other sprite to be tested on collision or point X ordinate
- {number} [arg2] - point Y ordinate

#### Returns:

- {boolean} - true in case of intersection, otherwise false

### function SpriteSet.createSprite

creates a new Sprite instance and appends it to this SpriteSet

#### Parameters:

- {number\|boolean} [tile=0\|srcX=0] - tile number for tiled source or source x origin or false for an invisible sprite
- {number} [srcY=0] - source y origin
- {number} [srcW] - source width, default is parent SpriteSet texture width
- {number} [srcH] - source height, default is parent SpriteSet texture height

#### Returns:

- {object} - created sprite instance

### function SpriteSet.update

updates position of contained sprites based on their velocity and passed time

#### Parameters:

- {number} deltaT - time since last update in seconds

### function SpriteSet.removeSprite

removes a sprite from this sprite set

#### Parameters:

- {Sprite} sprite - sprite instance to be removed

### function gfx.drawSprites

draws the contained sprites in sequence of their insertion

#### Parameters:

- {object} spriteset - SpriteSet instance

### function gfx.drawTile

draws a tile of a tiled sprite set

#### Parameters:

- {object} spriteset - SpriteSet instance
- {number} tile - tile number
- {number} x - X ordinate
- {number} y - Y ordinate
- {number} [w] - width
- {number} [h] - height
- {number} [align=gfx.ALIGN_LEFT_TOP] - horizontal and vertical alignment, a combination of the gfx.ALIGN_xyz constants
- {number} [angle=0] - rotation angle in radians
- {number} [flip=gfx.FLIP_NONE] - flip tile in X (gfx.FLIP_X), Y (gfx.FLIP_Y), or in both (gfx.FLIP_XY) directions

### function app.createSpriteSet

creates a new SpriteSet instance

a SpriteSet is a group of graphics sharing the same texture (atlas) that
are drawn and updated together

#### Parameters:

- {number} texture - texture image resource handle
- {number} [tilesX=1] - number for tiles in horizontal direction
- {number} [tilesY=1] - number for tiles in vertical direction
- {number} [border=0] - border width (of each tile) in pixels

#### Returns:

- {object} - created SpriteSet instance

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

### function intersects.arrays

Test if two geometries intersect

Depending on the number of ordinates, the arrays are interpreted either as point (2), circle (3),
axis-aligned rectangle (4), or polygon (6+).

#### Parameters:

- {array\|ArrayBuffer} array1 - first test geometry ordinates
- {array\|ArrayBuffer} array2 - second test geometry ordinates

#### Returns:

- {boolean} true if the two objects intersect

