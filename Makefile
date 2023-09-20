ifeq (,$(wildcard settings.mk))
  $(error Please run ./setup.sh to automatically install ExtLib)
endif
include settings.mk

CFLAGS       = -g -Ofast -Wall -DEXTLIB=220 -Isrc -Idlcopy/src
SRC_C        = $(shell find src/* -type f -name '*.c')
OBJ_LINUX   := $(foreach f,$(SRC_C:.c=.o),bin/linux/$f)
OBJ_WIN32   := $(foreach f,$(SRC_C:.c=.o),bin/win32/$f)

RELEASE_EXECUTABLE_LINUX := z64audio
RELEASE_EXECUTABLE_WIN32 := $(RELEASE_EXECUTABLE_LINUX).exe

default: linux

include $(PATH_EXTLIB)/ext_lib.mk

linux: $(RELEASE_EXECUTABLE_LINUX)
win32: $(RELEASE_EXECUTABLE_WIN32)

clean:
	rm -rf bin

-include $(OBJ_LINUX:.o=.d)
-include $(OBJ_WIN32:.o=.d)

$(RELEASE_EXECUTABLE_LINUX): $(OBJ_LINUX) $(ExtLib_Linux_O) $(Mp3_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(XFLAGS) $(CFLAGS)

$(RELEASE_EXECUTABLE_WIN32): $(OBJ_WIN32) $(ExtLib_Win32_O) $(Mp3_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(XFLAGS) $(CFLAGS) -D_WIN32
	@i686-w64-mingw32.static-strip --strip-unneeded $@
