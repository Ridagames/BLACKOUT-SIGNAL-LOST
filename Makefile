# BLACKOUT: SIGNAL LOST — PS4 / OpenOrbis

TITLE       := BLACKOUT - SIGNAL LOST
VERSION     := 01.00
TITLE_ID    := BLSG00001
CONTENT_ID  := IV0000-BLSG00001_00-0000000000000001

LIBS        := -lc -lkernel -lc++ -lSceSysUtil -lScePad
EXTRAFLAGS  := -O2 -Wall

TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJDIR     := source
INTDIR      := $(PROJDIR)/x64/Debug
STAGE       := build/pkg

CFILES      := $(wildcard $(PROJDIR)/*.c)
CPPFILES    := $(wildcard $(PROJDIR)/*.cpp)
OBJS        := $(patsubst $(PROJDIR)/%.c,$(INTDIR)/%.o,$(CFILES)) $(patsubst $(PROJDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES))

CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
COMMONDIR   := $(TOOLCHAIN)/samples/_common
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -I$(COMMONDIR)
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

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

eboot.bin: $(INTDIR) $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/blackout.elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-eboot -in=$(INTDIR)/blackout.elf --paid 0x3800000000000011
	test -s $@

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

pkg.gp4: eboot.bin sce_sys/param.sfo assets/USRDIR/BLACKOUT_MENU.png tools/create_gp4.py
	python3 tools/create_gp4.py --content-id '$(CONTENT_ID)' --title-id '$(TITLE_ID)' --stage '$(STAGE)' --eboot eboot.bin --sfo sce_sys/param.sfo --asset assets/USRDIR/BLACKOUT_MENU.png --output $(STAGE)/pkg.gp4

$(CONTENT_ID).pkg: pkg.gp4
	cd $(STAGE) && $(PKGTOOL) pkg_build pkg.gp4 .
	cp $(STAGE)/$(CONTENT_ID).pkg $@

clean:
	rm -rf $(INTDIR) $(STAGE) build eboot.bin pkg.gp4 $(CONTENT_ID).pkg sce_sys/param.sfo

.PHONY: all clean