#include <ExtGui/Global.h>
#include "AudioConvert.h"

typedef struct {
	AppInfo app;
	InputContext   input;
	GeoGridContext geoGrid;
	CursorContext  cursor;
	void* vg;
} WindowContext;

typedef struct {
	ElButton    playButton;
	ElTextbox   sampleName;
	ElSlider    loopA;
	ElSlider    loopB;
	ElText      text[2];
	AudioSample sample;
	void* player;
	f32   y;
	s32   butTog;
	struct {
		f32 start;
		f32 end;
		s32 modifying;
		f32 rel;
		
		f32 vStart;
		f32 vEnd;
	} zoom;
	struct {
		f32 playPos;
		f32 loopA;
		f32 loopB;
		NVGcolor findCol;
	} visual;
	f32 mousePrevX;
	f32 prevLoopB;
} Sampler;

void Window_DropCallback();
void Window_Update(WindowContext* winCtx);
void Window_Draw(WindowContext* winCtx);
void Sampler_Init(WindowContext* winCtx, Sampler* this, Split* split);
void Sampler_Destroy(WindowContext* winCtx, Sampler* this, Split* split);
void Sampler_Update(WindowContext* winCtx, Sampler* this, Split* split);
void Sampler_Draw(WindowContext* winCtx, Sampler* this, Split* split);
extern SplitTask gTaskTable[2];