OPT_WIN32 := -Os
OPT_LINUX := -Os

CFLAGS          = -Wall -Wno-unused-result -pthread -DEXTLIB=148
SOURCE_C       := $(shell find lib/* -maxdepth 0 -type f -name '*.c')
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)

ExtLibDep := $(C_INCLUDE_PATH)/ExtLib.h

PRNT_DGRY := \e[90;2m
PRNT_GRAY := \e[0;90m
PRNT_DRED := \e[91;2m
PRNT_REDD := \e[0;91m
PRNT_GREN := \e[0;92m
PRNT_YELW := \e[0;93m
PRNT_BLUE := \e[0;94m
PRNT_PRPL := \e[0;95m
PRNT_CYAN := \e[0;96m
PRNT_RSET := \e[m

include $(C_INCLUDE_PATH)/ExtLib.mk

# bin/win32/lib/AudioSDK.o: CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O2
# bin/linux/lib/AudioSDK.o: CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O2

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(SOURCE_O_WIN32)) \
	$(dir $(SOURCE_O_LINUX)), $(dir)))

.PHONY: clean default linux win32

default: linux
all: linux win32
linux: $(SOURCE_O_LINUX) z64audio
win32: $(SOURCE_O_WIN32) bin/icon.o z64audio.exe

clean:
	@rm -f z64audio *.exe
	@rm -f -R bin/*

# LINUX
bin/linux/lib/External.o: lib/External.c $(ExtLibDep) $(C_INCLUDE_PATH)/ExtLib.c
bin/linux/%.o: %.c %.h $(ExtLibDep)
bin/linux/%.o: %.c $(ExtLibDep)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS)

z64audio: z64audio.c $(SOURCE_O_LINUX) $(ExtLib_Linux_O) $(Mp3_Linux_O) $(Audio_Linux_O) $(ExtGui_Linux_O)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(OPT_LINUX) $(CFLAGS) -lm -Wl,--no-as-needed -ldl -lGL $(ExtGui_Linux_Flags)

# WINDOWS32
bin/win32/lib/External.o: lib/External.c $(ExtLibDep) $(C_INCLUDE_PATH)/ExtLib.c
bin/win32/%.o: %.c %.h $(ExtLibDep)
bin/win32/%.o: %.c $(ExtLibDep)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32

bin/icon.o: lib/icon.rc lib/icon.ico
	@i686-w64-mingw32.static-windres -o $@ $<

z64audio.exe: z64audio.c bin/icon.o $(SOURCE_O_WIN32) $(ExtLib_Win32_O) $(Mp3_Win32_O) $(Audio_Win32_O) $(ExtGui_Win32_O)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(OPT_WIN32) $(CFLAGS) -lm -D_WIN32 $(ExtGui_Win32_Flags)
