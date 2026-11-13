# HAUNTED.EXE - a Halloween VGA screensaver
#   make host   - test harness for Linux (PNG frames + WAV audio); build/host_debug has the debug switches and event log
#   make asan   - build/host_asan: the debug harness under AddressSanitizer and UBSan (run with ASAN_OPTIONS=detect_leaks=0)
#   make scr    - Windows screensaver (needs x86_64-w64-mingw32-gcc)
#   make zip    - the release package
#   make smoke  - a short headless run of both scenes in the test harness
#   make cards  - the project page's title cards in site/cards (needs Pillow)
#   make sheets - CATACOMB's sprite sheets in build/sheets, by rank, for approval
#   make build/titlemock - the title screen's mock-ups (docs/PLAN-title.md): build/titlemock <dir>
#   make build/portraitmock - the portraits of the slain (docs/PLAN-portraits.md): build/portraitmock <dir>
#   make items  - a hundred rolled wares a level as build/items.json, for checking names and numbers
#   make combat - the balance sheet as build/combat.txt: the dead's harm and the caster's by level (docs/PLAN-combat.md)
#   make web    - the browser's build in build/web and dist/HAUNTED-<version>-web.zip (needs Emscripten's emcc)
# On Linux each file is compiled once into build/obj/ and kept, as many at a time as there are cores, so a change compiles
# only what it touched (make -j1 for one at a time; make clean after changing CFLAGS).
VERSION := $(shell cat VERSION)
FILEVER := $(shell sed -E 's/^([0-9]+)\.([0-9]+)\.([0-9]+).*/\1,\2,\3,0/' VERSION)
PKG      = dist/HAUNTED-$(VERSION)-win64.zip

# The sources, by kind (docs/PLAN-tidy.md): core/ what both scenes stand on, haunt/ and crawl/ the two scenes, art/ what only
# draws, data/ traced and generated bitmaps, platform/ the Windows and browser shells. The compiler is given the folders, so a
# file asks for a header by its name. The order here is the order they are linked in: keep it. A part of a file is not a
# source of its own and never goes in SRC.
INC      = -I. -Isrc/core -Isrc/haunt -Isrc/crawl -Isrc/art
HDR      = $(wildcard src/*/*.h)
# The parts of a file cut into parts (docs/PLAN-tidy.md): scene_crawl.c #includes its own from src/crawl/game and draw, and
# crawl_art.c its own from src/art/parts. On Linux the compiler's lists (below) say which object read which; the Windows and
# browser builds keep no lists, so they are told here.
PARTS    = $(wildcard src/*/*/*.c)
SRC      = src/core/app.c src/core/gfx.c src/data/font_vga.c src/core/snd.c src/crawl/scene_crawl.c src/art/crawl_art.c src/data/hand_img.c \
           src/crawl/crawl_gear.c src/data/face_img.c src/data/cat_img.c src/haunt/scene_haunt.c src/haunt/haunt_art.c src/art/crawl_ui.c \
           src/art/ui_art.c src/art/crawl_title.c src/art/crawl_portrait.c src/crawl/crawl_tutor.c
CFLAGS  ?= -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation
DEBUG    = -DHAUNT_DEBUG -DCRAWL_DEBUG
ASAN     = -O1 -g -std=gnu11 -fsanitize=address,undefined -fno-omit-frame-pointer

ifeq (,$(filter clean,$(MAKECMDGOALS)))
MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 2)
endif

all: host

# Objects (docs/PLAN-tidy.md, step 3). A file is compiled once for each way it is built: rel as it ships, debug with the debug
# switches, asan for the sanitizers; into build/obj/<way>/<its own path>.o. The compiler writes beside each what it #included
# (-MMD), so a header, or a part of a file cut into parts, compiles again exactly what read it. objs gives a list of sources
# its objects, in the same order: the order they are linked in.
OBJ      = build/obj
objs     = $(patsubst %.c,$(OBJ)/$(1)/%.o,$(2))

$(OBJ)/rel/%.o: %.c | build/version.h
	@mkdir -p $(@D)
	gcc $(CFLAGS) $(INC) -MMD -MP -c -o $@ $<

$(OBJ)/debug/%.o: %.c | build/version.h
	@mkdir -p $(@D)
	gcc $(CFLAGS) $(DEBUG) $(INC) -MMD -MP -c -o $@ $<

$(OBJ)/asan/%.o: %.c | build/version.h
	@mkdir -p $(@D)
	gcc $(ASAN) $(DEBUG) $(INC) -MMD -MP -c -o $@ $<

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)

build/host: $(call objs,rel,$(SRC) tools/host.c)
	gcc $(CFLAGS) -o $@ $^ -lm -lz

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

dist/HAUNTED.scr: $(SRC) $(PARTS) src/platform/win_main.c $(HDR) build/saver_res.o build/version.h
	@mkdir -p dist
	$(MINGW) $(CFLAGS) $(INC) -mwindows -static -o $@ $(SRC) src/platform/win_main.c build/saver_res.o \
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

build/host_debug: $(call objs,debug,$(SRC) tools/host.c)
	gcc $(CFLAGS) -o $@ $^ -lm -lz

build/host_asan: $(call objs,asan,$(SRC) tools/host.c)
	gcc $(ASAN) -o $@ $^ -lm -lz

asan: build/host_asan

cards: build/host_debug
	@mkdir -p build/cards
	cd build/cards && ../host_debug shot haunt 25 h 1 >/dev/null && HAUNT_PLAN=10 ../host_debug shot haunt 14 pk 1 >/dev/null \
	    && CRAWL_SEED=7 CRAWL_LEVEL=3 ../host_debug shot crawl 75,110 c 1 >/dev/null
	python3 tools/make_cards.py build/cards site/cards

ART_SRC  = src/art/crawl_art.c src/data/hand_img.c src/data/face_img.c src/data/cat_img.c src/data/font_vga.c

build/sheet: $(call objs,rel,$(ART_SRC) tools/sheet.c)
	gcc $(CFLAGS) -o $@ $^ -lm -lz

UI_SRC   = src/art/crawl_ui.c src/art/ui_art.c src/core/gfx.c src/crawl/crawl_tutor.c

build/uisheet: $(call objs,rel,$(ART_SRC) $(UI_SRC) tools/uisheet.c)
	gcc $(CFLAGS) -o $@ $^ -lm -lz

build/titlemock: $(call objs,rel,$(ART_SRC) $(UI_SRC) src/art/crawl_title.c tools/titlemock.c)
	gcc $(CFLAGS) -o $@ $^ -lm -lz

build/portraitmock: $(call objs,rel,$(ART_SRC) $(UI_SRC) src/art/crawl_portrait.c tools/portraitmock.c)
	gcc $(CFLAGS) -o $@ $^ -lm -lz

sheets: build/sheet build/uisheet
	@mkdir -p build/sheets
	./build/sheet all build/sheets 1
	./build/uisheet all build/sheets
	./build/uisheet pad build/sheets/pad.png
	./build/uisheet tutor build/sheets

build/items: $(call objs,rel,src/crawl/crawl_gear.c tools/items.c)
	gcc $(CFLAGS) -o $@ $^ -lm

items: build/items
	./build/items > build/items.json

combat: build/host_debug
	./build/host_debug combat > build/combat.txt
	@echo build/combat.txt

# The browser's build (docs/PLAN-web.md): the same sources with Space casting (HVS_WEB) and src/platform/web_main.c, as
# haunted-core.js and .wasm, beside the page and its script. Served from /play/ on the project page.
EMCC     ?= emcc
WEBPKG    = dist/HAUNTED-$(VERSION)-web.zip
WEBFLAGS  = -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation -DHVS_WEB $(INC) \
            -sMODULARIZE=1 -sEXPORT_NAME=HauntedCore -sENVIRONMENT=web -sINITIAL_MEMORY=64MB -sSTACK_SIZE=1MB \
            -sFILESYSTEM=0 -sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPF32,UTF8ToString

build/web/haunted-core.js: $(SRC) $(PARTS) src/platform/web_main.c $(HDR) build/version.h
	@mkdir -p build/web
	$(EMCC) $(WEBFLAGS) -o $@ $(SRC) src/platform/web_main.c

build/web/index.html: web/index.html web/haunted.js web/audio.js VERSION third_party/font/*
	@mkdir -p build/web
	sed -e "s/@VERSION@/$(VERSION)/g" web/index.html > $@
	cp web/haunted.js web/audio.js build/web/
	mkdir -p build/web/fonts
	cp third_party/font/Px437_IBM_VGA_8x16.ttf third_party/font/Px437_IBM_BIOS.ttf third_party/font/LICENSE-oldschool-pc-font-pack.txt build/web/fonts/

web: build/web/haunted-core.js build/web/index.html

$(WEBPKG): web
	@mkdir -p dist
	rm -f $@
	cd build/web && zip -q -9 -r ../../$@ index.html haunted.js audio.js haunted-core.js haunted-core.wasm fonts

webzip: $(WEBPKG)

clean:
	rm -rf build dist/HAUNTED.scr dist/HAUNTED.EXE dist/*.zip
	-@git worktree prune 2>/dev/null
.PHONY: all host asan scr zip smoke cards sheets items combat web webzip clean
