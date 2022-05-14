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
	ElButton  playButton;
	ElTextbox sampleName;
	ElText    text;
	AudioSampleInfo sample;
	void* player;
	f32   y;
	s32   butTog;
} Sampler;

void Window_DropCallback();
void Window_Update(WindowContext* winCtx);
void Window_Draw(WindowContext* winCtx);
void Sampler_Init(WindowContext* winCtx, Sampler* this, Split* split);
void Sampler_Destroy(WindowContext* winCtx, Sampler* this, Split* split);
void Sampler_Update(WindowContext* winCtx, Sampler* this, Split* split);
void Sampler_Draw(WindowContext* winCtx, Sampler* this, Split* split);
extern SplitTask gTaskTable[2];