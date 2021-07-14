#ifndef __Z64AUDIO_GUI_H__
#define __Z64AUDIO_GUI_H__

#include "z64snd.h"
#include "lib.h"

typedef void (* GuiFunc)(void);

typedef enum {
	Z64WINSTATE_INPUT = 0
	, Z64WINSTATE_OUTPUT
	, Z64WINSTATE_BANK
} AppState;

typedef struct {
	s16 x;
	s16 y;
} Vec2s;

typedef struct {
	GuiFunc  func;
	Vec2s    pos;
	Vec2s    window;
	Vec2s    scale;
	Vec2s    targetScale;
	Vec2s    lastScale;
	AppState state;
	u32      frame;
	struct {
		char file[3][FILENAME_BUFFER];
		char output[3][FILENAME_BUFFER];
		s8   fileCount;
	} files;
} z64audio;

void Gui_InputFile();
void Gui_OutputFile();

static z64audio this = {
	.scale = {
		.x = 320,
		.y = 64,
	},
	.window = {
		.x = 320,
		.y = 64,
	},
	.targetScale = {
		.x = 320,
		.y = 64,
	},
	.state = Z64WINSTATE_INPUT
};

static SWindowData* sWinData;
static SWindowData_Win* sWinDataWin;

static struct wowGui_window sWindowBackground = {
	.rect = {
		.x = 0
		, .y = 0
		, .w = 32
		, .h = 32
	}
	, .color = 0x181818FF // sides 0x301818FF
	, .not_scrollable = 1
	, .scroll_valuebox = 1
};

void wowGui_update_window(s16 x, s16 y, s16 sx, s16 sy) {
	RECT pos = { 0 };
	
	GetWindowRect(sWinDataWin->window, &pos);
	
	MoveWindow(sWinDataWin->window, pos.left + x, pos.top + y, sx + 2, sy + 29, TRUE);
	
	sWinData->window_height = sy;
	sWinData->buffer_height = sy;
	sWinData->window_width = sx;
	sWinData->buffer_width = sx;
}

void Setup_GuiFunc(void* func) {
	this.func = func;
}

void Draw_Corner() {
	for(s32 y = 0; y < this.scale.y; y++) {
		for (s32 x = 0; x < this.scale.x; x++) {
			if (!(x > 12 && x < this.scale.x - 12) || !(y > 12 && y < this.scale.y - 12))
				((u32*)g_buffer->pixels)[x + (this.scale.x * y)] = 0xFF301818;
		}
	}
}

static struct wowGui_fileDropper sGetFileBox = {
	.label = "Input file"
	, .labelWidth = 64 + 32 - 24
	, .filenameWidth = 64 + 52 + 32 + 24
	, .extension = "wav,aiff,aifc"
	, .isOptional = 0
	, .isCreateMode = 0 /* ask for save file name */
};

static struct wowGui_fileDropper sSaveFileBox[3] = {
	{
		.label = "Primary"
		, .labelWidth = 64 + 32 - 24
		, .filenameWidth = 64 + 32 + 24
		, .extension = "bin"
		, .isOptional = 0
		, .isCreateMode = 1
	},
	{
		.label = "Secondary"
		, .labelWidth = 64 + 32 - 24
		, .filenameWidth = 64 + 52 + 32 + 24
		, .extension = "bin"
		, .isOptional = 0
		, .isCreateMode = 1
	},
	{
		.label = "Previous"
		, .labelWidth = 64 + 32 - 24
		, .filenameWidth = 64 + 52 + 32 + 24
		, .extension = "bin"
		, .isOptional = 0
		, .isCreateMode = 1
	},
};

void Gui_InputFile() {
	if (wowGui_fileDropper(&sGetFileBox)) {
		if (!wowGui_fileDropper_filenameIsEmpty(&sGetFileBox)) {
			char* files[3] = { 0 };
			this.files.fileCount = File_GetAndSort(sGetFileBox.filename, files);
			if (this.files.fileCount <= 0) {
				wowGui_popup(WOWGUI_POPUP_ICON_ERR, 0, WOWGUI_POPUP_NO, "Error: InputFile", "Could not end up with any files for some reason");
			} else {
				this.targetScale.y = this.scale.y + 24 * (1 + this.files.fileCount);
				this.lastScale = this.scale;
				
				for (s32 i = 0; i < this.files.fileCount; i++) {
					strcpy(this.files.file[i], files[i]);
				}
				
				Setup_GuiFunc(Gui_OutputFile);
			}
		}
	}
}

const char fileExtension[4][6] = {
	".none",
	".wav",
	".aiff",
	".aifc"
};

static struct wowGui_fileDropper sInstFileBox = {
	.label = "Instrument"
	, .labelWidth = 64 + 32 - 24
	, .filenameWidth = 64 + 52 + 32 + 24
	, .extension = "tsv"
	, .isOptional = 0
	, .isCreateMode = 1
};

void Gui_OutputFile() {
	static char fname[4][FILENAME_BUFFER] = { 0 };
	static char path[4][FILENAME_BUFFER] = { 0 };
	
	for(s32 i = 0; i < this.files.fileCount; i++) {
		if (wowGui_fileDropper(&sSaveFileBox[i])) {
			if (!wowGui_fileDropper_filenameIsEmpty(&sSaveFileBox[i])) {
				strcpy(this.files.output[i], sSaveFileBox[i].filename);
				GetFilename(this.files.output[i], fname[i], path[i]);
				GetFilename(this.files.file[i], fname[i], NULL);
			} else {
				this.targetScale.y = this.window.y;
				sGetFileBox.filename = 0;
				sSaveFileBox[0].filename = 0;
				sSaveFileBox[1].filename = 0;
				sSaveFileBox[2].filename = 0;
				Setup_GuiFunc(Gui_InputFile);
			}
		}
	}
	
	if (wowGui_fileDropper(&sInstFileBox)) {
		if (!wowGui_fileDropper_filenameIsEmpty(&sInstFileBox)) {
		}
	}
	
	char proc[] = "[y]";
	
	if (wowGui_clickable(proc)) {
		for (s32 i = 0; i < this.files.fileCount; i++) {
			if (sSaveFileBox[i].filename[0] == 0 || sInstFileBox.filename[0] == 0) {
				wowGui_popup(WOWGUI_POPUP_ICON_ERR, 0, 0, "Error", "Wow!");
				
				return;
			}
			gAudioState.cleanState.aifc = true;
			gAudioState.cleanState.aiff = false;
			gAudioState.cleanState.table = true;
			for (s32 i = 0; i < this.files.fileCount; i++) {
				switch (gAudioState.ftype) {
				    case NONE:
					    PrintFail("File you've given isn't something I can work with sadly");
					    break;
					    
				    case WAV:
					    DebugPrint("switch WAV");
					    Audio_WavConvert(this.files.file[i], &fname[i][0], &path[i][0], i);
					    if (gAudioState.mode == Z64AUDIOMODE_WAV_TO_AIFF)
						    break;
				    case AIFF:
					    DebugPrint("switch AIFF");
					    Audio_AiffConvert(this.files.file[i], &fname[i][0], &path[i][0], i);
					    if (gAudioState.mode == Z64AUDIOMODE_WAV_TO_AIFC)
						    break;
				    case AIFC:
					    DebugPrint("switch AIFC");
					    Audio_AifcParseZzrtl(this.files.file[i], &fname[i][0], &path[i][0], i);
				}
				
				Audio_Clean(fname[i], path[i]);
			}
		}
		char* file[3] = {
			this.files.file[0],
			this.files.file[1],
			this.files.file[2],
		};
		
		for(s32 i = 0; i < this.files.fileCount; i++) {
			s32 h = strlen(path[i]);
			h--;
			while (path[i][h - 1] != '/' && path[i][h - 1] != '\\' ) {
				h--;
			}
			s32 e = h;
			while (path[i][e] != ' ' && path[i][e] != '-') {
				e++;
			}
			
			if (path[h] >= '0' || path[h] <= '9') {
				if (e - h == 1) {
					gAudioState.sampleID[i] += (path[i][h] - '0');
				} else if (e - h == 2) {
					gAudioState.sampleID[i] += (path[i][h + 1] - '0');
					gAudioState.sampleID[i] += (path[i][h] - '0') * 10;
				} else if (e - h == 3) {
					gAudioState.sampleID[i] += (path[i][h + 2] - '0');
					gAudioState.sampleID[i] += (path[i][h + 1] - '0') * 10;
					gAudioState.sampleID[i] += (path[i][h] - '0') * 100;
				}
			}
		}
		
		GetFilename(sInstFileBox.filename, fname[3], path[3]);
		Audio_GenerateInstrumentConf(file[0], path[3], this.files.fileCount, sInstFileBox.filename);
		
		wowGui_popup(WOWGUI_POPUP_ICON_INFO, 0, 0, "Done", "Files have been created!");
	}
}

#endif