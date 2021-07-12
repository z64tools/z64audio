#ifndef __Z64AUDIO_GUI_H__
#define __Z64AUDIO_GUI_H__

#include "z64snd.h"
#include "lib.h"

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
	Vec2s pos;
	Vec2s window;
	Vec2s scale;
	struct {
		Vec2s pos;
		Vec2s scale;
	} target;
	AppState state;
	u32      frame;
	struct {
		char file[FILENAME_BUFFER / 2];
	} input;
} z64audio;

static z64audio gApp = {
	.scale = {
		.x = 256,
		.y = 64,
	},
	.window = {
		.x = 256,
		.y = 64,
	},
	.target = {
		.scale = {
			.x = 256,
			.y = 64,
		},
	},
	.state = Z64WINSTATE_INPUT
};

static SWindowData* sWinData;
static SWindowData_Win* sWinDataWin;

static struct wowGui_window sBgBack = {
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

static struct wowGui_fileDropper sDropBox = {
	.label = "Input file"
	, .labelWidth = 64
	, .filenameWidth = 64 + 52
	, .extension = "wav,aiff,aifc"
	, .isOptional = 0
	, .isCreateMode = 0 /* ask for save file name */
};

void wowGui_update_window(s16 x, s16 y, s16 sx, s16 sy) {
	RECT pos = { 0 };
	
	GetWindowRect(sWinDataWin->window, &pos);
	MoveWindow(sWinDataWin->window, pos.left + x, pos.top + y, sx, sy + 29, TRUE);
	sWinData->window_height = sy;
	sWinData->buffer_height = sy;
	sWinData->dst_height = sy;
	sWinData->factor_height = sy;
	__wowGui_mfbWinH = sy;
	wowGui.viewport.h = sy;
}

void Draw_Background() {
	wowGui_viewport(0, 0, gApp.scale.x, gApp.scale.y);
	
	if (wowGui_window(&sBgBack)) {
		wowGui_window_end();
	}
	
	Draw_StateInput();
	
	Draw_Corner();
}

void Draw_StateInput() {
	if (gApp.input.file[0] == 0) {
		wowGui_padding(24, 24);
		wowGui_columns(4);
		__wowGui_redraw = 1;
		if (wowGui_fileDropper(&sDropBox)) {
			if (!wowGui_fileDropper_filenameIsEmpty(&sDropBox)) {
				gApp.target.scale.y = gApp.scale.y * 5;
			}
			wowGui_window_end();
		}
	}
}

void Draw_Corner() {
	for(s32 y = 0; y < gApp.scale.y; y++) {
		for (s32 x = 0; x < gApp.scale.x; x++) {
			if (!(x > 12 && x < gApp.scale.x - 12) || !(y > 12 && y < gApp.scale.y - 12))
				((u32*)g_buffer->pixels)[x + (gApp.scale.x * y)] = 0xFF301818;
		}
	}
}

#endif