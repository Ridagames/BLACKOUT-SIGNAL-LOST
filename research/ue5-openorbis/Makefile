OO_PS4_TOOLCHAIN ?= /lib/OpenOrbisSDK
CXX := clang++-12
LD := ld.lld-12
OUT := build
TARGET := $(OUT)/ue5_orbis_probe.elf
OBJ := $(OUT)/probe.o

CXXFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -O2 -Wall -Wextra -c \
            -isysroot $(OO_PS4_TOOLCHAIN) \
            -isystem $(OO_PS4_TOOLCHAIN)/include \
            -isystem $(OO_PS4_TOOLCHAIN)/include/c++/v1
LDFLAGS := -m elf_x86_64 -pie --script $(OO_PS4_TOOLCHAIN)/link.x \
           --eh-frame-hdr -L$(OO_PS4_TOOLCHAIN)/lib \
           -lc -lkernel -lc++ $(OO_PS4_TOOLCHAIN)/lib/crt1.o

.PHONY: all clean
all: $(TARGET)

$(OUT):
	mkdir -p $(OUT)

$(OBJ): probe.cpp | $(OUT)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	$(LD) $(OBJ) -o $@ $(LDFLAGS)
	test -s $@

clean:
	rm -rf $(OUT)
