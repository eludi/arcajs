# arcajs packaging and startup

If the arcajs stand-alone runtime is invoked with a directory or zip archive
name as command line argument, it first looks for a JSON-formatted text file
called `manifest.json` within this location during startup. If no manifest file
exists, then arcajs looks for a file named `main.js` at this location. If also
this script file does not exist or is not valid, the runtime aborts with an
error message.

## Manifest files

The purpose of manifest files is to provide meta information about an
application and to control its startup behavior. Arcajs' manifest files use a
similar format to
[Web App manifests](https://developer.mozilla.org/docs/Web/Manifest), but
one needs to be aware of the slight semantic and syntactic differences resulting
from their different contexts. Arcajs interprets the following parameters:

```javascript
{
	"name":"application window title",
	"display":"fullscreen", // either "fullscreen", "window" (default), or "resizable"
	"orientation":"landscape", // fixates screen orientation on mobile platforms. Either "any" (default), "landscape", or "portrait"
	"window_width": 640, // initial window width, only relevant if display is "window" or "resizable"
	"window_height": 480, // initial window height, only relevant if display is "window" or "resizable"
	"console_y": 0, // overlay console vertical position
	"console_height": 448, // overlay debug console height, set 0 to disable
	"background_color" : "#000", // initial background color
	"icon": "icon.svg", // window icon displayed during startup
	"scripts":[ "main.js" ], // the script files containing the application logic
	"audio_frequency": 44100, // sample rate of the audio device
	"audio_tracks": 8 // number of parallel audio tracks
}
```

## File system access

For security reasons, arcajs apps by default have no read access to the host 
computer's file system outside the startup directory or archive, and also there
they can only perform read operations. For persisting the application state,
arcajs apps can use the [localStorage API](https://developer.mozilla.org/docs/Web/API/Window/localStorage),
as also provided by modern web browsers.

If a particular app needs full filesystem read/write access or other functionality
provided by the operating system, it is possible to extend arcajs with native
modules. The modules directory contains an example for such a native module written
in C called fs.c . The arcajs makefile contains exemplary directives for
compiling this source file into a shared library. This is however an advanced
topic beyond the main scope of arcajs and therefore not officially supported.

## Packaging

Starting arcajs apps via command line parameters is convenient during
development, but usually not preferable for deploying completed apps to
consumers. The easiest way to make an app runnable by simply clicking on it in a
file browser or explorer window is to put an arcajs stand-alone executable in
the same folder as the app and to provide a manifest file with all necessary
settings. The arcajs executable can also be renamed to match the app's name.

However, this approach exposes all application logic and resource files
directly to the consumer. In order to hide these implementation details from
non-technical users, one could package all script and resource files into a zip
archive. Please make sure that the archive is a flat collection of files without
an inner folder structure. The zip suffix may also be removed from the archive
name. Now the app can be launched by dragging the archive onto the arcajs
executable, or by additionally providing a single line batch or shell script.

Finally, on Windows the arcajs.exe runtime and the application logic zip archive
can be combined into a single executable file using the zzipsetstub utility from
[lib_zzip](https://github.com/xriss/gamecake/blob/master/lib_zzip/test/zzipsetstub.c)
and additional free command line tools ([upx](https://upx.github.io/) and
[zip](http://http://infozip.sourceforge.net/)). This compilation procedure
can be automized, the arcajs source contains an exemplary package.bat file
demonstrating this, but this process likely requires manual tweaking, which is
clearly beyond the supported scope of arcajs and of this document.

## Native Android apk packaging

Since September 2024 arcajs apps can be packaged and deployed as Android APKs. As a
limitation, the httpRequest call (requiring libcurl and openssl) is not yet ported
to Android. The deployment and assembly process is still experimental not yet
streamlined; manual compilation and therefore basic Android development knowledge is
required.

The development procedure widely resembles other SDL2-based C applications as
described in the [SDL2 documentation](https://wiki.libsdl.org/SDL2/README/android)
and requires the Android SDK and NDK to be installed on a development machine.

The arcajs sources contain a respective Android.mk file that needs to be placed
into the app/jni/src location of the generated projects alongside the arcajs .c and
.h source files. All resources and javascript code need to be packed into an
assets.zip file to be placed into the app/src/main/assets location of the Android
project. Then the arcajs project may be built, either via Android Studio or via
the command line by calling `./gradlew build` in the project's home directory.
