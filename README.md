# ![](doc/arcajs.png "arcajs logo") arcajs

arcajs is a simple and lightweight cross-platform Javascript 2D multimedia/game
framework that runs both within a browser and on its own compact and optimized
stand-alone runtime environment. This unique combination enables you to write
portable games and multimedia apps that efficiently run on a wide range of
modern devices, from low-end single board computers (such as the Raspberry Pi)
over smartphones and tablets up to powerful gaming PCs.

## Components

Both the arcajs web app environment and the stand-alone executable offer the
same Javascript API, no additional HTML or CSS code is required. While the
arcajs web runtime environment is mainly a thin wrapper around the browser
standard APIs, the stand-alone executable is built upon the following
components, all written in portable and efficient C:

- [duktape](https://duktape.org) - embedded Javascript engine
- [SDL](https://libsdl.org) - cross-platform layer providing unified access to
  graphics, audio, and input hardware
- [stb\_image](https://github.com/nothings/stb),
  [stb\_truetype](https://github.com/nothings/stb),
  [nano\_svg](https://github.com/memononen/nanosvg),
  [dr\_mp3](https://github.com/mackron/dr_libs/blob/master/dr_mp3.h) - resource
  loaders for PNG and JPG images, TTF fonts, SVG vector graphics primitives, and
  MP3 or WAV audio samples
- [miniz](https://github.com/richgel999/miniz) - for accessing apps bundled as
  single zip or exe files
- own simplified graphics, sprites, collision, audio, and input APIs
  (unified touch/mouse, keyboard, game controllers)

## Getting started

Either check out the [arcajs git repository](https://github.com/eludi/arcajs),
or download the prepackaged browser runtime, a precompiled stand-alone
executable from the [releases](https://github.com/eludi/arcajs/releases/latest),
or a bootstrap script fitting to your platform from the
[doc folder](https://github.com/eludi/arcajs/tree/master/doc).

A [minimal hello world app](https://eludi.github.io/arcajs/examples/hello/index.html)
using arcajs could look as follows:

```javascript
var audio = app.require('audio');

var hello = {
    x: app.width/2,
    y: app.height/2,
    rot: 0,
    sc: 0,
    image: app.getResource('hello_arcajs.svg', {centerX:0.5, centerY:0.5})
};

app.on('resize', function(winSzX, winSzY) {
    hello.x = winSzX/2;
    hello.y = winSzY/2;
});

app.on('update', function(deltaT, now) {
    hello.rot = now*Math.PI/2
    hello.sc = Math.sin(now*3);
});

app.on('draw', function(gfx) {
    gfx.drawSprite(hello);
});

app.on('pointer', function(evt) {
    if(evt.type==='start')
        audio.sound('square', 50+Math.random()*1000, 0.5, 0.5);
});
```

As one can see from this little example, writing apps with arcajs requires just
a basic understanding of Javascript.

Ordinary Javascript objects are used for defining and managing the basic
entities of your application (in this example `hello`).
The unified entry point to the entire arcajs API is the global `app` object. It
provides functions for accessing modules and resources. Furthermore, the app
object allows your program to register for various events that are the core
entry points for its interaction flow.

## Running your program

For runnning this app using the arcajs stand-alone runtime, put the arcajs
executable, your application logic script(s), and the application resources
(that is graphics, font, and sound files) in the same directory. Then you can
start your app from the command line by switching to this folder and typing
`.\arcajs.exe hello.js` on Windows or `./arcajs hello.js` on Linux.

The arcajs executable has a few command line parameters, the most relevant are

- -f - run in fullscreen mode
- -w {number} - window size in pixels, pass 0 to run as command line tool
- -h {number} - window height in pixels
- -j {number} - which SDL joystick API to use, value 0 means semantic Gamepad API, 1 low-
  level Joystick API, -1 completely disables joystick input
- -d - enable debug output
- -m {number} - cap maximum number of frames per second

For example, you could also invoke the introductory example by typing
`.\arcajs.exe -f hello.js` or `.\arcajs.exe -w 1280 -h 720 hello.js`.

arcajs offers further ways to configure and simplify application startup via a
manifest file. This also opens up additional deployment options, for example 
packaging all assets into a zip archive and even linking everything together
into a single executable, as described in [PACKAGING.md](doc/PACKAGING.md).

If you want to deploy your application on the web, it is sufficient to maintain
a manifest file with the sources and put them together with all resources
into the same folder as copy of the web runtime.

Since September 2024 arcajs also offers experimental native Android support.
Refer to [PACKAGING.md](doc/PACKAGING.md) for further details.

## API documentation

The foundations of arcajs are the ECMAScript standard-compliant APIs of its
underlying Duktape Javascript engine, which are also provided by all major
current web browsers. Please note that Duktape supports the Javascript standard
up to ECMAScript E5/E5.1, so some very modern language features should be
avoided in order to write portable code.

Beyond that, arcajs implements a few standard browser APIs (console, setTimeout,
localStorage, Worker) and its own interfaces accessible via the global app object.
For more detailed information take a look at the [arcajs API reference](doc/API.md)
and [application events documentation](doc/EVENTS.md).

## Further examples

For further examples please take a look at the [examples](https://github.com/eludi/arcajs/tree/master/examples)
and [test](https://github.com/eludi/arcajs/tree/master/test) folders within
the repository or try them directly from your browser:

- [asteroludi](https://eludi.github.io/arcajs/examples/asteroludi/)
- [Bro vs Bro](https://eludi.github.io/arcajs/examples/bro_vs_bro/)
- [memomini](https://eludi.github.io/arcajs/examples/memomini/)
- [return!](https://eludi.github.io/arcajs/examples/return/)
- [sprite performance stresstest](https://eludi.github.io/arcajs/examples/perf/)

## Status

Arcajs has recently entered its beta stage. From our experience so far it runs
fairly fast and stable and also scope-wise it is widely complete, so we consider
it already now as a good basis for smaller projects.

The current development focus is on ease of use, therefore some incompatible API changes are to be expected. Of course, any feedback and ideas are very welcome!
Please contact us either using the
[github issues tracker](https://github.com/eludi/arcajs/issues/new) or via
[email](mailto:arcajs$AT$eludi.net).

## License

This project is licensed under the permissive MIT License - see the
[LICENSE.md](LICENSE.md) file for further details.
