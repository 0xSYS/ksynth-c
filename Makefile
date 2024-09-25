CC := gcc
CFLAGS := -D_POSIX_C_SOURCE=199309L -std=c99 -ffast-math
LDFLAGS :=
LIBS := -lm

ifeq ($(DEBUG),YES)
	CFLAGS += -g
endif

ifeq ($(EMSCRIPTEN),YES)
	CC := emcc
	CFLAGS := -D_POSIX_C_SOURCE=199309L -std=c99 -ffast-math
	LDFLAGS := -s ALLOW_MEMORY_GROWTH=1
	EMSCRIPTEN_FLAGS := -s EXPORTED_FUNCTIONS='["stringToUTF8", "lengthBytesUTF8", "_malloc", "_free", "_ksynth_get_commit_number", "_ksynth_new", "_ksynth_note_on", "_ksynth_note_off", "_ksynth_note_off_all", "_ksynth_cc", "_ksynth_get_polyphony", "_ksynth_get_max_polyphony", "_ksynth_set_max_polyphony", "_ksynth_set_release_oldest_instance", "_ksynth_fill_buffer", "_ksynth_generate_buffer", "_ksynth_get_rendering_time", "_ksynth_get_polyphony_for_channel", "_ksynth_buffer_free", "_ksynth_free"]'
endif

.PHONY: all clean format

ifdef WIN32
	CC := i686-w64-mingw32-gcc
	WINDOWS := YES
	LIBS += -lpthread
endif

ifdef WIN64
	CC := x86_64-w64-mingw32-gcc
	WINDOWS := YES
	LIBS += -lpthread
endif

ifdef ARM64
	CC := aarch64-linux-gnu-gcc
	LIBS += -lpthread
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

    ifeq ($(ARM64),YES)
    ./out/libksynth_arm64.a: $(OBJS)
	  mkdir -p ./out
	  ar rcs $@ $^
    endif
  else
    ifeq ($(EMSCRIPTEN),YES)
    ./out/ksynth.js: $(OBJS)
	  mkdir -p ./out
	  emcc $(CFLAGS) $(LDFLAGS) $(EMSCRIPTEN_FLAGS) -o $@ $^ $(LIBS) -s WASM=1
    else
      ifeq ($(ARM64),YES)
      ./out/libksynth_arm64.so: $(OBJS)
	  mkdir -p ./out
	  $(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)
      else
      ./out/libksynth.so: $(OBJS)
	  mkdir -p ./out
	  $(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)
    endif
  endif
endif

all:
	$(MAKE) -C . WIN32=YES
	$(MAKE) -C . WIN64=YES
	$(MAKE) -C . ARM64=YES
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

clean_objs:
	rm -rf $(OBJS)
