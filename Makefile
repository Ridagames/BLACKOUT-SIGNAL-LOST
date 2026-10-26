.SUFFIXES:

ifeq ($(strip $(PSL1GHT)),)
$(error "PSL1GHT is not set")
endif

include $(PSL1GHT)/ppu_rules

TARGET := blackout
BUILD := build
SOURCES := source

TITLE := BLACKOUT: SIGNAL LOST
APPID := BLSG00001
CONTENTID := UP0001-BLSG00001_00-0000000000000000

PKGFILES := $(CURDIR)/assets

INCLUDES := $(PS3DEV)/ppu/include $(PS3DEV)/portlibs/ppu/include $(PS3DEV)/portlibs/ppu/include

CFLAGS := -O2 -Wall -mcpu=cell $(MACHDEP)
CXXFLAGS := $(CFLAGS) $(foreach dir,$(INCLUDES),-I$(dir))

LDFLAGS := $(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS := -ltiny3d -lrsx -lgcm_sys -lio -lsysutil -lrt -llv2 -lsysmodule -lm

LIBDIRS := $(PORTLIBS)

ifeq ($(BUILD),$(notdir $(CURDIR)))

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).elf: $(OFILES)

$(OUTPUT).self: $(OUTPUT).elf

-include $(DEPENDS)

else

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
export BUILDDIR := $(CURDIR)/$(BUILD)

CFILES := $(notdir $(wildcard $(SOURCES)/*.c))
CPPFILES := $(notdir $(wildcard $(SOURCES)/*.cpp))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  $(LIBPSL1GHT_INC) \
                  -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
                   $(LIBPSL1GHT_LIB)

.PHONY: all clean

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD) *.elf *.self *.pkg *.map
endif
