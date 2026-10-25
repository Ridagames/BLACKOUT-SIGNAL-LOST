
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

INCLUDES := $(PS3DEV)/portlibs/ppu/include/freetype2

CFLAGS := -O2 -Wall -mcpu=cell $(MACHDEP) $(INCLUDE)
CXXFLAGS := $(CFLAGS)

LDFLAGS := $(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS := -lNoRSX -lgcm_sys -lrsx -lsysutil -lio -lm \
        -lfreetype -lz -lpixman-1 -lrt -llv2 \
        -lsysmodule -lpng -lpngdec -ljpgdec

LIBDIRS := $(PORTLIBS)

ifeq ($(BUILD),$(notdir $(CURDIR)))

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf

$(OUTPUT).elf: $(OFILES)

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

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  -I$(PS3DEV)/portlibs/ppu/include/freetype2 \
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
