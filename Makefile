CC     = gcc
CFLAGS = -Wall -Wpedantic -Wno-overlength-strings -O3

SDL = ../SDL2

ifeq ($(OS),Windows_NT)
  OS            = win32
  ARCH          = $(OS)-x64
else
  OS            = $(shell uname -s)
  ARCH          = $(OS)_$(shell uname -m)
endif
CFLAGS += -DARCAJS_ARCH=\"$(ARCH)\"

ifeq ($(OS),Linux)
  INCDIR        = -I$(SDL)/include -D_REENTRANT -Iexternal
  LIBS          = -rdynamic
  ifeq ($(ARCH),Linux_armv7l)
    LIBS       += -L$(SDL)/lib/$(ARCH) -Wl,-rpath,$(SDL)/lib/$(ARCH) -Wl,--enable-new-dtags -lSDL2 -Wl,--no-undefined -Wl,-rpath,/opt/vc/lib -L/opt/vc/lib -lbcm_host -lpthread -lrt -ldl -lcurl -lm
  else
    LIBS       += -L$(SDL)/lib/$(ARCH) -Wl,-rpath,$(SDL)/lib/$(ARCH) -Wl,--enable-new-dtags -lSDL2 -Wl,--no-undefined -lpthread -ldl -lcurl -lm
  endif
  GLLIBS        = -lGL
  CFLAGS       += -fPIC -no-pie
  DLLFLAGS      = -shared
  DLLSUFFIX     = .so
  EXESUFFIX     = .$(shell uname -m)
  RM = rm -f
  SEP = /
else
  ifeq ($(OS),Darwin) # MacOS
    DLLFLAGS = -bundle
    DLLSUFFIX = .so
    EXESUFFIX = .app
  else # windows, MinGW
    INCDIR        = -I$(SDL)/include -Iexternal
    LIBS          = -L$(SDL)/lib/$(ARCH) -static -lmingw32 -lSDL2main -lSDL2 -Wl,--no-undefined -lm \
                    -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 \
                    -lshell32 -lsetupapi -lversion -luuid -lwininet -lwsock32 -static-libgcc -mwindows
    GLLIBS        = -lopengl32 -lSetupapi -lhid -lole32 -loleaut32 -limm32 -lversion
    DLLFLAGS      = -shared -s
    DLLSUFFIX     = .dll
    EXESUFFIX     = .exe
    RM = del /s
    SEP = \\#
  endif
endif

SRCLIB = window.c graphicsUtils.c console.c audio.c resources.c archive.c \
  value.c httpRequest.c external/miniz.c graphics.c log.c
SRC = arcajs.c graphicsBindings.c jsBindings.c worker.c \
  modules/intersects.c modules/intersectsBindings.c external/duktape.c
OBJ = $(SRC:.c=.o)
EXE = arcajs$(EXESUFFIX)
LIB = libarcajs.a

all: $(EXE) $(LIB)

# executable link rules:
$(EXE) : $(LIB) $(OBJ)
	$(CC) $(CFLAGS) $^ $(LIB) $(LIBS) -o $@ -s

$(LIB) : $(SRCLIB:.c=.o)
	ar -rcs $@ $^

zzipsetstub$(EXESUFFIX): zzipsetstub.o
	$(CC) $(CFLAGS) $^ -o $@ -s

dist: $(EXE)
	rcedit-x64.exe $(EXE) --set-icon doc\arcajs.ico
	upx $(EXE)

# modules:
modules/dgram$(DLLSUFFIX): modules/dgram.o external/duktape.o
	$(CC) -o $@ $(DLLFLAGS) $^ $(LIBS)
modules/os$(DLLSUFFIX): modules/os.o external/duktape.o
	$(CC) -o $@ $(DLLFLAGS) $^ $(LIBS)

# compilation dependencies:
arcajs.o: arcajs.c window.h graphics.h audio.h console.h resources.h archive.h jsBindings.h value.h log.h dukt_debug.h
resources.o: resources.c resources.h archive.h graphics.h audio.h graphicsUtils.h
archive.o: archive.c archive.h external/miniz.h
window.o: window.c window.h log.h
graphics.o: graphics.c graphics.h
graphicsUtils.o: graphicsUtils.c graphicsUtils.h font12x16.h \
  external/stb_truetype.h external/stb_image.h external/nanosvg.h external/nanosvgrast.h
audio.o: audio.c audio.h external/dr_mp3.h
console.o: console.c console.h graphics.h
jsBindings.o: jsBindings.c jsBindings.h jsCode.h window.h graphics.h audio.h \
  value.h graphicsUtils.h httpRequest.h log.h external/duktape.h external/duk_config.h
external/duktape.o: external/duktape.c external/duktape.h 
value.o: value.c value.h
httpRequest.o: httpRequest.c httpRequest.h log.h
log.o: log.c log.h
modules/intersects.o: modules/intersects.c modules/intersects.h
modules/intersectsBindings.o: modules/intersectsBindings.c modules/intersects.h \
  external/duktape.h external/duk_config.h
zzipsetstub.o: zzipsetstub.c
modules/dgram.o: modules/dgram.c
modules/fs.o: modules/fs.c

# generic rules and targets:
.c.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

clean:
	$(RM) *.o
	$(RM) external$(SEP)*.o
	$(RM) modules$(SEP)*.o
	$(RM) modules$(SEP)*$(DLLSUFFIX)
