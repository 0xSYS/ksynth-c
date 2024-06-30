CC := gcc
CFLAGS := -D_POSIX_C_SOURCE=199309L -std=c99 -ffast-math
LDFLAGS :=
LIBS := -lm
ifeq ($(DEBUG),YES)
	CFLAGS += -g
endif

.PHONY: all clean

ifdef WIN32
CC := i686-w64-mingw32-gcc
WINDOWS := YES
LIBS += -lrt
endif

ifdef WIN64
CC := x86_64-w64-mingw32-gcc
WINDOWS := YES
LIBS += -lrt
endif

OBJS := ./src/ksynth.o ./src/sample.o ./src/voice.o

ifdef WINDOWS
ifdef WIN32
./out/ksynth_x86.dll: $(OBJS)
	mkdir -p ./out
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)
endif
ifdef WIN64
./out/ksynth_x64.dll: $(OBJS)
	mkdir -p ./out
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)
endif
else
ifdef STATIC
./out/libksynth.a: $(OBJS)
	mkdir -p ./out
	ar rcs $@ $^
else
./out/libksynth.so: $(OBJS)
	mkdir -p ./out
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)
endif
endif

all:
	$(MAKE) -C . WIN32=YES
	$(MAKE) -C . WIN64=YES
	$(MAKE) -C .

./src/ksynth.o: ./src/ksynth.c ./src/ksynth.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

./src/sample.o: ./src/sample.c ./src/sample.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

./src/voice.o: ./src/voice.c ./src/voice.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

format:
	clang-format -i `find . -name "*.h" -or -name "*.c"`

clean:
	rm -rf out $(OBJS)
