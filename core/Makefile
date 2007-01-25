#(C)2004-2006 SourceMM Development Team
# Makefile written by David "BAILOPAN" Anderson

SMM_BASE = ../../../sourcemm
SMM_TRUNK = $(SMM_BASE)/trunk
HL2SDK = $(SMM_BASE)/hl2sdk
SMSDK = ..
SRCDS = ~/srcds

### EDIT BELOW FOR OTHER PROJECTS ###

OPT_FLAGS = -O3 -funroll-loops -s -pipe -fno-strict-aliasing
GCC4_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
DEBUG_FLAGS = -g -ggdb3
CPP = gcc-4.1
BINARY = sourcemod_mm.so

HL2PUB = $(HL2SDK)/public
HL2LIB = $(HL2SDK)/linux_sdk

OBJECTS = sourcemm_api.cpp sourcemod.cpp AdminCache.cpp CDbgReporter.cpp CLogger.cpp \
	  CPlayer.cpp CPlayerManager.cpp CTextParsers.cpp CTranslator.cpp \
	  sm_autonatives.cpp sm_memtable.cpp sm_srvcmds.cpp sm_trie.cpp \
	  sm_stringutil.cpp smn_filesystem.cpp smn_float.cpp smn_handles.cpp \
	  smn_player.cpp smn_string.cpp smn_textparse.cpp \
	  systems/ExtensionSys.cpp systems/ForwardSys.cpp systems/HandleSys.cpp \
	  systems/LibrarySys.cpp systems/PluginInfoDatabase.cpp \
	  systems/PluginSys.cpp systems/ShareSys.cpp vm/sp_vm_basecontext.cpp \
	  vm/sp_vm_engine.cpp vm/sp_vm_function.cpp
OBJECTS += zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzio.c \
	   zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c \
	   zlib/uncompr.c zlib/zutil.c

LINK = $(HL2LIB)/tier1_i486.a vstdlib_i486.so tier0_i486.so -static-libgcc

INCLUDE = -I. -I.. -I$(HL2PUB) -I$(HL2PUB)/dlls -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 \
          -I$(HL2PUB)/vstdlib -I$(HL2SDK)/tier1 -I$(SMM_TRUNK) -I$(SMM_TRUNK)/sourcehook -I$(SMM_TRUNK)/sourcemm \
	  -I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn -I$(SMSDK)/public/extensions \
	  -Isystems

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS = $(DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS = $(OPT_FLAGS)
endif

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)

CFLAGS += -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror -fPIC -msse -DSOURCEMOD_BUILD
CPPFLAGS = -Wno-non-virtual-dtor -fno-exceptions -fno-rtti

ifeq "$(GCC_VERSION)" "4"
	CPPFLAGS += $(GCC4_FLAGS)
endif

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)
OBJ_LINUX := $(OBJECTS:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/%.o: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/systems
	mkdir -p $(BIN_DIR)/vm
	mkdir -p $(BIN_DIR)/zlib
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so
	$(MAKE) sourcemod
	rm -rf $(BINARY)
	ln -sf $(BIN_DIR)/$(BINARY) $(BINARY)

sourcemod: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) $(OBJ_LINUX) $(LINK) -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:	
	$(MAKE) all DEBUG=true

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/$(BINARY)
	rm -rf Debug/*.o
	rm -rf Debug/$(BINARY)
