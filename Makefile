CFLAGS := -Os -s -flto -DNDEBUG -Wall -Wno-unused-result
SOURCE_C       := $(shell find lib/* -maxdepth 0 -type f -name '*.c')
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)

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

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, $(dir $(SOURCE_O_WIN32)) $(dir $(SOURCE_O_LINUX)), $(dir)))

.PHONY: clean default win lin

default: lin

lin: $(SOURCE_O_LINUX) z64audio
win: $(SOURCE_O_WIN32) z64audio.exe

clean:
	@echo "$(PRNT_RSET)rm $(PRNT_RSET)[$(PRNT_CYAN)$(shell find bin/* -type f)$(PRNT_RSET)]"
	@rm -f $(shell find bin/* -type f)
	@echo "$(PRNT_RSET)rm $(PRNT_RSET)[$(PRNT_CYAN)$(shell find z64audi* -type f -not -name '*.c')$(PRNT_RSET)]"
	@rm -f z64audio z64audio.exe

# LINUX
bin/linux/%.o: %.c %.h
	@echo "$(PRNT_RSET)Linux: $(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(CFLAGS)

z64audio: z64audio.c $(SOURCE_O_LINUX)
	@echo "$(PRNT_RSET)Linux: $(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(CFLAGS) -lm

# WINDOWS32
bin/win32/%.o: %.c %.h
	@echo "$(PRNT_RSET)Win32: $(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(CFLAGS) -D_WIN32

z64audio.exe: z64audio.c $(SOURCE_O_WIN32)
	@echo "$(PRNT_RSET)Win32: $(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(CFLAGS) -lm -D_WIN32