CC     = gcc
CFLAGS = -Wall -Wpedantic -Os

SHARED = ../shared

ifeq ($(OS),Windows_NT)
  OS            = win32
  ARCH          = $(OS)-x64
else
  OS            = $(shell uname -s)
  ARCH          = $(OS)_$(shell uname -m)
endif

ifeq ($(OS),Linux)
  INCDIR        = -I$(SHARED)/SDL2/include -D_REENTRANT -Iexternal
  ifeq ($(ARCH),Linux_armv6l)
    CFLAGS     += -DGRAPHICS_API_OPENGL_ES2
    LIBS        = `sdl2-config --libs` -L/opt/vc/lib -lbrcmGLESv2 -lbrcmEGL -ldl -lcurl -lm
  else
    CFLAGS     += -DGRAPHICS_API_OPENGL_33
    LIBS        = -L$(SHARED)/SDL2/lib/$(ARCH) -lSDL2 -lGL -ldl -lcurl -lm
  endif
  DLLFLAGS      = -fPIC -shared
  DLLSUFFIX     = .so
  EXESUFFIX     =
  RM = rm -rf
  SEP = /
else
  ifeq ($(OS),Darwin) # MacOS
    DLLFLAGS = -bundle
    DLLSUFFIX = .so
    EXESUFFIX = .app
  else # windows, MinGW
    INCDIR        = -I$(SHARED)/SDL2/include -Iexternal
    CFLAGS       += -DGRAPHICS_API_OPENGL_33
    LIBS          = -L$(SHARED)/SDL2/lib/$(ARCH) -lmingw32 -lSDL2main -lSDL2 -lwinmm -luser32 \
                    -lopengl32 -lgdi32 -lkernel32 -lwininet -lwsock32 -lm -mconsole
    STATIC_LIBS   = -L$(SHARED)/SDL2/lib/$(ARCH) -static -lmingw32 -lSDL2main -lSDL2 -Wl,--no-undefined -lm \
                    -ldinput8 -ldxguid -ldxerr8 -luser32 -lopengl32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 \
                    -lshell32 -lsetupapi -lversion -luuid -lwininet -static-libgcc -mwindows
    DLLFLAGS      = -shared -s
    DLLSUFFIX     = .dll
    EXESUFFIX     = .exe
    RM = del /s
    SEP = \\#
  endif
endif

SRC = arcajs.c window.c graphics.c graphicsGL.c graphicsUtils.c sprites.c spritesGl.c spritesBindings.c \
  audio.c resources.c archive.c jsBindings.c value.c httpRequest.c \
  modules/intersects.c modules/intersectsBindings.c external/miniz.c external/duktape.c external/gl3w.c
OBJ = $(SRC:.c=.o)
EXE = arcajs$(EXESUFFIX)

all: $(EXE) zzipsetstub$(EXESUFFIX) modules/dgram$(DLLSUFFIX) modules/fs$(DLLSUFFIX)

# executable link rules:
$(EXE) : $(OBJ)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@ -s

static: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(STATIC_LIBS) -o $(EXE) -s

zzipsetstub$(EXESUFFIX): zzipsetstub.o
	$(CC) $(CFLAGS) $^ -o $@ -s

# modules:
modules/dgram$(DLLSUFFIX): modules/dgram.o external/duktape.o
	$(CC) -o $@ $(DLLFLAGS) $^ $(LIBS)
modules/fs$(DLLSUFFIX): modules/fs.o external/duktape.o
	$(CC) -o $@ $(DLLFLAGS) $^ $(LIBS)

# compilation dependencies:
arcajs.o: arcajs.c window.h graphics.h graphicsGL.h audio.h resources.h archive.h jsBindings.h value.h
resources.o: resources.c resources.h archive.h graphics.h graphicsGL.h graphicsUtils.h \
  external/stb_image.h external/nanosvg.h external/nanosvgrast.h
archive.o: archive.c archive.h external/miniz.h
window.o: window.c window.h
graphics.o: graphics.c graphics.h external/stb_truetype.h external/stb_image.h
graphicsGL.o: graphicsGL.c graphicsGL.h \
  external/stb_truetype.h external/stb_image.h external/rlgl.noglad.h external/raymath.h
graphicsUtils.o: graphicsUtils.c graphicsUtils.h font12x16.h \
  external/stb_truetype.h external/stb_image.h external/nanosvg.h external/nanosvgrast.h
sprites.o: sprites.c sprites.h graphics.h modules/intersects.h
spritesGl.o: spritesGl.c sprites.c sprites.h graphicsGL.h modules/intersects.h
spritesBindings.o: spritesBindings.c sprites.h external/duktape.h external/duk_config.h
audio.o: audio.c audio.h external/dr_mp3.h
jsBindings.o: jsBindings.c jsBindings.h window.h graphics.h graphicsGL.h audio.h value.h httpRequest.h \
  external/duktape.h external/duk_config.h
external/duktape.o: external/duktape.c external/duktape.h 
value.o: value.c value.h
httpRequest.o: httpRequest.c httpRequest.h
modules/intersects.o: modules/intersects.c modules/intersects.h
modules/intersectsBindings.o: modules/intersectsBindings.c modules/intersects.h \
  external/duktape.h external/duk_config.h
zzipsetstub.o: zzipsetstub.c
external/gl3w.o: external/gl3w.c
modules/dgram.o: modules/dgram.c
modules/fs.o: modules/fs.c

# generic rules and targets:
.c.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

clean:
	$(RM) *.o
	$(RM) modules$(SEP)*$(DLLSUFFIX)
