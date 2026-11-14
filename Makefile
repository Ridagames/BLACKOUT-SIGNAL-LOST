# BLACKOUT: SIGNAL LOST — PS4 / OpenOrbis
TITLE := BLACKOUT - SIGNAL LOST
VERSION := 01.00
TITLE_ID := BLSG00001
CONTENT_ID := IV0000-BLSG00001_00-0000000000000001

TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
CREATE_FSELF ?= create-fself
CREATE_GP4 ?= /usr/local/bin/create-gp4
PROJDIR := source
INTDIR := $(PROJDIR)/x64/Debug
STAGE := build/pkg
DIAGNOSTIC_BUILD := 0
MENU_MODEL := assets/MODELS/NEXUS9_MENU.OBJ

# OpenOrbis runtime modules supplied by the SDK.
MODULE_SOURCE := $(TOOLCHAIN)/bin/data/modules
LIBC_PRX := $(MODULE_SOURCE)/libc.prx
FIOS_PRX := $(MODULE_SOURCE)/libSceFios2.prx

LIBS := -lc -lkernel -lc++ -lSceVideoOut -lScePad -lSceUserService -lSceSysmodule
EXTRAFLAGS := -O2 -Wall
CFILES := $(wildcard $(PROJDIR)/*.c)
CPPFILES := $(wildcard $(PROJDIR)/*.cpp)
GRAPHICS_OBJ := $(INTDIR)/graphics.o
OBJ_OBJ := $(INTDIR)/obj_renderer.o
OBJS := $(patsubst $(PROJDIR)/%.c,$(INTDIR)/%.o,$(CFILES)) \
        $(patsubst $(PROJDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES)) \
        $(GRAPHICS_OBJ) \
        $(OBJ_OBJ)

CFLAGS := -Itools/opengnm/include --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -DDIAGNOSTIC_BUILD=$(DIAGNOSTIC_BUILD)
LDFLAGS := $(OPENGNM_LIB) -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
CC := clang
CXX := clang++
LD := ld.lld
CDIR := linux
endif
ifeq ($(UNAME_S),Darwin)
CC := /usr/local/opt/llvm/bin/clang
CXX := /usr/local/opt/llvm/bin/clang++
LD := /usr/local/opt/llvm/bin/ld.lld
CDIR := macos
endif

PKGTOOL := $(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core

all: $(CONTENT_ID).pkg

$(INTDIR):
	mkdir -p $(INTDIR)

$(INTDIR)/%.o: $(PROJDIR)/%.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: $(PROJDIR)/%.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(GRAPHICS_OBJ): $(PROJDIR)/common/graphics.cpp $(PROJDIR)/common/graphics.h | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OBJ_OBJ): $(PROJDIR)/common/obj_renderer.cpp $(PROJDIR)/common/obj_renderer.h $(PROJDIR)/common/graphics.h | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

sce_sys/icon0.png: sce_sys/ICON0.PNG
	convert "$<" -resize '512x512!' -background black -alpha remove -alpha off -depth 8 PNG24:"$@"
	test -s "$@"

eboot.bin: $(INTDIR) $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/blackout.elf $(LDFLAGS)
	test -x $(CREATE_FSELF)
	$(CREATE_FSELF) -in=$(INTDIR)/blackout.elf -out=$(INTDIR)/blackout.oelf -eboot=$@ -paid=0x3800000000000011
	test -s $@
	od -An -N4 -tx1 "$@" | grep -q "4f 15 3d 1d"

sce_sys/param.sfo: Makefile
	mkdir -p sce_sys
	$(PKGTOOL) sfo_new $@
	$(PKGTOOL) sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOL) sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOL) sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOL) sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOL) sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOL) sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOL) sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png $(MENU_MODEL) $(LIBC_PRX) $(FIOS_PRX)
	test -x $(CREATE_GP4)
	rm -rf $(STAGE)
	mkdir -p $(STAGE)/sce_sys $(STAGE)/sce_module $(STAGE)/USRDIR/MODELS
	cp eboot.bin $(STAGE)/eboot.bin
	cp sce_sys/param.sfo $(STAGE)/sce_sys/param.sfo
	cp sce_sys/icon0.png $(STAGE)/sce_sys/icon0.png
	cp $(MENU_MODEL) $(STAGE)/USRDIR/MODELS/NEXUS9_MENU.OBJ
	cp $(LIBC_PRX) $(STAGE)/sce_module/libc.prx
	cp $(FIOS_PRX) $(STAGE)/sce_module/libSceFios2.prx
	cd $(STAGE) && $(CREATE_GP4) -out pkg.gp4 --content-id=$(CONTENT_ID) --files "eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx USRDIR/MODELS/NEXUS9_MENU.OBJ"
	cp $(STAGE)/pkg.gp4 pkg.gp4

$(CONTENT_ID).pkg: pkg.gp4
	cd $(STAGE) && $(PKGTOOL) pkg_build pkg.gp4 .
	cp $(STAGE)/$(CONTENT_ID).pkg $@

clean:
	rm -rf $(INTDIR) $(STAGE) build eboot.bin pkg.gp4 $(CONTENT_ID).pkg sce_sys/param.sfo sce_sys/icon0.png

.PHONY: all clean
