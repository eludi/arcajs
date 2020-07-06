# arcajs

arcajs is a simple and lightweight cross-platform JavaScript 2D multimedia/game
framework that runs both within a browser and on its own compact and optimized
stand-alone runtime environment. This unique combination enables you to write
portable games and multimedia apps that efficiently run on a wide range of
modern devices, from low-end single board computers (such as the Raspberry Pi
Zero) over smartphones and tablets up to powerful gaming PCs.

## Components

Both the arcajs web app environment and the stand-alone executable offer the
same Javascript API, no additional HTML or CSS code is required. While the
arcajs web runtime environment is mainly a thin wrapper around the browser
standard APIs, the stand-alone executable is built upon the following
components, all written in portable and efficient C:

- [duktape](https://duktape.org) - embedded Javascript engine
- [SDL](https://libsdl.org) - cross-platform layer providing unified access to
  graphics, audio, and input hardware
- [stb\_image]((https://github.com/nothings/stb),
  [stb\_truetype]((https://github.com/nothings/stb),
  [nano\_svg](https://github.com/memononen/nanosvg),
  [dr\_mp3](https://github.com/mackron/dr_libs/blob/master/dr_mp3.h) - resource
  loaders for PNG and JPG images, TTF fonts, SVG vector graphics primitives, and
  MP3 audio samples
- [miniz](https://github.com/richgel999/miniz) - for accessing apps bundled as
  single zip or exe files
- own simplified graphics, sprites, collision, audio, and input APIs
  (unified touch/mouse, keyboard, game controllers)

## Getting started

Either check out the arcajs git archive, or preferably download the prepackaged
browser runtime or a precompiled stand-alone executable from [](https://).

A minimal hello world program using arcajs could look as follows:

```javascript
var audio = app.require('audio');

var img = app.getResource('hello_arcajs.svg');
var sprites = app.createSpriteSet(img);
var hello = sprites.createSprite();
hello.setPos(app.width/2, app.height/2);

app.on('update', function(deltaT, now) {
    hello.setRot(now*Math.PI*0.5);
    hello.setScale(Math.sin(now*3));
});

app.on('draw', function(gfx) {
    gfx.drawSprites(sprites);
});

app.on('pointer', function(evt) {
    if(evt.type==='start')
        audio.sound('square', 50+Math.random()*1000, 0.5, 0.5);
});
```

As one can see in this little example, writing apps with arcajs requires a basic
knowledge of Javascript.

The unified entry point to the entire arcajs API is the global app object. It
provides functions for accessing modules, resources, and creating game
entities. Furthermore the app object allows your program to register for
various events that are the core entry points for its interaction flow.

## Running your program

For runnning this app using the arcajs stand-alone runtime, put the arcajs
executable, your application logic script(s), and the application resources
(that is graphics, font, and sound files) in the same directory. Then you can
start your app from the command line by switching to this folder and typing
`.\arcajs.exe hello.js` on Windows or `./arcajs hello.js` on Linux.

The arcajs executable has a few command line parameters, the most relevant are

- -f - run in fullscreen mode
- -w - window size in pixels
- -h - window height in pixels

For example, you could also invoke the introductory example by typing
`.\arcajs.exe -f hello.js` or `.\arcajs.exe -w 1280 -h 720 hello.js`.

arcajs offers further ways to configure and simplify application startup via a
manifest file. This also opens up additional deployment options, for example 
packaging all assets into a zip archive and even linking everyting together
into a single executable, as described in [PACKAGING.md](doc/PACKAGING.md).

If you want to deploy your application on the web, you currently have to put all
your scripts and resources in the same folder as the web runtime and adapt the
web runtime's index.html file' last script tag to execute your application
logic:

```html
	<script type="text/javascript" src="hello.js"></script>
```

## API documentation

The foundations of arcajs are the ECMAScript standard-compliant APIs of its
underlying Duktape Javascript engine, which are also provided by all major
current web browsers. Please note that Duktape supports the Javascript standard
up to ECMAScript E5/E5.1, so some very modern language features should be
avoided in order to write portable code.

Beyond that, arcajs implements a few standard browser APIs (console, setTimeout,
localStorage) and its own interfaces accessible via the global app object. For
more detailed information take a look at the [arcajs API reference](doc/API.md)
and [application events documentation](doc/EVENTS.md).

## Further examples

For further examples please take a look at the examples folder within the
repository.

## License

This project is licensed under the permissive MIT License - see the
[LICENSE.md](LICENSE.md) file for further details.
