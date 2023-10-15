CXX = g++
LDFLAGS := -lSDL2 -lSDL2_image -Iinclude -mwindows
ICON = 

ifeq ($(OS),Windows_NT)
	LDFLAGS := -static-libgcc -static-libstdc++ -lmingw32 -lSDL2main $(LDFLAGS)
    ICON := res/icon.res
endif

all: dir
	$(CXX) -o bin/shannon.exe src/main.cpp include/zip.c $(ICON) $(LDFLAGS)

dir:
	if [ ! -d "./bin" ]; then mkdir -p bin; fi

install:
	cp -r ./res/dll/. ./bin/
