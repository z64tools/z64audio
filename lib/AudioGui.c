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
		if (StrEndCase(item[0], ".wav") || StrEndCase(item[0], ".aiff") || StrEndCase(item[0], ".mp3")) {
			SoundFormat fmt;
			char* fileName = item[0];
			FILE* tst;
			
			if (__sampler == NULL)
				return;
			
			tst = fopen(fileName, "r");
			if (tst == NULL)
				return;
			fclose(tst);
			
			if (__sampler->player) {
				Sound_Free(__sampler->player);
				Audio_FreeSample(&__sampler->sample);
			}
			
			Audio_InitSample(&__sampler->sample, fileName, 0);
			Log("Input File %s", fileName);
			Audio_LoadSample(&__sampler->sample);
			
			fmt = __sampler->sample.bit == 16 ? SOUND_S16 : (__sampler->sample.bit == 32 && __sampler->sample.dataIsFloat) ? SOUND_F32 : SOUND_S32;
			
			__sampler->player = Sound_Init(fmt, __sampler->sample.sampleRate, __sampler->sample.channelNum, Audio_Playback, &__sampler->sample);
			
			if (StrStr(fileName, "Sample.wav")) {
				char* ptr = StrStr(fileName, "\\Sample.wav");
				char* str;
				if (ptr == NULL) StrStr(fileName, "/Sample.wav");
				if (ptr == NULL)
					strncpy(__sampler->sampleName.txt, Basename(fileName), 64);
				else {
					str = ptr;
					
					while (str[-1] != '/' && str[-1] != '\\' && str[-1] != '\0')
						str--;
					
					strncpy(__sampler->sampleName.txt, str, (uPtr)ptr - (uPtr)str);
				}
			} else
				strncpy(__sampler->sampleName.txt, Basename(fileName), 64);
			
			__sampler->zoom.end = 1.0;
			__sampler->zoom.start = 0.0;
			
			glfwFocusWindow(window);
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
	
	this->setLoopButton.txt = "Set Loop";
	this->clearLoopButton.txt = "Clear Loop";
	
	this->sampleName.txt = Calloc(0, 66);
	this->sampleName.size = 64;
	
	this->textInfo.txt = Calloc(0, 128);
	
	this->zoom.end = 1.0f;
	this->zoom.vEnd = 1.0f;
	
	this->visual.findCol = Theme_GetColor(THEME_HIGHLIGHT, 25, 1.0);
}

void Sampler_Destroy(WindowContext* winCtx, Sampler* this, Split* split) {
	Free(this->sampleName.txt);
	Free(this->textInfo.txt);
	
	if (this->player) {
		Sound_Free(this->player);
		Audio_FreeSample(&this->sample);
	}
}

void Sampler_Update(WindowContext* winCtx, Sampler* this, Split* split) {
	AudioSample* sample = &this->sample;
	s32 y = SPLIT_ELEM_X_PADDING;
	
	// For drag n dropping
	if (split->mouseInSplit)
		__sampler = this;
	
	this->setLoopButton.isDisabled = false;
	if (sample->selectStart == sample->selectEnd)
		this->setLoopButton.isDisabled = true;
	
	this->clearLoopButton.isDisabled = false;
	if (sample->instrument.loop.count == 0)
		this->clearLoopButton.isDisabled = true;
	
	Element_SetRect_Multiple(
		split,
		y,
		&this->playButton.rect,
		0.20,
		NULL,
		0.20,
		&this->sampleName.rect,
		0.60
	);
	
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
			this->butTog = false;
		}
	}
	
	Element_Textbox(&winCtx->geoGrid, split, &this->sampleName);
	
	y += SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING;
	
	Element_SetRect_Multiple(
		split,
		y,
		&this->textInfo.rect,
		0.40,
		NULL,
		0.20,
		&this->setLoopButton.rect,
		0.20,
		&this->clearLoopButton.rect,
		0.20
	);
	
	if (Element_Button(&winCtx->geoGrid, split, &this->clearLoopButton)) {
		sample->instrument.loop.start = 0;
		sample->instrument.loop.end = 0;
		sample->instrument.loop.count = 0;
	}
	
	if (Element_Button(&winCtx->geoGrid, split, &this->setLoopButton)) {
		if (sample->selectStart && sample->selectEnd) {
			sample->instrument.loop.start = sample->selectStart;
			sample->instrument.loop.end = sample->selectEnd;
			sample->instrument.loop.count = -1;
			
			sample->selectStart = sample->selectEnd = 0;
		}
	}
	
	const char* bit[] = {
		"-int",
		"-float"
	};
	
	sprintf(this->textInfo.txt, "%d %d", this->sample.sampleRate, this->sample.bit);
	if (this->sample.bit == 32) {
		catprintf(this->textInfo.txt, "%s", bit[this->sample.dataIsFloat]);
	}
	if (this->sample.selectStart != this->sample.selectEnd) {
		catprintf(this->textInfo.txt, "    Selection: %d / %-7d", this->sample.selectStart, this->sample.selectEnd);
	}
	
	Element_Text(&winCtx->geoGrid, split, &this->textInfo);
	
	if (Input_GetKey(KEY_SPACE)->press) {
		if (sample->doPlay == 1) {
			sample->doPlay = 0;
			this->playButton.toggle = 1;
		} else if (sample->doPlay == 0)
			this->playButton.toggle = 2;
	}
	
	this->waveFormPos = y + SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING;
}

static f64 Sampler_GetSamplePos(f64 i, Rect* waverect, AudioSample* sample, Sampler* this) {
	f64 endSample = sample->samplesNum * this->zoom.vEnd;
	
	return waverect->x + waverect->w * ((i - sample->samplesNum * this->zoom.vStart) / (f64)(endSample - sample->samplesNum * this->zoom.vStart));
}

static void Sampler_Draw_WaveRect(void* vg, Rect* waverect) {
	nvgBeginPath(vg);
	nvgFillPaint(
		vg,
		nvgBoxGradient(
			vg,
			waverect->x,
			waverect->y,
			waverect->w,
			waverect->h,
			0,
			8,
			Theme_GetColor(THEME_BASE_L1, 255, 1.05),
			Theme_GetColor(THEME_BASE_L1, 255, 0.45)
		)
	);
	nvgRoundedRect(
		vg,
		waverect->x,
		waverect->y,
		waverect->w,
		waverect->h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
}

static void Sampler_Draw_Waveform_Spline(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	f32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 smplPixelRatio = (endSample - sample->samplesNum * this->zoom.vStart) / waverect->w;
	
	for (s32 s = sample->channelNum; s > 0; s--) {
		f32 y;
		f32 x;
		f32 k = 0;
		s32 m = 0;
		
		nvgBeginPath(vg);
		nvgLineCap(vg, NVG_ROUND);
		nvgStrokeWidth(vg, 1.25f);
		
		for (s32 i = sample->samplesNum * this->zoom.vStart; i < endSample;) {
			f32 yl[4];
			
			x = Sampler_GetSamplePos(i + k, waverect, sample, this);
			
			if (!IsBetween(x, waverect->x, waverect->x + waverect->w))
				goto adv;
			
			for (s32 j = -1; j < 3; j++) {
				s32 smpidclmp = Clamp(i + j, 0, sample->samplesNum - 1);
				
				if (sample->bit == 16) {
					yl[j + 1] = (f32)sample->audio.s16[smpidclmp * sample->channelNum + s - 1] / __INT16_MAX__;
				} else if (sample->dataIsFloat) {
					yl[j + 1] = ((f32)sample->audio.f32[smpidclmp * sample->channelNum + s - 1]);
				} else {
					yl[j + 1] = (f32)sample->audio.s32[smpidclmp * sample->channelNum + s - 1] / (f32)__INT32_MAX__;
				}
			}
			
			y = Math_Spline_Audio(k, yl[0], yl[1], yl[2], yl[3]);
			
			y = Clamp(y, -1.0, 1.0);
			
			if (m == 0) {
				m++;
				nvgMoveTo(
					vg,
					x,
					waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * y
				);
			} else {
				nvgLineTo(
					vg,
					x,
					waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * y
				);
			}
			
adv:
			k += smplPixelRatio / 8;
			if (!(k < 1.0)) {
				i += floorf(k);
				k -= floorf(k);
			}
		}
		
		if (s == 1)
			nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.95));
		else
			nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.75));
		nvgStroke(vg);
	}
}

static void Sampler_Draw_Waveform_Block(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	s32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 smplStart = sample->samplesNum * this->zoom.vStart;
	f32 smplCount = (f32)(endSample - smplStart) / waverect->w;
	
	for (s32 i = 0; i < waverect->w; i++) {
		f64 yMin = 0;
		f64 yMax = 0;
		f64 y = 0;
		s32 e = smplStart + smplCount * i;
		
		for (s32 j = 0; j < smplCount; j++) {
			if (sample->bit == 16) {
				y = (f32)sample->audio.s16[(e + j) * sample->channelNum];
			} else if (sample->dataIsFloat) {
				y = ((f32)sample->audio.f32[(e + j) * sample->channelNum]);
			} else {
				y = (f32)sample->audio.s32[(e + j) * sample->channelNum];
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
		nvgLineCap(vg, NVG_SQUARE);
		nvgStrokeWidth(vg, 1.75);
		
		yMax = Clamp(yMax, -1.0, 1.0);
		yMin = Clamp(yMin, -1.0, 1.0);
		
		nvgMoveTo(
			vg,
			waverect->x + i,
			waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * yMin - 0.1
		);
		nvgLineTo(
			vg,
			waverect->x + i,
			waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * yMax + 0.1
		);
		
		nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.95));
		nvgStroke(vg);
	}
}

static void Sampler_Draw_LoopMarker(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	if (sample->instrument.loop.count) {
		for (s32 j = 0; j < 2; j++) {
			f32 point = j == 0 ? this->visual.loopA : this->visual.loopB;
			f32 mul = j == 0 ? 1.0f : -1.0f;
			f32 y = waverect->y;
			Vec2f shape[] = {
				{ 8.0f, 0.0f },
				{ 0.0f, 8.0f },
				{ 0.0f, 0.0f },
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
			
			nvgBeginPath(vg);
			nvgFillColor(vg, Theme_GetColor(THEME_RED, 255, 1.0));
			nvgRoundedRect(
				vg,
				point,
				waverect->y + 4,
				j == 0 ? 2 : -2,
				waverect->h - 8 * 2 + 4,
				SPLIT_ROUND_R
			);
			
			nvgFill(vg);
		}
	}
}

static void Sampler_Draw_Selection(void* vg, Rect* waverect, Rect* finder, AudioSample* sample, Sampler* this) {
	if (sample->selectStart == sample->selectEnd)
		return;
	
	Rect rect = {
		this->visual.selectStart,
		waverect->y + 8,
		this->visual.selectStart != this->visual.selectEnd ? this->visual.selectEnd - this->visual.selectStart + 2 : 2,
		waverect->h - 8 * 2
	};
	
	rect.w += rect.x - ClampMin(rect.x, waverect->x);
	rect.x = ClampMin(rect.x, waverect->x);
	
	rect.w = ClampMax(rect.w, waverect->w - rect.x + 12);
	
	if (rect.w <= 0 || rect.x > waverect->x + waverect->w)
		return;
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 127, 1.25));
	nvgRoundedRect(
		vg,
		rect.x,
		rect.y,
		rect.w,
		rect.h,
		SPLIT_ROUND_R
	);
	
	nvgFill(vg);
}

static void Sampler_Draw_Position(void* vg, Rect* waverect, Rect* finder, AudioSample* sample, Sampler* this) {
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 255, 1.25));
	nvgRoundedRect(
		vg,
		this->visual.playPos,
		waverect->y + 8,
		2,
		waverect->h - 8 * 2,
		SPLIT_ROUND_R
	);
	
	nvgFill(vg);
}

static void Sampler_Draw_Grid(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	f32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 smplPixelRatio = (endSample - sample->samplesNum * this->zoom.vStart) / waverect->w;
	s32 o = 0;
	
	for (f32 i = 0; i < 1.0; i += 0.1) {
		f32 y = waverect->y + waverect->h * i;
		f32 a = powf(Abs(0.5 - i) * 2, 8);
		nvgBeginPath(vg);
		nvgLineCap(vg, NVG_ROUND);
		nvgStrokeWidth(vg, 1.25f);
		nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 50 - a * 35, 1.0));
		nvgMoveTo(vg, waverect->x + 2, y);
		nvgLineTo(vg, waverect->x + waverect->w - 4, y);
		nvgStroke(vg);
	}
	
	if (smplPixelRatio < 0.5) {
		for (s32 i = sample->samplesNum * this->zoom.vStart; i < endSample; i++) {
			f32 x = Sampler_GetSamplePos(i, waverect, sample, this);
			
			nvgBeginPath(vg);
			nvgLineCap(vg, NVG_ROUND);
			nvgStrokeWidth(vg, 1.25f);
			nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 25, 1.0));
			nvgMoveTo(vg, x, waverect->y + 2);
			nvgLineTo(vg, x, waverect->y + waverect->h - 4);
			nvgStroke(vg);
			o++;
		}
	}
}

static void Sampler_Draw_Finder(void* vg, Rect* waverect, Rect* finder, AudioSample* sample, Sampler* this) {
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_BASE_L1, 255, 0.75));
	nvgRoundedRect(
		vg,
		waverect->x - 1,
		finder->y - 1,
		waverect->w + 2,
		finder->h + 2,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->visual.findCol);
	nvgRoundedRect(
		vg,
		finder->x,
		finder->y,
		finder->w,
		finder->h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 255, 1.0));
	nvgRoundedRect(
		vg,
		waverect->x + waverect->w * ((f32)sample->playFrame / sample->samplesNum) - 1,
		finder->y,
		2,
		finder->h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	if (sample->selectStart == sample->selectEnd)
		return;
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 25, 1.25));
	nvgRoundedRect(
		vg,
		waverect->x + waverect->w * ((f32)sample->selectStart / sample->samplesNum) - 1,
		finder->y,
		waverect->w * (f32)(sample->selectEnd - sample->selectStart) / sample->samplesNum,
		finder->h,
		SPLIT_ROUND_R
	);
	
	nvgFill(vg);
}

static void Sampler_ZoomLogic(WindowContext* winCtx, Split* split, Sampler* this, Rect* waverect, Rect* finder) {
	f32 mouseX = (f32)(split->mousePos.x - waverect->x) / waverect->w;
	
	mouseX = Clamp(mouseX, 0.0, 1.0);
	
	this->state.waveWinBlock = false;
	
	// Finder Zoom
	if (GeoGrid_Cursor_InRect(split, finder) || this->zoom.modifying) {
		this->state.waveWinBlock = true;
		
		Theme_SmoothStepToCol(&this->visual.findCol, Theme_GetColor(THEME_HIGHLIGHT, 175, 1.0), 0.25, 1.0, 0.0);
		
		if (winCtx->input.mouse.clickL.hold) {
			f32 xmod = (mouseX - this->mousePrevX);
			f32 endSample;
			f32 smplPixelRatio;
			if (this->zoom.modifying == 0) {
				s32 chk = Abs(split->mousePos.x - finder->x);
				s32 a = chk;
				
				chk = Min(chk, Abs(split->mousePos.x - (finder->x + finder->w)));
				
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
					endSample = this->sample.samplesNum * this->zoom.end;
					smplPixelRatio = (endSample - this->sample.samplesNum * mouseX) / waverect->w;
					
					if (smplPixelRatio < 0.01) {
						mouseX = (this->sample.samplesNum * this->zoom.end - 0.01 * waverect->w) / this->sample.samplesNum;
					}
					
					this->zoom.start = mouseX;
					break;
				case 1:
					endSample = this->sample.samplesNum * mouseX;
					smplPixelRatio = (endSample - this->sample.samplesNum * this->zoom.start) / waverect->w;
					
					if (smplPixelRatio < 0.01) {
						mouseX = (this->sample.samplesNum * this->zoom.start + 0.01 * waverect->w) / this->sample.samplesNum;
					}
					
					this->zoom.end = mouseX;
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
	
	if (GeoGrid_Cursor_InRect(split, waverect) && this->state.waveWinBlock == false) {
		f32 zoomRatio = this->zoom.end - this->zoom.start;
		f32 scrollY = winCtx->input.mouse.scrollY;
		
		if (winCtx->input.mouse.clickMid.hold) {
			f32 mod = (mouseX - this->mousePrevX) * zoomRatio;
			
			if (this->zoom.start - mod >= 0 && this->zoom.end - mod <= 1.0) {
				this->zoom.start -= mod;
				this->zoom.end -= mod;
			}
		}
		
		if (scrollY) {
			if (scrollY > 0) {
				f32 relPosX = this->zoom.start + ((f32)(split->mousePos.x - waverect->x) / waverect->w) * zoomRatio;
				f32 start = this->zoom.start - (this->zoom.start - relPosX) * 0.25;
				f32 end = this->zoom.end - (this->zoom.end - relPosX) * 0.25;
				f32 endSample = this->sample.samplesNum * end;
				f32 smplPixelRatio = (endSample - this->sample.samplesNum * start) / waverect->w;
				
				if (smplPixelRatio > 0.01) {
					this->zoom.start = start;
					this->zoom.end = end;
				} else {
					f32 mid = (this->zoom.end + this->zoom.start) * 0.5;
					this->zoom.start = (this->sample.samplesNum * mid - 0.005 * waverect->w) / this->sample.samplesNum;
					this->zoom.end = (this->sample.samplesNum * mid + 0.005 * waverect->w) / this->sample.samplesNum;
				}
			} else {
				this->zoom.start += (0.0 - this->zoom.start) * zoomRatio;
				this->zoom.end += (1.0 - this->zoom.end) * zoomRatio;
			}
		}
		
		this->zoom.start = Clamp(this->zoom.start, 0.0, 1.0);
		this->zoom.end = Clamp(this->zoom.end, 0.0, 1.0);
	}
	
	Math_SmoothStepToF(&this->zoom.vStart, this->zoom.start, 0.25, 1.0, 0.0);
	Math_SmoothStepToF(&this->zoom.vEnd, this->zoom.end, 0.25, 1.0, 0.0);
	this->mousePrevX = mouseX;
}

void Sampler_Draw(WindowContext* winCtx, Sampler* this, Split* split) {
	void* vg = winCtx->vg;
	AudioSample* sample = &this->sample;
	Rect waverect = {
		SPLIT_ELEM_X_PADDING * 2,
		this->waveFormPos,
		split->rect.w - SPLIT_ELEM_X_PADDING * 4,
		split->rect.h - (this->waveFormPos + SPLIT_TEXT_H * 0.5 + SPLIT_BAR_HEIGHT) - 20
	};
	Rect finder = {
		waverect.x + ((waverect.w + 2) * this->zoom.start),
		waverect.y + waverect.h + 4,
		waverect.w * this->zoom.end - ((waverect.w - 4) * this->zoom.start),
		16,
	};
	
	Sampler_ZoomLogic(winCtx, split, this, &waverect, &finder);
	Sampler_Draw_WaveRect(vg, &waverect);
	Sampler_Draw_Finder(vg, &waverect, &finder, sample, this);
	
	if (this->sample.audio.p == NULL)
		return;
	
	f32 zoomRatio = this->zoom.vEnd - this->zoom.vStart;
	
	this->visual.playPos = Sampler_GetSamplePos((f32)sample->playFrame, &waverect, sample, this);
	this->visual.selectStart = Sampler_GetSamplePos((f32)sample->selectStart, &waverect, sample, this);
	this->visual.selectEnd = Sampler_GetSamplePos((f32)sample->selectEnd, &waverect, sample, this);
	
	if (sample->instrument.loop.count) {
		this->visual.loopA = Sampler_GetSamplePos((f32)sample->instrument.loop.start, &waverect, sample, this);
		this->visual.loopB = Sampler_GetSamplePos((f32)sample->instrument.loop.end, &waverect, sample, this);
	}
	
	Sampler_Draw_Grid(vg, &waverect, sample, this);
	Sampler_Draw_Selection(vg, &waverect, &finder, sample, this);
	if (sample->samplesNum * zoomRatio > waverect.w * 4)
		Sampler_Draw_Waveform_Block(vg, &waverect, sample, this);
	else {
		// Sampler_Draw_Waveform_Line(vg, &waverect, sample, this);
		Sampler_Draw_Waveform_Spline(vg, &waverect, sample, this);
	}
	Sampler_Draw_Position(vg, &waverect, &finder, sample, this);
	Sampler_Draw_LoopMarker(vg, &waverect, sample, this);
	
	// Adjust play position line by clicking
	
	if ((Split_Cursor(split, 1) && GeoGrid_Cursor_InRect(split, &waverect) && this->state.waveWinBlock == false) || this->state.selecting || this->state.selModify || this->state.setLoop) {
		MouseInput* mouse = &winCtx->input.mouse;
		static s32 noPlayPosSet;
		f32 curPosX = split->mousePos.x - waverect.x;
		f32 prsPosX = split->mousePressPos.x - waverect.x;
		
		curPosX = ClampMin(curPosX, 0);
		
		curPosX = sample->samplesNum * this->zoom.start + (f32)sample->samplesNum * (this->zoom.end - this->zoom.start) * (curPosX / waverect.w);
		prsPosX = sample->samplesNum * this->zoom.start + (f32)sample->samplesNum * (this->zoom.end - this->zoom.start) * (prsPosX / waverect.w);
		
		if (this->sample.instrument.loop.count) {
			if (this->state.setLoop == false) {
				if (Abs(split->mousePos.x - this->visual.loopA) < 8) {
					Cursor_ForceCursor(CURSOR_ARROW_H);
					if (mouse->clickL.press)
						this->state.setLoop = 1;
				}
				if (Abs(split->mousePos.x - this->visual.loopB) < 8) {
					Cursor_ForceCursor(CURSOR_ARROW_H);
					if (mouse->clickL.press)
						this->state.setLoop = -1;
				}
			}
			
			if (this->state.setLoop) {
				if (mouse->clickL.hold == false) {
					this->state.setLoop = 0;
					
					return;
				}
				
				if (this->state.setLoop == 1) {
					this->sample.instrument.loop.start = curPosX;
				} else {
					this->sample.instrument.loop.end = curPosX;
				}
				
				return;
			}
		}
		
		if (mouse->clickL.release && noPlayPosSet == false)
			sample->playFrame = curPosX;
		noPlayPosSet = false;
		
		if (mouse->doubleClick)
			sample->selectEnd = sample->selectStart = 0;
		
		if (mouse->clickL.hold == 0) {
			this->state.selecting = 0;
			this->state.selModify = 0;
		}
		
		if (mouse->clickL.hold && (Input_GetPressPosDist() > 4 || this->state.selecting || this->state.selecting)) {
			if (this->state.selModify == false && this->state.selecting == false) {
				if (sample->selectStart != sample->selectEnd) {
					if (Abs(sample->selectStart - prsPosX) > Abs(sample->selectEnd - prsPosX))
						this->state.lockPos = sample->selectStart;
					else
						this->state.lockPos = sample->selectEnd;
					this->state.selModify = 1;
				} else {
					sample->selectStart = sample->playFrame;
					this->state.selecting = 1;
				}
			}
			
			if (this->state.selecting) {
				sample->selectStart = Min(prsPosX, curPosX);
				sample->selectEnd = Max(prsPosX, curPosX);
			}
			
			if (this->state.selModify) {
				sample->selectStart = Min(this->state.lockPos, curPosX);
				sample->selectEnd = Max(this->state.lockPos, curPosX);
			}
			
			noPlayPosSet = true;
		}
		
		sample->selectStart = ClampMin(sample->selectStart, 0);
		sample->selectEnd = ClampMax(sample->selectEnd, sample->samplesNum);
	}
}