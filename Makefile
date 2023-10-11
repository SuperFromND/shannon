CXX = g++
LDFLAGS := -lSDL2 -lSDL2_image -mwindows

ifeq ($(OS),Windows_NT)
	LDFLAGS := -static-libgcc -static-libstdc++ -lmingw32 -lSDL2main $(LDFLAGS)
endif

all: dir
	$(CXX) -o bin/shannon.exe src/main.cpp $(LDFLAGS)

dir:
	if [ ! -d "./bin" ]; then mkdir -p bin; fi

install:
	cp -r ./res/dll/. ./bin/
