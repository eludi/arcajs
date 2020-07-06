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

```JSON
{
	"name":"application window title",
	"display":"fullscreen", // either "fullscreen" or "window" (default)
	"window_width": 640, // window width, only relevant if display is "window"
	"window_height": 480, // window height, only relevant if display is "window"
	"background_color" : "#000", // initial background color
	"icon": "icon.svg", // window icon displayed during startup
	"scripts":[ "main.js" ] // the script files containing the application logic
}
```

## File system access

For security reasons, arcajs apps by default have no read access to the host 
computer's file system outside the startup directory or archive, and also there
they can only perform read operations. For persisting the application state,
arcajs apps can use the [localStorage API](https://developer.mozilla.org/docs/Web/API/Window/localStorage),
as also provided by modern web browsers.

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
[zip](ftp://ftp.info-zip.org/pub/infozip/win32/)). This compilation procedure
can be automized, the arcajs source contains an exemplary package.bat file
demonstrating this, but this process likely requires manual tweaking, which is
clearly beyond the supported scope of arcajs and of this document.
