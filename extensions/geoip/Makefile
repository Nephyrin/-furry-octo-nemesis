# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ../..
SRCDS_BASE = ~/srcds
HL2SDK_ORIG = ../../../hl2sdk
HL2SDK_OB = ../../../hl2sdk-ob
SOURCEMM14 = ../../../sourcemm-1.4
SOURCEMM16 = ../../../sourcemm-1.6

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = geoip

#Uncomment for Metamod: Source enabled extension
#USEMETA = true

OBJECTS = sdk/smsdk_ext.cpp extension.cpp GeoIP.c

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc-4.1

override ENGSET = false
ifeq "$(ENGINE)" "original"
	HL2SDK = $(HL2SDK_ORIG)
	HL2PUB = $(HL2SDK_ORIG)/public
	HL2LIB = $(HL2SDK_ORIG)/linux_sdk
	METAMOD = $(SOURCEMM14)
	INCLUDE += -I$(HL2SDK)/public/dlls
	SRCDS = $(SRCDS_BASE)
	override ENGSET = true
endif
ifeq "$(ENGINE)" "orangebox"
	HL2SDK = $(HL2SDK_OB)
	HL2PUB = $(HL2SDK_OB)/public
	HL2LIB = $(HL2SDK_OB)/lib/linux
	CFLAGS += -DORANGEBOX_BUILD
	METAMOD = $(SOURCEMM16)
	INCLUDE += -I$(HL2SDK)/public/game/server
	SRCDS = $(SRCDS_BASE)/orangebox
	override ENGSET = true
endif

ifeq "$(USEMETA)" "true"
	LINK_HL2 = $(HL2LIB)/tier1_i486.a vstdlib_i486.so tier0_i486.so

	LINK += $(LINK_HL2)

	INCLUDE += -I. -I.. -Isdk -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 \
		-I$(METAMOD) -I$(METAMOD)/sourcehook -I$(METAMOD)/sourcemm -I$(SMSDK)/public \
		-I$(SMSDK)/public/sourcepawn
else
	INCLUDE += -I. -I.. -Isdk -I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn
endif

LINK += -static-libgcc

CFLAGS += -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
	-D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror -mfpmath=sse \
	-msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -m32
CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS += $(C_DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS += $(C_OPT_FLAGS)
endif

ifeq "$(USEMETA)" "true"
	BIN_DIR := $(BIN_DIR).$(ENGINE)
endif

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)
ifeq "$(GCC_VERSION)" "4"
	CFLAGS += $(C_GCC4_FLAGS)
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

BINARY = $(PROJECT).ext.so

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all: check
	mkdir -p $(BIN_DIR)/sdk
	if [ "$(USEMETA)" == "true" ]; then \
		ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so; \
		ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so; \
	fi
	$(MAKE) -f Makefile extension

check:
	if [ "$(USEMETA)" == "true" ] && [ "$(ENGSET)" == "false" ]; then \
		echo "You must supply ENGINE=orangebox or ENGINE=original"; \
		exit 1; \
	fi

extension: check $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -m32 -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean: check
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/sdk/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
