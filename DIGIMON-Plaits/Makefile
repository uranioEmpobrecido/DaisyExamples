# Project Name
TARGET = Digimon-poly-plaits

#required for the compiler to generate symbols for debugging
DEBUG = 1
OCD_DIR = $(OPENOCD_SCRIPTS)/

# Packages to build
PACKAGES       = plaits \
		plaits/drivers \
		plaits/dsp \
		plaits/dsp/fx \
		plaits/dsp/drum_modelling \
		plaits/dsp/engine \
		plaits/dsp/physical_modelling \
		plaits/dsp/speech \
		stmlib/dsp \
		stmlib/utils \
		stmlib/system \
		Processing
RESOURCES      = plaits

# Sources
CPP_SOURCES = Daisy-Plaits.cpp
CC_SOURCES += $(wildcard $(patsubst %,%/*.cc,$(PACKAGES)))
CPP_SOURCES += $(wildcard $(patsubst %,%/*.cpp,$(PACKAGES)))
#CPP_SOURCES += $(notdir $(wildcard $(patsubst %,%/*.cc,$(RESOURCES))))

# Headers
#C_INCLUDES += -I$(CURDIR)
C_INCLUDES += -Iplaits
C_INCLUDES += -I.

# Library Locations
DAISYSP_DIR = $(DAISYSPDIR)
LIBDAISY_DIR = $(LIBDAISYDIR)

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

info:
	echo $(CPP_SOURCES) $(CC_SOURCES)

include $(SYSTEM_FILES_DIR)/Makefile

