CC     = gcc
CFLAGS = -Wall -Wpedantic -Wno-overlength-strings -Os
LFLAGS = -s -no-pie

SDL = ../../SDL2

ifeq ($(OS),Windows_NT)
  OS            = win32
  ARCH          = $(OS)-x64
else
  OS            = $(shell uname -s)
  ARCH          = $(OS)_$(shell uname -m)
endif

ifeq ($(OS),Linux)
  INCDIR        = -I$(SDL)/include -D_REENTRANT -Iexternal
  ifeq ($(ARCH),Linux_x86_64)
    LIBS        = -L$(SDL)/lib/$(ARCH) -Wl,-rpath,$(SDL)/lib/$(ARCH) -Wl,--enable-new-dtags -lSDL2 -Wl,--no-undefined -ldl -lpthread -lrt -ldl -lcurl -lm
  else
    LIBS        = -L$(SDL)/lib/$(ARCH) -Wl,-rpath,$(SDL)/lib/$(ARCH) -Wl,--enable-new-dtags -lSDL2 -Wl,--no-undefined -ldl -Wl,-rpath,/opt/vc/lib -L/opt/vc/lib -lbcm_host -lpthread -lrt -ldl -lcurl -lm
  endif
  CFLAGS       += -fPIC
  DLLFLAGS      = -shared
  DLLSUFFIX     = .so
  EXESUFFIX     =
  RM = rm -f
  SEP = /
else
  ifeq ($(OS),Darwin) # MacOS
    DLLFLAGS = -bundle
    DLLSUFFIX = .so
    EXESUFFIX = .app
  else # windows, MinGW
    INCDIR        = -I$(SDL)/include -Iexternal
    LIBS          = -L$(SDL)/lib/$(ARCH) -lmingw32 -lSDL2main -lSDL2 -lwinmm -luser32 \
                    -lgdi32 -lkernel32 -lwininet -lwsock32 -lm -mconsole
    STATIC_LIBS   = -L$(SDL)/lib/$(ARCH) -static -lmingw32 -lSDL2main -lSDL2 -Wl,--no-undefined -lm \
                    -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 \
                    -lshell32 -lsetupapi -lversion -luuid -lwininet -static-libgcc -mwindows
    DLLFLAGS      = -shared -s
    DLLSUFFIX     = .dll
    EXESUFFIX     = .exe
    RM = del /s
    SEP = \\#
  endif
endif

all: httpTest$(EXESUFFIX) archiveTest$(EXESUFFIX) dllTest$(DLLSUFFIX)

# link rules:
httpTest$(EXESUFFIX): httpTest.o ../httpRequest.o
	$(CC) $^ $(LIBS) $(GFXLIBS) -o $@ $(LFLAGS)
archiveTest$(EXESUFFIX): archiveTest.o ../archive.o ../external/miniz.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@ $(LFLAGS)
dllTest$(DLLSUFFIX): dllTest.o ../external/duktape.o
	$(CC) $(DLLFLAGS) $^ $(LIBS) -o $@ $(LFLAGS)

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
