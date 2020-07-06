CC = gcc
CFLAGS = -Os -Wall -D_NO_GL #-g

SHARED = ../../shared
INCDIR = -I$(SHARED)/SDL2/include

ifeq ($(OS),Windows_NT)
  LIBS          = -lwininet -mconsole -s
  GFXLIBS       = -L$(SHARED)/SDL2/lib/win32-x64 -static -lmingw32 -lSDL2main -lSDL2 -Wl,--no-undefined -lm \
                  -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 \
                  -lshell32 -lsetupapi -lversion -luuid -lwininet -static-libgcc -mwindows
  DLLFLAGS      = -shared
  DLLSUFFIX     = .dll
  EXESUFFIX     = .exe
  RM            = del
else  # Linux
  OS            = $(shell uname -s)
  ARCH          = $(OS)_$(shell uname -m)
  LIBS          = -lcurl -s
  ifeq ($(ARCH),Linux_armv6l)
    GFXLIBS     = `sdl2-config --libs` -lm
  else
    GFXLIBS     = -L$(SHARED)/SDL2/lib/$(ARCH) -lSDL2 -lm
  endif
  DLLFLAGS      = -fPIC -shared
  DLLSUFFIX     = .so
  EXESUFFIX     =
  RM            = rm -f
endif

all: httpTest$(EXESUFFIX) archiveTest$(EXESUFFIX) dllTest$(DLLSUFFIX)

# link rules:
httpTest$(EXESUFFIX): httpTest.o ../httpRequest.o
	$(CC) $^ $(LIBS) $(GFXLIBS) -o $@
archiveTest$(EXESUFFIX): archiveTest.o ../archive.o ../external/miniz.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@
dllTest$(DLLSUFFIX): dllTest.o ../external/duktape.o
	$(CC) $(DLLFLAGS) $^ $(LIBS) -o $@

# compile dependencies:
httpTest.o: httpTest.c ../httpRequest.h
../httpRequest.o: ../httpRequest.c ../httpRequest.h
../window.o: ../window.c ../window.h
archiveTest.o: archiveTest.c ../archive.h
dllTest.o: dllTest.c

# compile rules:
.c.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

clean:
	$(RM) *.o *.exe *$(DLLSUFFIX)
