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
GPU_MODEL_DIR := build/gpu-model
GPU_MODEL := $(GPU_MODEL_DIR)/NEXUS9_MENU.gltf
GPU_MODEL_BIN := $(GPU_MODEL_DIR)/NEXUS9_MENU.bin
GPU_SHADERS := assets/SHADERS/nexus9.vert.sb assets/SHADERS/nexus9.frag.sb assets/SHADERS/clear.frag.sb assets/SHADERS/nexus9_ui.frag.sb assets/SHADERS/nexus9_shadow.vert.sb assets/SHADERS/nexus9_shadow.frag.sb

# OpenOrbis runtime modules supplied by the SDK.
MODULE_SOURCE := $(TOOLCHAIN)/bin/data/modules
LIBC_PRX := $(MODULE_SOURCE)/libc.prx
FIOS_PRX := $(MODULE_SOURCE)/libSceFios2.prx

OPENGNM_LIB := tools/opengnm/libopengnm.a
LIBS := -lc -lkernel -lc++ -lSceVideoOut -lScePad -lSceUserService -lSceSysmodule -lSceGnmDriver
EXTRAFLAGS := -O2 -Wall
CFILES := $(wildcard $(PROJDIR)/*.c)
CPPFILES := $(wildcard $(PROJDIR)/*.cpp)
GRAPHICS_OBJ := $(INTDIR)/graphics.o
GPU_OBJS := $(INTDIR)/gpu_renderer.o $(INTDIR)/gpu_fs.o $(INTDIR)/gpu_utility.o \
            $(INTDIR)/gpu_memalloc.o $(INTDIR)/gpu_misc.o $(INTDIR)/gpu_displayctx.o \
            $(INTDIR)/gpu_gltfmodel.o $(INTDIR)/gpu_pass_model.o
OBJS := $(patsubst $(PROJDIR)/%.c,$(INTDIR)/%.o,$(CFILES)) \
        $(patsubst $(PROJDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES)) \
        $(GRAPHICS_OBJ) $(GPU_OBJS)

CFLAGS := -Itools/opengnm/include -Itools/freegnm-examples/shared/src -Itools/freegnm-examples/gltf/src -Itools/cglm/include -DUSE_OPENGNM -DCGLM_ALL_UNALIGNED -DCGLM_FORCE_LEFT_HANDED -DCGLM_FORCE_DEPTH_ZERO_TO_ONE -D_BSD_SOURCE --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -DDIAGNOSTIC_BUILD=$(DIAGNOSTIC_BUILD)
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

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

$(INTDIR)/gpu_renderer.o: $(PROJDIR)/common/gpu_renderer.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/gpu_fs.o: tools/freegnm-examples/shared/src/u/fs.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/gpu_utility.o: tools/freegnm-examples/shared/src/u/utility.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/gpu_memalloc.o: tools/freegnm-examples/shared/src/memalloc.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/gpu_misc.o: tools/freegnm-examples/shared/src/misc.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/gpu_displayctx.o: tools/freegnm-examples/shared/src/displayctx.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/gpu_gltfmodel.o: tools/freegnm-examples/shared/src/gltfmodel.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/gpu_pass_model.o: tools/freegnm-examples/gltf/src/pass_model.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

sce_sys/icon0.png: sce_sys/ICON0.PNG
	convert "$<" -resize '512x512!' -background black -alpha remove -alpha off -depth 8 PNG24:"$@"
	test -s "$@"

eboot.bin: $(INTDIR) $(OBJS)
	$(LD) $(OBJS) $(OPENGNM_LIB) -o $(INTDIR)/blackout.elf $(LDFLAGS)
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

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png $(MENU_MODEL) $(GPU_MODEL) $(GPU_MODEL_BIN) $(GPU_SHADERS) $(LIBC_PRX) $(FIOS_PRX)
	test -x $(CREATE_GP4)
	rm -rf $(STAGE)
	mkdir -p $(STAGE)/sce_sys $(STAGE)/sce_module $(STAGE)/USRDIR/MODELS $(STAGE)/USRDIR/SHADERS
	cp eboot.bin $(STAGE)/eboot.bin
	cp sce_sys/param.sfo $(STAGE)/sce_sys/param.sfo
	cp sce_sys/icon0.png $(STAGE)/sce_sys/icon0.png
	cp $(MENU_MODEL) $(STAGE)/USRDIR/MODELS/NEXUS9_MENU.OBJ
	cp $(GPU_MODEL) $(STAGE)/USRDIR/MODELS/NEXUS9_MENU.gltf
	cp $(GPU_MODEL_BIN) $(STAGE)/USRDIR/MODELS/NEXUS9_MENU.bin
	cp $(GPU_SHADERS) $(STAGE)/USRDIR/SHADERS/
	cp $(LIBC_PRX) $(STAGE)/sce_module/libc.prx
	cp $(FIOS_PRX) $(STAGE)/sce_module/libSceFios2.prx
	cd $(STAGE) && $(CREATE_GP4) -out pkg.gp4 --content-id=$(CONTENT_ID) --files "eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx USRDIR/MODELS/NEXUS9_MENU.OBJ USRDIR/MODELS/NEXUS9_MENU.gltf USRDIR/MODELS/NEXUS9_MENU.bin USRDIR/SHADERS/nexus9.vert.sb USRDIR/SHADERS/nexus9.frag.sb USRDIR/SHADERS/clear.frag.sb USRDIR/SHADERS/nexus9_ui.frag.sb USRDIR/SHADERS/nexus9_shadow.vert.sb USRDIR/SHADERS/nexus9_shadow.frag.sb"
	cp $(STAGE)/pkg.gp4 pkg.gp4

$(CONTENT_ID).pkg: pkg.gp4
	cd $(STAGE) && $(PKGTOOL) pkg_build pkg.gp4 .
	cp $(STAGE)/$(CONTENT_ID).pkg $@

clean:
	rm -rf $(INTDIR) $(STAGE) build eboot.bin pkg.gp4 $(CONTENT_ID).pkg sce_sys/param.sfo sce_sys/icon0.png

.PHONY: all clean
