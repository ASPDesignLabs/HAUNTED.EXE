# HAUNTED.EXE - a Halloween VGA screensaver
#   make host   - test harness for Linux (PNG frames + WAV audio)
#   make scr    - Windows screensaver (needs x86_64-w64-mingw32-gcc)
#   make zip    - the release package
#   make smoke  - a short headless run of both scenes in the test harness
VERSION := $(shell cat VERSION)
FILEVER := $(shell sed -E 's/^([0-9]+)\.([0-9]+)\.([0-9]+).*/\1,\2,\3,0/' VERSION)
PKG      = dist/HAUNTED-$(VERSION)-win64.zip

SRC      = src/app.c src/gfx.c src/font_vga.c src/snd.c src/scene_crawl.c src/crawl_art.c src/hand_img.c src/crawl_gear.c src/face_img.c src/cat_img.c src/scene_haunt.c src/haunt_art.c
CFLAGS  ?= -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation

all: host

build/host: $(SRC) tools/host.c src/*.h
	@mkdir -p build
	gcc $(CFLAGS) -o $@ $(SRC) tools/host.c -lm -lz

host: build/host

MINGW ?= x86_64-w64-mingw32-gcc
WINDRES ?= x86_64-w64-mingw32-windres
STRIP ?= x86_64-w64-mingw32-strip
scr: dist/HAUNTED.scr

build/version.h: VERSION
	@mkdir -p build
	printf '#define HVS_VERSION "%s"\n#define HVS_FILEVERSION %s\n' '$(VERSION)' '$(FILEVER)' > $@

build/saver_res.o: res/saver.rc res/saver.manifest build/version.h
	$(WINDRES) -I. res/saver.rc -O coff -o $@

dist/HAUNTED.scr: $(SRC) src/win_main.c src/*.h build/saver_res.o build/version.h
	@mkdir -p dist
	$(MINGW) $(CFLAGS) -I. -mwindows -static -o $@ $(SRC) src/win_main.c build/saver_res.o \
	    -lopengl32 -lgdi32 -luser32 -lwinmm -ladvapi32 -lcomctl32 -lkernel32 -lm
	$(STRIP) --strip-unneeded $@

dist/HAUNTED.EXE: dist/HAUNTED.scr
	cp $< $@

$(PKG): dist/HAUNTED.scr dist/HAUNTED.EXE
	python3 tools/package.py $@

zip: $(PKG)

smoke: build/host
	@mkdir -p build/smoke
	./build/host shot haunt 5,20,40 build/smoke/haunt
	./build/host shot crawl 5,20,40 build/smoke/crawl
	HOST_INPUT="1:+H;4.3:-H;5:+W;7:-W;7.5:+MOUSE1;7.7:-MOUSE1;8:m400,0;9:+TAB;9.2:-TAB;10:+TAB;10.2:-TAB" \
	    ./build/host shot crawl 3,6,9.5,12 build/smoke/play
	./build/host bench crawl 60

clean:
	rm -rf build dist/HAUNTED.scr dist/HAUNTED.EXE dist/*.zip
.PHONY: all host scr zip smoke clean
