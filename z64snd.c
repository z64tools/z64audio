
#include "include/z64snd.h"

#ifndef __Z64AUDIO_TERMINAL__
#include "include/gui.h"
#endif

int z64audio_Window(int argc, char** argv) {
	#ifndef __Z64AUDIO_TERMINAL__
		
		static s32 firstDraw = 0;
		
		wowGui_bind_init("z64audio", this.window.x, this.window.y * 5);
		wow_windowicon(1);
		wowGui_bind_set_fps(60);
		
		sWinData = (SWindowData*)__wowGui_mfbWindow;
		sWinDataWin = (SWindowData_Win*)sWinData->specific;
		
		wowGui_update_window(0, 0, this.window.x, this.window.y);
		Setup_GuiFunc(Gui_InputFile);
		
		while (1) {
			sWindowBackground.rect.w = this.scale.x;
			sWindowBackground.rect.h = this.scale.y * 2;
			wowGui_padding(24, 24);
			wowGui_columns(3);
			wowGui_row_height(24);
			
			wowGui_frame();
			wowGui_bind_events();
			wowGui_viewport(0, 0, this.scale.x, this.scale.y);
			
			if (wowGui_bind_should_redraw() || firstDraw != 10 || this.scale.y != this.targetScale.y) {
				if (this.scale.y != this.targetScale.y) {
					Math_SmoothStepToS(&this.scale.y, this.targetScale.y, 5, 8, 1);
					wowGui_update_window(0, 0, this.scale.x, this.scale.y);
				}
				
				if (firstDraw != 2)
					firstDraw++;
				
				if (wowGui_window(&sWindowBackground)) {
					wowGui_window_end();
				}
				
				this.func();
				
				Draw_Corner();
			}
			
			wowGui_bind_result();
			if (wowGui_bind_endmainloop())
				break;
		}
	#endif
	
	return 0;
}

int z64audio_Terminal(int argc, char** argv) {
	#ifdef __Z64AUDIO_TERMINAL__
		s8 fileCount = 0;
		int i;
		char* file[3] = { 0 };
		char* infile = 0;
		
		atexit(z64audioAtExit);
		
		STATE_DEBUG_PRINT = true;
		STATE_FABULOUS = false;
		
		#ifdef _WIN32
			WIN32_CMD_FIX;
		#endif
		
		fprintf(
			stderr,
			"/******************************\n"
			" * \e[0;36mz64audio 1.0.0\e[m             *\n"
			" *   by rankaisija and z64me  *\n"
			" ******************************/\n\n"
		);
		
		/*{
		   char cwd[1024];
		   DebugPrint("cwd: '%s'\n", wow_getcwd(cwd, sizeof(cwd)));
		   }*/
		
		if (argc == 1) {
			showArgs();
		}
		/* one input file = guided mode */
		else if (argc == 2 && memcmp(argv[1], "--", 2)) {
			// TODO revert for final release
			// gAudioState.mode = requestMode();
			gAudioState.mode = Z64AUDIOMODE_WAV_TO_ZZRTL; // For testing purposes
			infile = argv[1];
			#ifdef _WIN32
				gWaitAtExit = true;
			#endif
		}
		/* multiple arguments */
		else{
			int i;
			for (i = 1; i < argc; i += 2) {
				char* arg = argv[i];
				char* next = argv[i + 1];
				
				if (!strcmp(arg, "--tabledesign")) {
					shiftArgs(&argc, argv, i);
					
					return tabledesign(argc, argv, stdout);
				}else if (!strcmp(arg, "--vadpcm_enc")) {
					shiftArgs(&argc, argv, i);
					
					return vadpcm_enc(argc, argv);
				}
				
				if (!next)
					PrintFail("Argument '%s' is missing parameter\n", arg);
				
				if (!strcmp(arg, "--wav"))
					infile = next;
				else if (!strcmp(arg, "--mode"))
					gAudioState.mode = atoi(next);
				else
					PrintFail("Unknown argument '%s'\n", arg);
			}
		}
		
		/* ensure provided mode is acceptable */
		assertMode(gAudioState.mode);
		
		if ((fileCount = File_GetAndSort(infile, file)) <= 0)
			PrintFail("Something has gone terribly wrong... fileCount == %d", fileCount);
		printf("\n");
		
		/* These variables are used for output */
		char fname[FILENAME_BUFFER] = { 0 };
		char path[FILENAME_BUFFER] = { 0 };
		
		for (s32 i = 0; i < fileCount; i++) {
			GetFilename(file[i], fname, path);
			
			printf("\n");
			ColorPrint(1, "[%s]", file[i]);
			
			switch (gAudioState.ftype) {
			    case NONE:
				    PrintFail("File you've given isn't something I can work with sadly");
				    break;
				    
			    case WAV:
				    DebugPrint("switch WAV");
				    Audio_WavConvert(file[i], fname, path, i);
				    if (gAudioState.mode == Z64AUDIOMODE_WAV_TO_AIFF)
					    break;
			    case AIFF:
				    DebugPrint("switch AIFF");
				    Audio_AiffConvert(file[0], fname, path, i);
				    if (gAudioState.mode == Z64AUDIOMODE_WAV_TO_AIFC)
					    break;
			    case AIFC:
				    DebugPrint("switch AIFC");
				    Audio_AifcParseZzrtl(file[0], fname, path, i);
			}
			
			Audio_Clean(fname, path);
		}
		
		Audio_GenerateInstrumentConf(file[0], path, fileCount);
		
		for (i = 0; i < (sizeof(file) / sizeof(*file)); ++i)
			if (file[i])
				free(file[i]);
		
		DebugPrint("File free\t\t\tOK\n");
	#endif
	
	return 0;
}

int wow_main(argc, argv) {
	wow_main_args(argc, argv);
	s32 wow;
	
	#ifndef __Z64AUDIO_TERMINAL__
		wow = z64audio_Window(argc, argv);
		
	#else
		win32icon();
		wow = z64audio_Terminal(argc, argv);
	#endif
	
	return wow;
}
