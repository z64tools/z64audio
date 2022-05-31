OPT_WIN32 := -Os
OPT_LINUX := -Os

CFLAGS          = -Wall -Wno-unused-result -pthread -DEXTLIB=143
SOURCE_C       := $(shell find lib/* -maxdepth 0 -type f -name '*.c')
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)

ADPCM_C         := $(shell find sdk/adpcm/* -type f -name '*.c')
ADPCM_CW        := $(shell find sdk/adpcm/* -type f -name '*.c' -not -name 'vadpcm_dec*')
ADPCM_C_B       := $(shell find sdk/adpcm/* -type f -name '*.c' -not -name 'vadpcm_*')
ADPCM_O_LINUX   := $(foreach f,$(ADPCM_C:.c=.o), bin/linux/$f)
ADPCM_O_LINUX_B := $(foreach f,$(ADPCM_C_B:.c=.o), bin/linux/$f)
ADPCM_O_WIN32   := $(foreach f,$(ADPCM_CW:.c=.o), bin/win32/$f)
ADPCM_O_WIN32_B := $(foreach f,$(ADPCM_C_B:.c=.o), bin/win32/$f)

TABLEDESIGN_C       := $(shell find sdk/tabledesign/* -maxdepth 0 -type f -name '*.c')
TABLEDESIGN_O_LINUX := $(foreach f,$(TABLEDESIGN_C:.c=.o), bin/linux/$f)
TABLEDESIGN_O_WIN32 := $(foreach f,$(TABLEDESIGN_C:.c=.o), bin/win32/$f)
AUDIOFILE_CPP       := sdk/tabledesign/audiofile/audiofile.cpp
AUDIOFILE_O_LINUX   := $(foreach f,$(AUDIOFILE_CPP:.cpp=.o), bin/linux/$f)
AUDIOFILE_O_WIN32   := $(foreach f,$(AUDIOFILE_CPP:.cpp=.o), bin/win32/$f)

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
	$(dir $(TABLEDESIGN_O_LINUX)) \
	$(dir $(TABLEDESIGN_O_WIN32)) \
	$(dir $(ADPCM_O_LINUX)) \
	$(dir $(AUDIOFILE_O_LINUX)) \
	$(dir $(AUDIOFILE_O_WIN32)) \
	$(dir $(ADPCM_O_WIN32)) \
	$(dir $(SOURCE_O_WIN32)) \
	$(dir $(SOURCE_O_LINUX)), $(dir)))

.PHONY: clean default win lin

default: linux
all: lin-tools win-tools linux win32
lin-tools: $(ADPCM_O_LINUX) $(TABLEDESIGN_O_LINUX) $(AUDIOFILE_O_LINUX) tabledesign vadpcm_enc vadpcm_dec
win-tools: $(ADPCM_O_WIN32) $(TABLEDESIGN_O_WIN32) $(AUDIOFILE_O_WIN32) tabledesign.exe vadpcm_enc.exe
linux: lin-tools $(SOURCE_O_LINUX) z64audio
win32: win-tools $(SOURCE_O_WIN32) bin/icon.o z64audio.exe

clean:
	@echo "$(PRNT_RSET)rm $(PRNT_RSET)[$(PRNT_CYAN)$(shell find bin/* -type f)$(PRNT_RSET)]"
	@rm -f $(shell find bin/* -type f)
	@echo "$(PRNT_RSET)rm $(PRNT_RSET)[$(PRNT_CYAN)$(shell find z64audi* -type f -not -name '*.c*')$(PRNT_RSET)]"
	@rm -f z64audio *.exe
	@rm -f $(shell find *.c -type f -not -name 'z64audio.c')
	@rm -f *.bin
	@rm -f *.book
	@rm -f tabledesign
	@rm -f vadpcm_*
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

# AUDIOTOOLS LINUX

bin/linux/sdk/%.o: sdk/%.c
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(CFLAGS)

bin/linux/sdk/%.o: sdk/%.cpp
	@g++ -std=c++11 -O2 -I. -c $< -o $@

tabledesign: $(TABLEDESIGN_O_LINUX) $(AUDIOFILE_O_LINUX)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@g++ -o $@ $^ -Wall -Wno-uninitialized -O2 -lm

vadpcm_enc: bin/linux/sdk/adpcm/vadpcm_enc.o $(ADPCM_O_LINUX_B)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(CFLAGS)

vadpcm_dec: bin/linux/sdk/adpcm/vadpcm_dec.o $(ADPCM_O_LINUX_B)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(CFLAGS)

# AUDIOTOOLS WIN32

bin/win32/sdk/%.o: sdk/%.c
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(CFLAGS) -D_WIN32 -DNDEBUG

bin/win32/sdk/%.o: sdk/%.cpp
	@i686-w64-mingw32.static-g++ -std=c++11 -O2 -I. -c $< -o $@ -DNDEBUG

tabledesign.exe: $(TABLEDESIGN_O_WIN32) $(AUDIOFILE_O_WIN32)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-g++ -o $@ $^ -Wall -Wno-uninitialized -O2 -lm

vadpcm_enc.exe: bin/win32/sdk/adpcm/vadpcm_enc.o $(ADPCM_O_WIN32_B)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(CFLAGS) -D_WIN32

vadpcm_dec.exe: bin/win32/sdk/adpcm/vadpcm_dec.o $(ADPCM_O_WIN32_B)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(CFLAGS) -D_WIN32