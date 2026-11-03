# BLACKOUT: SIGNAL LOST — PS4 / OpenOrbis
# Minimal PS4 package: executable, param.sfo and menu asset.

TITLE       := BLACKOUT - SIGNAL LOST
VERSION     := 01.00
TITLE_ID    := BLSG00001
CONTENT_ID  := IV0000-BLSG00001_00-0000000000000001

LIBS        := -lc -lkernel -lc++ -lSceSysUtil -lScePad
EXTRAFLAGS  := -O2 -Wall

TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJDIR     := source
INTDIR      := $(PROJDIR)/x64/Debug

CFILES      := $(wildcard $(PROJDIR)/*.c)
CPPFILES    := $(wildcard $(PROJDIR)/*.cpp)
OBJS        := $(patsubst $(PROJDIR)/%.c,$(INTDIR)/%.o,$(CFILES)) $(patsubst $(PROJDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES))

CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
COMMONDIR   := $(TOOLCHAIN)/samples/_common
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -I$(COMMONDIR)
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

UNAME_S     := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
CC          := clang
CXX         := clang++
LD          := ld.lld
CDIR        := linux
endif
ifeq ($(UNAME_S),Darwin)
CC          := /usr/local/opt/llvm/bin/clang
CXX         := /usr/local/opt/llvm/bin/clang++
LD          := /usr/local/opt/llvm/bin/ld.lld
CDIR        := macos
endif

.PHONY: all clean
all: $(CONTENT_ID).pkg

$(INTDIR):
	mkdir -p $(INTDIR)

$(INTDIR)/%.o: $(PROJDIR)/%.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: $(PROJDIR)/%.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

eboot.bin: $(INTDIR) $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/blackout.elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-eboot -in=$(INTDIR)/blackout.elf --paid 0x3800000000000011
	test -s eboot.bin
	file eboot.bin

sce_sys/param.sfo: Makefile
	mkdir -p sce_sys
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0x14000000
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

pkg.gp4: eboot.bin sce_sys/param.sfo assets/USRDIR/BLACKOUT_MENU.png
	$(TOOLCHAIN)/bin/$(CDIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) \
		--files "eboot.bin=eboot.bin" \
		--files "sce_sys/param.sfo=sce_sys/param.sfo" \
		--files "assets/USRDIR/BLACKOUT_MENU.png=USRDIR/BLACKOUT_MENU.png"

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

clean:
	rm -rf $(INTDIR) eboot.bin pkg.gp4 $(CONTENT_ID).pkg sce_sys/param.sfo
