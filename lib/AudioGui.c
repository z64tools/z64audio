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
	
	this->zoom.end = 1.0f;
	this->zoom.vEnd = 1.0f;
	
	this->visual.findCol = Theme_GetColor(THEME_HIGHLIGHT, 25, 1.0);
}

void Sampler_Destroy(WindowContext* winCtx, Sampler* this, Split* split) {
	Free(this->sampleName.txt);
	
	if (this->player) {
		Sound_Free(this->player);
		Audio_FreeSample(&this->sample);
	}
}

void Sampler_Update(WindowContext* winCtx, Sampler* this, Split* split) {
	Thread thread;
	AudioSample* sample = &this->sample;
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

static f32 GetPos(f32 i, Rect* waverect, AudioSample* sample, Sampler* this) {
	s32 endSample = sample->samplesNum * this->zoom.vEnd;
	
	return waverect->x + waverect->w * ((f32)(i - sample->samplesNum * this->zoom.vStart) / (f32)(endSample - sample->samplesNum * this->zoom.vStart));
}

static void Sampler_Draw_Waveform_Line(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	s32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 zoomRatio = this->zoom.vEnd - this->zoom.vStart;
	f32 y;
	f32 x;
	s32 ii = 0;
	
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 1.25f);
	
	nvgMoveTo(vg, waverect->x, waverect->y + waverect->h * 0.5);
	
	for (s32 i = sample->samplesNum * this->zoom.vStart; i < endSample; i++) {
		x = GetPos(i, waverect, sample, this);
		
		if (sample->bit == 16) {
			y = (f32)sample->audio.s16[(s32)floorf(i) * sample->channelNum] / __INT16_MAX__;
		} else if (sample->dataIsFloat) {
			y = ((f32)sample->audio.f32[(s32)floorf(i) * sample->channelNum]);
		} else {
			y = (f32)sample->audio.s32[(s32)floorf(i) * sample->channelNum] / (f32)__INT32_MAX__;
		}
		
		nvgLineTo(
			vg,
			x,
			waverect->y + waverect->h * 0.5 + (waverect->h * 0.5) * y
		);
	}
	
	nvgLineTo(vg, waverect->x + waverect->w, waverect->y + waverect->h * 0.5);
	nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.95));
	nvgStroke(vg);
}

static void Sampler_Draw_Waveform_Block(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	s32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 sampleRatio = this->zoom.vEnd - this->zoom.vStart;
	f32 smplStart = sample->samplesNum * this->zoom.vStart;
	f32 smplCount = (f32)(endSample - smplStart) / waverect->w;
	
	for (s32 i = 0; i < waverect->w; i++) {
		f64 yMin = 0;
		f64 yMax = 0;
		f64 y = 0;
		s32 e = smplStart + smplCount * i * sample->channelNum;
		
		for (s32 j = 0; j < smplCount; j++) {
			if (sample->bit == 16) {
				y = (f32)sample->audio.s16[e + j];
			} else if (sample->dataIsFloat) {
				y = ((f32)sample->audio.f32[e + j]);
			} else {
				y = (f32)sample->audio.s32[e + j];
			}
			
			yMax = Max(yMax, y);
			yMin = Min(yMin, y);
		}
		
		if (sample->bit == 16) {
			yMax /= __INT16_MAX__;
			yMin /= __INT16_MAX__;
		} else if (sample->dataIsFloat == false) {
			yMax /= __INT32_MAX__;
			yMin /= __INT32_MAX__;
		}
		
		nvgBeginPath(vg);
		nvgLineCap(vg, NVG_ROUND);
		nvgStrokeWidth(vg, 1.25f);
		nvgMoveTo(
			vg,
			waverect->x + i,
			waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * Abs(yMin)
		);
		nvgLineTo(
			vg,
			waverect->x + i,
			waverect->y + waverect->h * 0.5 + (waverect->h * 0.5) * yMax + 1
		);
		nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.95));
		nvgStroke(vg);
	}
}

static void Sampler_Draw_Waveform(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	f32 zoomRatio = this->zoom.vEnd - this->zoom.vStart;
	
	this->visual.playPos = GetPos((f32)sample->playFrame / sample->channelNum, waverect, sample, this);
	
	if (sample->instrument.loop.count) {
		this->visual.loopA = GetPos((f32)sample->instrument.loop.start, waverect, sample, this);
		this->visual.loopB = GetPos((f32)sample->instrument.loop.end, waverect, sample, this);
	}
	
	if (sample->samplesNum * zoomRatio > waverect->w * 4)
		Sampler_Draw_Waveform_Block(vg, waverect, sample, this);
	else
		Sampler_Draw_Waveform_Line(vg, waverect, sample, this);
}

static void Sampler_Draw_LoopMarker(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	if (sample->doLoop) {
		for (s32 j = 0; j < 2; j++) {
			f32 point = j == 0 ? this->visual.loopA : this->visual.loopB;
			f32 mul = j == 0 ? 1.0f : -1.0f;
			f32 y = waverect->y;
			Vec2f shape[] = {
				8.0f, 0.0f,
				0.0f, 8.0f,
				0.0f, 0.0f,
			};
			
			nvgBeginPath(vg);
			nvgFillColor(vg, Theme_GetColor(THEME_RED, 255, 1.0));
			
			nvgMoveTo(
				vg,
				point,
				y
			);
			
			for (s32 i = 0; i < 3; i++) {
				nvgLineTo(
					vg,
					shape[i].x * mul + point,
					shape[i].y + y
				);
			}
			
			nvgFill(vg);
		}
	}
}

static void Sampler_Draw_PlayPosLine(void* vg, Rect* waverect, Rect* finder, AudioSample* sample, Sampler* this) {
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_RED, 255, 1.25));
	nvgRoundedRect(
		vg,
		this->visual.playPos,
		waverect->y + 2,
		2,
		waverect->h - 4 - finder->h,
		SPLIT_ROUND_R
	);
	
	nvgFill(vg);
}

void Sampler_Draw(WindowContext* winCtx, Sampler* this, Split* split) {
	void* vg = winCtx->vg;
	AudioSample* sample = &this->sample;
	Rect waverect = {
		SPLIT_ELEM_X_PADDING * 2,
		this->y,
		split->rect.w - SPLIT_ELEM_X_PADDING * 4,
		split->rect.h - (this->y + SPLIT_TEXT_H * 0.5 + SPLIT_BAR_HEIGHT)
	};
	Rect finder = {
		waverect.x + 2 + waverect.w * this->zoom.start,
		waverect.y + waverect.h - 16 - 2,
		waverect.w * this->zoom.end - (waverect.w * this->zoom.start) - 4,
		16,
	};
	u32 block = 0;
	f32 mouseX = Clamp((f32)(split->mousePos.x - waverect.x) / waverect.w, 0.0, 1.0);
	
	nvgBeginPath(vg);
	nvgFillPaint(
		vg,
		nvgBoxGradient(
			vg,
			waverect.x,
			waverect.y,
			waverect.w,
			waverect.h,
			0,
			8,
			Theme_GetColor(THEME_BASE_L1, 255, 1.05),
			Theme_GetColor(THEME_BASE_L1, 255, 0.45)
		)
	);
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
	
	if (GeoGrid_Cursor_InRect(split, &finder) || this->zoom.modifying) {
		block = true;
		
		Theme_SmoothStepToCol(&this->visual.findCol, Theme_GetColor(THEME_HIGHLIGHT, 175, 1.0), 0.25, 1.0, 0.0);
		
		if (winCtx->input.mouse.clickL.hold) {
			f32 xmod = (mouseX - this->mousePrevX);
			if (this->zoom.modifying == 0) {
				s32 chk = Abs(split->mousePos.x - finder.x);
				s32 a = chk;
				
				chk = Min(chk, Abs(split->mousePos.x - (finder.x + finder.w)));
				
				if (chk < 10) {
					if (chk == a)
						this->zoom.modifying = -1;
					else
						this->zoom.modifying = 1;
				} else {
					this->zoom.modifying = 8;
				}
			}
			
			switch (this->zoom.modifying) {
				case -1:
					this->zoom.start = ClampMax(mouseX, this->zoom.end - 0.00001);
					break;
				case 1:
					this->zoom.end = ClampMin(mouseX, this->zoom.start + 0.00001);
					break;
				case 8:
					if (this->zoom.start + xmod >= 0 && this->zoom.end + xmod <= 1.0) {
						this->zoom.start += xmod;
						this->zoom.end += xmod;
					}
					break;
			}
		} else
			this->zoom.modifying = 0;
	} else {
		Theme_SmoothStepToCol(&this->visual.findCol, Theme_GetColor(THEME_HIGHLIGHT, 25, 1.0), 0.25, 1.0, 0.0);
	}
	
	if (GeoGrid_Cursor_InRect(split, &waverect) && block == false) {
		f32 relPosX;
		f32 zoomRatio = this->zoom.end - this->zoom.start;
		f32 mod = (mouseX - this->mousePrevX) * zoomRatio;
		f32 scrollY = winCtx->input.mouse.scrollY;
		
		relPosX = this->zoom.start + ((f32)(split->mousePos.x - waverect.x) / waverect.w) * zoomRatio;
		
		if (winCtx->input.mouse.clickMid.hold) {
			if (this->zoom.start - mod >= 0 && this->zoom.end - mod <= 1.0) {
				this->zoom.start -= mod;
				this->zoom.end -= mod;
			}
		}
		
		if (scrollY) {
			if (scrollY > 0) {
				if (zoomRatio > 0.00001) {
					Math_DelSmoothStepToD(&this->zoom.start, relPosX, 0.15, zoomRatio, 0.0);
					Math_DelSmoothStepToD(&this->zoom.end, relPosX, 0.15, zoomRatio, 0.0);
				}
			} else {
				this->zoom.start += (0.0 - this->zoom.start) * zoomRatio;
				this->zoom.end += (1.0 - this->zoom.end) * zoomRatio;
			}
		}
		
		if (this->zoom.start >= this->zoom.end) {
			f32 median = this->zoom.start + this->zoom.end;
			
			median *= 0.5;
			this->zoom.start = median - 0.00001;
			this->zoom.end = median + 0.00001;
		}
	}
	
	this->zoom.start = Clamp(this->zoom.start, 0.0, 0.99999);
	this->zoom.end = Clamp(this->zoom.end, 0.00001, 1.0);
	
	Math_DelSmoothStepToD(&this->zoom.vStart, this->zoom.start, 0.25, 0.25, 0.0);
	Math_DelSmoothStepToD(&this->zoom.vEnd, this->zoom.end, 0.25, 0.25, 0.0);
	
	Sampler_Draw_Waveform(vg, &waverect, sample, this);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->visual.findCol);
	nvgRoundedRect(
		vg,
		finder.x,
		finder.y,
		finder.w,
		finder.h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	Sampler_Draw_LoopMarker(vg, &waverect, sample, this);
	Sampler_Draw_PlayPosLine(vg, &waverect, &finder, sample, this);
	
	// Adjust play position line by clicking
	if (GeoGrid_Cursor_InRect(split, &waverect) && block == false) {
		if (winCtx->input.mouse.clickL.press) {
			f32 clickPosX = split->mousePos.x - waverect.x;
			sample->playFrame = sample->samplesNum * sample->channelNum * this->zoom.start + (f32)sample->samplesNum * sample->channelNum * (this->zoom.end - this->zoom.start) * (clickPosX / waverect.w);
			sample->playFrame = Align(sample->playFrame, sample->channelNum);
			printf_info("Cursor %f", clickPosX / waverect.w);
		}
	}
	
	this->mousePrevX = mouseX;
}