#include "AudioGui.h"
#include "AudioConvert.h"

SplitTask gTaskTable[2] = {
	{
		"None",
	},
	{
		"Sampler",
		DefineTask(Sampler)
	},
};

Sampler* __sampler;

// !! //

void Window_DropCallback(GLFWwindow* window, s32 count, char* item[]) {
	if (count == 1) {
		if (StrEndCase(item[0], ".wav")) {
			SoundFormat fmt;
			
			if (__sampler == NULL)
				return;
			
			if (__sampler->player) {
				Sound_Free(__sampler->player);
				Audio_FreeSample(&__sampler->sample);
			}
			
			Audio_InitSample(&__sampler->sample, item[0], 0);
			Audio_LoadSample(&__sampler->sample);
			
			fmt = __sampler->sample.bit == 16 ? SOUND_S16 : (__sampler->sample.bit == 32 && __sampler->sample.dataIsFloat) ? SOUND_F32 : SOUND_S32;
			
			__sampler->sample.doLoop = __sampler->sample.instrument.loop.count != 0;
			__sampler->player = Sound_Init(fmt, __sampler->sample.sampleRate, __sampler->sample.channelNum, Audio_Playback, &__sampler->sample);
			
			if (StrStr(item[0], "Sample.wav")) {
				char* ptr = StrStr(item[0], "\\Sample.wav");
				char* str;
				if (ptr == NULL) StrStr(item[0], "/Sample.wav");
				if (ptr == NULL)
					strncpy(__sampler->sampleName.txt, String_GetBasename(item[0]), 64);
				else {
					str = ptr;
					
					while (str[-1] != '/' && str[-1] != '\\' && str[-1] != '\0')
						str--;
					
					strncpy(__sampler->sampleName.txt, str, (uPtr)ptr - (uPtr)str);
				}
			} else
				strncpy(__sampler->sampleName.txt, String_GetBasename(item[0]), 64);
		}
	}
}

void Window_Update(WindowContext* winCtx) {
	GeoGrid_Update(&winCtx->geoGrid);
	Cursor_Update(&winCtx->cursor);
}

void Window_Draw(WindowContext* winCtx) {
	GeoGrid_Draw(&winCtx->geoGrid);
}

// !! //

void Sampler_Init(WindowContext* winCtx, Sampler* this, Split* split) {
	__sampler = this;
	
	this->playButton.txt = "Play Sample";
	this->playButton.toggle = true;
	
	this->sampleName.txt = Calloc(0, 66);
	this->sampleName.size = 64;
	
	this->text.txt = Calloc(0, 64);
}

void Sampler_Destroy(WindowContext* winCtx, Sampler* this, Split* split) {
	Free(this->sampleName.txt);
}

void Sampler_Update(WindowContext* winCtx, Sampler* this, Split* split) {
	Thread thread;
	AudioSampleInfo* sample = &this->sample;
	s32 y = SPLIT_ELEM_X_PADDING;
	
	// For drag n dropping
	if (split->mouseInSplit)
		__sampler = this;
	
	Element_SetRect_Multiple(split, y, 2, &this->playButton.rect, &this->sampleName.rect);
	
	if (Element_Button(&winCtx->geoGrid, split, &this->playButton)) {
		if (sample->doPlay == 0) {
			sample->doPlay = 1;
		}
		
		if (sample->doPlay < 0) {
			this->playButton.toggle = 1;
			sample->playFrame = 0;
			sample->doPlay = 0;
		}
		
		this->butTog = true;
	} else {
		if (this->butTog) {
			sample->doPlay = false;
			sample->playFrame = 0;
			this->butTog = false;
		}
	}
	
	Element_Textbox(&winCtx->geoGrid, split, &this->sampleName);
	y += SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING;
	
	sprintf(this->text.txt, "%d", sample->channelNum == 0 ? 0 : sample->playFrame / sample->channelNum);
	Element_SetRect_Multiple(split, y, 1, &this->text.rect);
	Element_Text(&winCtx->geoGrid, split, &this->text);
	
	this->y = y + SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING;
}

void Sampler_Draw(WindowContext* winCtx, Sampler* this, Split* split) {
	void* vg = winCtx->vg;
	AudioSampleInfo* sample = &this->sample;
	Rect waverect = {
		SPLIT_ELEM_X_PADDING,
		this->y,
		split->rect.w - SPLIT_ELEM_X_PADDING * 2,
		split->rect.h - (this->y + SPLIT_TEXT_H * 0.5 + SPLIT_BAR_HEIGHT)
	};
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_BASE_L1, 255, 0.5));
	nvgRoundedRect(
		vg,
		waverect.x,
		waverect.y,
		waverect.w,
		waverect.h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	if (this->sample.audio.p == NULL)
		return;
	
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 1.0f);
	
	nvgMoveTo(
		vg,
		waverect.x,
		waverect.y + waverect.h * 0.5
	);
	
	// Draw waveform
	for (s32 i = 0; i < sample->samplesNum; i++) {
		static s32 xPrev;
		f32 x = waverect.x + waverect.w * ((f32)i / (f32)sample->samplesNum);
		s32 y;
		
		if (sample->bit == 16) {
			y = waverect.y + waverect.h * 0.5 + (waverect.h * 0.5) * ((f32)sample->audio.s16[i * sample->channelNum] / __INT16_MAX__);
		} else if (sample->dataIsFloat) {
			y = waverect.y + waverect.h * 0.5 + (waverect.h * 0.5) * sample->audio.f32[i * sample->channelNum];
		} else {
			y = waverect.y + waverect.h * 0.5 + (waverect.h * 0.5) * ((f32)sample->audio.s32[i * sample->channelNum] / (f32)__INT32_MAX__);
		}
		
		if (x - xPrev < 0.25 && i > 0) {
			continue;
		}
		
		nvgLineTo(
			vg,
			x,
			y
		);
		
		xPrev = x;
	}
	
	nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.8));
	nvgStroke(vg);
	
	// Draw loop markers
	if (sample->doLoop) {
		for (s32 j = 0; j < 2; j++) {
			f32 point = j == 0 ? sample->instrument.loop.start : sample->instrument.loop.end;
			f32 mul = j == 0 ? 1.0f : -1.0f;
			f32 startX = waverect.x + waverect.w * (point / (f32)sample->samplesNum);
			f32 y = waverect.y;
			Vec2f shape[] = {
				8.0f, 0.0f,
				0.0f, 8.0f,
				0.0f, 0.0f,
			};
			
			nvgBeginPath(vg);
			nvgFillColor(vg, Theme_GetColor(THEME_RED, 255, 1.0));
			
			nvgMoveTo(
				vg,
				startX,
				y
			);
			
			for (s32 i = 0; i < 3; i++) {
				nvgLineTo(
					vg,
					shape[i].x * mul + startX,
					shape[i].y + y
				);
			}
			
			nvgFill(vg);
		}
	}
	
	// Draw play position line
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 2.5f);
	nvgMoveTo(
		vg,
		waverect.x + waverect.w * ((f32)sample->playFrame / (f32)sample->samplesNum / sample->channelNum),
		waverect.y
	);
	nvgLineTo(
		vg,
		waverect.x + waverect.w * ((f32)sample->playFrame / (f32)sample->samplesNum / sample->channelNum),
		waverect.y + waverect.h
	);
	
	nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 1.0));
	nvgStroke(vg);
	
	// Adjust play position line by clicking
	if (GeoGrid_Cursor_InRect(split, &waverect)) {
		if (winCtx->input.mouse.clickL.press) {
			f32 clickPosX = split->mousePos.x - waverect.x;
			sample->playFrame = (f32)sample->samplesNum * sample->channelNum * (clickPosX / waverect.w);
			sample->playFrame = Align(sample->playFrame, sample->channelNum);
			printf_info("Cursor %f", clickPosX / waverect.w);
		}
	}
}