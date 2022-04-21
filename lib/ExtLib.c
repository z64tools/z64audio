#define __EXTLIB_C__
#include "ExtLib.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#ifdef __IDE_FLAG__
    #ifdef _WIN32
	#undef _WIN32
    #endif
#endif

// ExtendedLibrary

// Print
PrintfSuppressLevel gPrintfSuppress = 0;
char* sPrintfPrefix = "ExtLib";
u8 sPrintfType = 1;
u8 gPrintfProgressing;
u8* sSegment[255];
char sCurrentPath[2048];
char* sPrintfPreType[][4] = {
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
	{
		">",
		">",
		">",
		">"
	}
};
u8 sBuffer_Temp[MbToBin(64)];
u32 sSeek_Temp = 0;
time_t sTime;
MemFile sLog;

// Segment
void SetSegment(const u8 id, void* segment) {
	sSegment[id] = segment;
}

void* SegmentedToVirtual(const u8 id, void32 ptr) {
	if (sSegment[id] == NULL)
		printf_error("Segment 0x%X == NULL", id);
	
	return &sSegment[id][ptr];
}

void32 VirtualToSegmented(const u8 id, void* ptr) {
	return (uPtr)ptr - (uPtr)sSegment[id];
}

// Static Memory
void* Tmp_Alloc(u32 size) {
	u8* ret;
	
	if (size >= sizeof(sBuffer_Temp) / 2)
		printf_error("Can't fit %fMb into the GraphBuffer", BinToMb(size));
	
	if (size == 0)
		return NULL;
	
	if (sSeek_Temp + size + 0x10 > sizeof(sBuffer_Temp)) {
		printf_warning_align("Tmp_Alloc:", "rewind\a");
		sSeek_Temp = 0;
	}
	
	ret = &sBuffer_Temp[sSeek_Temp];
	memset(ret, 0, size + 1);
	sSeek_Temp = sSeek_Temp + size + 1;
	
	return ret;
}

char* Tmp_String(char* str) {
	char* ret = Tmp_Alloc(strlen(str));
	
	strcpy(ret, str);
	
	return ret;
}

char* Tmp_Printf(char* fmt, ...) {
	char tempBuf[512 * 2];
	
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(tempBuf, ArrayCount(tempBuf), fmt, args);
	va_end(args);
	
	return Tmp_String(tempBuf);
}

struct timeval sTimeStart, sTimeStop;
void gettimeofday(struct timeval*, void*);

void Time_Start(void) {
	gettimeofday(&sTimeStart, NULL);
}

f32 Time_Get(void) {
	gettimeofday(&sTimeStop, NULL);
	
	return (sTimeStop.tv_sec - sTimeStart.tv_sec) + (f32)(sTimeStop.tv_usec - sTimeStart.tv_usec) / 1000000;
}

// Dir
struct {
	s32 enterCount[512];
	s32 pos;
	DirParam param;
} sDir;

void Dir_SetParam(DirParam w) {
	sDir.param |= w;
}

void Dir_UnsetParam(DirParam w) {
	sDir.param &= ~(w);
}

void Dir_Set(char* path, ...) {
	s32 firstSet = 0;
	va_list args;
	
	if (sCurrentPath[0] == '\0')
		firstSet++;
	
	memset(sCurrentPath, 0, 512);
	va_start(args, path);
	vsnprintf(sCurrentPath, ArrayCount(sCurrentPath), path, args);
	va_end(args);
	
	if (firstSet) {
		for (s32 i = 0; i < 512; i++) {
			sDir.enterCount[i] = 0;
		}
		for (s32 i = 0; i < strlen(sCurrentPath); i++) {
			if (sCurrentPath[i] == '/' || sCurrentPath[i] == '\\')
				sDir.enterCount[++sDir.pos] = 1;
		}
	}
}

void Dir_Enter(char* fmt, ...) {
	va_list args;
	char buffer[512];
	
	va_start(args, fmt);
	vsnprintf(buffer, ArrayCount(buffer), fmt, args);
	va_end(args);
	
	if (!(sDir.param & DIR__MAKE_ON_ENTER)) {
		if (!Dir_Stat(buffer)) {
			printf_error("Could not enter folder [%s%s]", sCurrentPath, buffer);
		}
	}
	
	sDir.pos++;
	sDir.enterCount[sDir.pos] = 0;
	for (s32 i = 0;; i++) {
		if (buffer[i] == '\0')
			break;
		if (buffer[i] == '/' || buffer[i] == '\\')
			sDir.enterCount[sDir.pos]++;
	}
	
	strcat(sCurrentPath, buffer);
	
	if (sDir.param & DIR__MAKE_ON_ENTER) {
		Dir_MakeCurrent();
	}
}

void Dir_Leave(void) {
	s32 count = sDir.enterCount[sDir.pos];
	
	for (s32 i = 0; i < count; i++) {
		#ifndef NDEBUG
			char compBuffer[512];
			strcpy(compBuffer, sCurrentPath);
		#endif
		
		sCurrentPath[strlen(sCurrentPath) - 1] = '\0';
		strcpy(sCurrentPath, String_GetPath(sCurrentPath));
	}
	
	sDir.enterCount[sDir.pos] = 0;
	sDir.pos--;
}

void Dir_Make(char* dir, ...) {
	char argBuf[512];
	va_list args;
	
	va_start(args, dir);
	vsnprintf(argBuf, ArrayCount(argBuf), dir, args);
	va_end(args);
	
	MakeDir(Tmp_Printf("%s%s", sCurrentPath, argBuf));
}

void Dir_MakeCurrent(void) {
	MakeDir(sCurrentPath);
}

char* Dir_Current(void) {
	return sCurrentPath;
}

char* Dir_File(char* fmt, ...) {
	char* buffer;
	char argBuf[512];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(argBuf, ArrayCount(argBuf), fmt, args);
	va_end(args);
	
	if (StrStr(argBuf, "*")) {
		return Dir_GetWildcard(argBuf);
	}
	
	buffer = Tmp_Printf("%s%s", sCurrentPath, argBuf);
	
	return buffer;
}

Time Dir_Stat(char* item) {
	struct stat st = { 0 };
	
	if (stat(Tmp_Printf("%s%s", sCurrentPath, item), &st) == -1)
		return 0;
	
	if (st.st_mtime == 0)
		printf_error("Stat: [%s] time is zero?!", item);
	
	return st.st_mtime;
}

char* Dir_GetWildcard(char* x) {
	ItemList list;
	char* sEnd;
	char* sStart = NULL;
	char* restorePath;
	char* search = StrStr(x, "*");
	char* posPath = String_GetPath(Tmp_Printf("%s%s", sCurrentPath, x));
	
	sEnd = Tmp_String(&search[1]);
	
	if ((uPtr)search - (uPtr)x > 0) {
		sStart = Tmp_Alloc((uPtr)search - (uPtr)x + 2);
		memcpy(sStart, x, (uPtr)search - (uPtr)x);
	}
	
	restorePath = Tmp_String(sCurrentPath);
	
	if (strcmp(posPath, restorePath)) {
		Dir_Set(posPath);
	}
	
	Dir_ItemList(&list, false);
	
	if (strcmp(posPath, restorePath)) {
		Dir_Set(restorePath);
	}
	
	for (s32 i = 0; i < list.num; i++) {
		if (StrStr(list.item[i], sEnd) && (sStart == NULL || StrStr(list.item[i], sStart))) {
			return Tmp_Printf("%s%s", posPath, list.item[i]);
		}
	}
	
	return NULL;
}

static bool __isDir(char* path) {
	struct stat st = { 0 };
	
	stat(path, &st);
	
	if (S_ISDIR(st.st_mode)) {
		return true;
	}
	
	return false;
}

void Dir_ItemList(ItemList* itemList, bool isPath) {
	DIR* dir = opendir(sCurrentPath);
	u32 bufSize = 0;
	struct dirent* entry;
	
	*itemList = (ItemList) { 0 };
	
	if (dir == NULL)
		printf_error_align("Could not opendir()", "%s", sCurrentPath);
	
	while ((entry = readdir(dir)) != NULL) {
		if (isPath) {
			if (__isDir(Dir_File(entry->d_name))) {
				if (entry->d_name[0] == '.')
					continue;
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		} else {
			if (!__isDir(Dir_File(entry->d_name))) {
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		}
	}
	
	closedir(dir);
	
	if (itemList->num) {
		u32 i = 0;
		dir = opendir(sCurrentPath);
		itemList->buffer = Tmp_Alloc(bufSize);
		itemList->item = Tmp_Alloc(sizeof(char*) * itemList->num);
		
		while ((entry = readdir(dir)) != NULL) {
			if (isPath) {
				if (__isDir(Dir_File(entry->d_name))) {
					if (entry->d_name[0] == '.')
						continue;
					strcpy(&itemList->buffer[itemList->writePoint], Tmp_Printf("%s/", entry->d_name));
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			} else {
				if (!__isDir(Dir_File(entry->d_name))) {
					strcpy(&itemList->buffer[itemList->writePoint], Tmp_Printf("%s", entry->d_name));
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			}
		}
		closedir(dir);
	}
}

static void Dir_ItemList_Recursive_ChildCount(ItemList* target, char* pathTo, char* keyword) {
	ItemList folder = { 0 };
	ItemList file = { 0 };
	
	Dir_ItemList(&folder, true);
	Dir_ItemList(&file, false);
	
	for (s32 i = 0; i < folder.num; i++) {
		Dir_Enter(folder.item[i]); {
			Dir_ItemList_Recursive_ChildCount(target, Tmp_Printf("%s%s/", pathTo, folder.item[i]), keyword);
			Dir_Leave();
		}
	}
	
	for (s32 i = 0; i < file.num; i++) {
		if (keyword && !StrStrCase(file.item[i], keyword))
			continue;
		target->num++;
	}
}

static void Dir_ItemList_Recursive_ChildWrite(ItemList* target, char* pathTo, char* keyword) {
	ItemList folder = { 0 };
	ItemList file = { 0 };
	
	Dir_ItemList(&folder, true);
	Dir_ItemList(&file, false);
	
	for (s32 i = 0; i < folder.num; i++) {
		Dir_Enter(folder.item[i]); {
			Dir_ItemList_Recursive_ChildWrite(target, Tmp_Printf("%s%s", pathTo, folder.item[i]), keyword);
			Dir_Leave();
		}
	}
	
	for (s32 i = 0; i < file.num; i++) {
		if (keyword && !StrStrCase(file.item[i], keyword))
			continue;
		target->item[target->num++] = Tmp_Printf("%s%s", pathTo, file.item[i]);
	}
}

void Dir_ItemList_Recursive(ItemList* target, char* keyword) {
	memset(target, 0, sizeof(*target));
	
	Dir_ItemList_Recursive_ChildCount(target, "", keyword);
	target->item = Tmp_Alloc(sizeof(char*) * target->num);
	target->num = 0;
	Dir_ItemList_Recursive_ChildWrite(target, "", keyword);
}

void Dir_ItemList_Not(ItemList* itemList, bool isPath, char* not) {
	DIR* dir = opendir(sCurrentPath);
	u32 bufSize = 0;
	struct dirent* entry;
	
	*itemList = (ItemList) { 0 };
	
	if (dir == NULL)
		printf_error_align("Could not opendir()", "%s", sCurrentPath);
	
	while ((entry = readdir(dir)) != NULL) {
		if (isPath) {
			if (__isDir(Dir_File(entry->d_name))) {
				if (entry->d_name[0] == '.')
					continue;
				if (strcmp(entry->d_name, not) == 0)
					continue;
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		} else {
			if (!__isDir(Dir_File(entry->d_name))) {
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		}
	}
	
	closedir(dir);
	
	if (itemList->num) {
		u32 i = 0;
		dir = opendir(sCurrentPath);
		itemList->buffer = Tmp_Alloc(bufSize);
		itemList->item = Tmp_Alloc(sizeof(char*) * itemList->num);
		
		while ((entry = readdir(dir)) != NULL) {
			if (isPath) {
				if (__isDir(Dir_File(entry->d_name))) {
					if (entry->d_name[0] == '.')
						continue;
					if (strcmp(entry->d_name, not) == 0)
						continue;
					strcpy(&itemList->buffer[itemList->writePoint], Tmp_Printf("%s/", entry->d_name));
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			} else {
				if (!__isDir(Dir_File(entry->d_name))) {
					strcpy(&itemList->buffer[itemList->writePoint], Tmp_Printf("%s", entry->d_name));
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			}
		}
		closedir(dir);
	}
}

void Dir_ItemList_Keyword(ItemList* itemList, char* ext) {
	DIR* dir = opendir(sCurrentPath);
	u32 bufSize = 0;
	struct dirent* entry;
	
	*itemList = (ItemList) { 0 };
	
	if (dir == NULL)
		printf_error_align("Could not opendir()", "%s", sCurrentPath);
	
	while ((entry = readdir(dir)) != NULL) {
		if (!__isDir(Dir_File(entry->d_name))) {
			if (!StrStr(entry->d_name, ext))
				continue;
			itemList->num++;
			bufSize += strlen(entry->d_name) + 2;
		}
	}
	
	closedir(dir);
	
	if (itemList->num) {
		u32 i = 0;
		dir = opendir(sCurrentPath);
		itemList->buffer = Tmp_Alloc(bufSize);
		itemList->item = Tmp_Alloc(sizeof(char*) * itemList->num);
		
		while ((entry = readdir(dir)) != NULL) {
			if (!__isDir(Dir_File(entry->d_name))) {
				if (!StrStr(entry->d_name, ext))
					continue;
				strcpy(&itemList->buffer[itemList->writePoint], Tmp_Printf("%s", entry->d_name));
				itemList->item[i] = &itemList->buffer[itemList->writePoint];
				itemList->writePoint += strlen(itemList->item[i]) + 1;
				i++;
			}
		}
		closedir(dir);
	}
}

Time Stat(char* item) {
	struct stat st = { 0 };
	
	if (stat(item, &st) == -1)
		return 0;
	
	if (st.st_mtime == 0)
		printf_error("Stat: [%s] time is zero?!", item);
	
	return st.st_mtime;
}

static void __MakeDir(const char* buffer) {
	struct stat st = { 0 };
	
	if (stat(buffer, &st) == -1) {
		#ifdef _WIN32
			if (mkdir(buffer)) {
				printf_error_align("mkdir", "%s", buffer);
			}
		#else
			if (mkdir(buffer, 0700)) {
				printf_error_align("mkdir", "%s", buffer);
			}
		#endif
	}
}

void MakeDir(const char* dir, ...) {
	char buffer[512];
	s32 pathNum;
	va_list args;
	
	va_start(args, dir);
	vsnprintf(buffer, ArrayCount(buffer), dir, args);
	va_end(args);
	
	pathNum = String_GetPathNum(buffer);
	
	if (pathNum == 1) {
		__MakeDir(buffer);
	} else {
		for (s32 i = 0; i < pathNum; i++) {
			char tempBuff[512];
			char* folder = String_GetFolder(buffer, 0);
			
			strcpy(tempBuff, folder);
			for (s32 j = 1; j < i + 1; j++) {
				folder = String_GetFolder(buffer, j);
				strcat(tempBuff, folder);
			}
			
			__MakeDir(tempBuff);
		}
	}
}

char* CurWorkDir(void) {
	static char buf[512];
	
	if (getcwd(buf, sizeof(buf)) == NULL) {
		printf_error("Could not get CurWorkDir");
	}
	
	for (s32 i = 0; i < strlen(buf); i++) {
		if (buf[i] == '\\')
			buf[i] = '/';
	}
	
	strcat(buf, "/");
	
	return buf;
}

// ItemList
void ItemList_NumericalSort(ItemList* list) {
	ItemList sorted = { 0 };
	u32 highestNum = 0;
	
	for (s32 i = 0; i < list->num; i++) {
		if (String_GetInt(list->item[i]) > highestNum)
			highestNum = String_GetInt(list->item[i]);
	}
	
	sorted.buffer = NULL;
	sorted.item = Tmp_Alloc(sizeof(char*) * (highestNum + 1));
	
	for (s32 i = 0; i <= highestNum; i++) {
		u32 null = true;
		
		for (s32 j = 0; j < list->num; j++) {
			if (String_GetInt(list->item[j]) == i) {
				sorted.item[sorted.num++] = Tmp_String(list->item[j]);
				null = false;
				break;
			}
		}
		
		if (null == true) {
			sorted.item[sorted.num++] = NULL;
		}
	}
	
	#ifndef NDEBUG
		printf_info("sorted, %d -> %d", list->num, sorted.num);
		for (s32 i = 0; i < sorted.num; i++) {
			printf_info("%d - %s", i, sorted.item[i]);
		}
	#endif
	
	*list = sorted;
}

void printf_SetSuppressLevel(PrintfSuppressLevel lvl) {
	gPrintfSuppress = lvl;
}

void printf_SetPrefix(char* fmt) {
	sPrintfPrefix = fmt;
}

void printf_SetPrintfTypes(const char* d, const char* w, const char* e, const char* i) {
	sPrintfType = 0;
	sPrintfPreType[sPrintfType][0] = String_Generate(d);
	sPrintfPreType[sPrintfType][1] = String_Generate(w);
	sPrintfPreType[sPrintfType][2] = String_Generate(e);
	sPrintfPreType[sPrintfType][3] = String_Generate(i);
}

void printf_toolinfo(const char* toolname, const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	u32 strln = strlen(toolname);
	u32 rmv = 0;
	u32 tmp = strln;
	va_list args;
	
	for (s32 i = 0; i < strln; i++) {
		if (rmv) {
			if (toolname[i] != 'm') {
				tmp--;
			} else {
				tmp -= 2;
				rmv = false;
			}
		} else {
			if (toolname[i] == '\e' && toolname[i + 1] == '[') {
				rmv = true;
				strln--;
			}
		}
	}
	
	strln = tmp;
	
	va_start(args, fmt);
	
	printf(PRNT_GRAY "[>]--");
	for (s32 i = 0; i < strln; i++)
		printf("-");
	printf("------[>]\n");
	
	printf(" |   ");
	printf(PRNT_CYAN "%s" PRNT_GRAY, toolname);
	printf("       |\n");
	
	printf("[>]--");
	for (s32 i = 0; i < strln; i++)
		printf("-");
	printf("------[>]\n" PRNT_RSET);
	
	vprintf(
		fmt,
		args
	);
	va_end(args);
}

static void __printf_call(u32 type) {
	char* color[4] = {
		PRNT_PRPL,
		PRNT_YELW,
		PRNT_REDD,
		PRNT_CYAN
	};
	
	printf(
		"%s"
		PRNT_GRAY "["
		"%s%s"
		PRNT_GRAY "]: "
		PRNT_RSET,
		sPrintfPrefix,
		color[type],
		sPrintfPreType[sPrintfType][type]
	);
}

void printf_debug(const char* fmt, ...) {
	if (gPrintfSuppress > PSL_DEBUG)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(0);
	vprintf(
		fmt,
		args
	);
	printf(PRNT_RSET "\n");
	va_end(args);
}

void printf_debug_align(const char* info, const char* fmt, ...) {
	if (gPrintfSuppress > PSL_DEBUG)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(0);
	printf(
		"%-16s " PRNT_RSET,
		info
	);
	vprintf(
		fmt,
		args
	);
	printf(PRNT_RSET "\n");
	va_end(args);
}

void printf_warning(const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_WARNING)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(1);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_warning_align(const char* info, const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_WARNING)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(1);
	printf(
		"%-16s " PRNT_RSET,
		info
	);
	vprintf(
		fmt,
		args
	);
	printf(PRNT_RSET "\n");
	va_end(args);
}

void printf_error(const char* fmt, ...) {
	if (gPrintfSuppress < PSL_NO_ERROR) {
		if (gPrintfProgressing) {
			printf("\n");
			gPrintfProgressing = false;
		}
		
		va_list args;
		
		printf("\n");
		va_start(args, fmt);
		__printf_call(2);
		vprintf(
			fmt,
			args
		);
		printf("\n");
		va_end(args);
	}
	
	#ifdef _WIN32
		printf(PRNT_RSET "Press any key to exit...");
		getchar();
	#endif
	exit(EXIT_FAILURE);
}

void printf_error_align(const char* info, const char* fmt, ...) {
	if (gPrintfSuppress < PSL_NO_ERROR) {
		if (gPrintfProgressing) {
			printf("\n");
			gPrintfProgressing = false;
		}
		
		va_list args;
		
		va_start(args, fmt);
		__printf_call(2);
		printf(
			"%-16s " PRNT_RSET,
			info
		);
		vprintf(
			fmt,
			args
		);
		printf(PRNT_RSET "\n");
		va_end(args);
	}
	
	#ifdef _WIN32
		printf(PRNT_RSET "Press any key to exit...");
		getchar();
	#endif
	exit(EXIT_FAILURE);
}

void printf_info(const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	va_list args;
	
	va_start(args, fmt);
	__printf_call(3);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_info_align(const char* info, const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	va_list args;
	
	va_start(args, fmt);
	__printf_call(3);
	printf(
		"%-16s " PRNT_RSET,
		info
	);
	vprintf(
		fmt,
		args
	);
	printf(PRNT_RSET "\n");
	va_end(args);
}

void printf_progress(const char* info, u32 a, u32 b) {
	if (gPrintfSuppress >= PSL_NO_INFO) {
		return;
	}
	
	static f32 lstPrcnt;
	f32 prcnt = (f32)a / (f32)b;
	
	if (lstPrcnt > prcnt)
		lstPrcnt = 0;
	
	if (prcnt - lstPrcnt > 0.125) {
		lstPrcnt = prcnt;
	} else {
		if (a != b) {
			return;
		}
	}
	
	printf("\r");
	__printf_call(3);
	printf(
		"%-16s " PRNT_RSET,
		info
	);
	printf("[%d / %d]", a, b);
	gPrintfProgressing = true;
	
	if (a == b) {
		gPrintfProgressing = false;
		printf("\n");
	}
}

s32 printf_get_answer(void) {
	char ans = 0;
	
	while (ans != 'n' && ans!= 'N' && ans != 'y' && ans != 'Y') {
		printf("\r" PRNT_GRAY "[" PRNT_DGRY "<" PRNT_GRAY "]: " PRNT_RSET);
		if (scanf("%c", &ans)) (void)0;
	}
	
	if (ans == 'N' || ans == 'n')
		return 0;
	
	return 1;
}

void printf_WinFix() {
	#ifdef _WIN32
		system("\0");
	#endif
}

// Lib
void* MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize) {
	if (haystack == NULL || needle == NULL)
		return NULL;
	register char* cur, * last;
	const char* cl = (const char*)haystack;
	const char* cs = (const char*)needle;
	
	/* we need something to compare */
	if (haystackSize == 0 || needleSize == 0)
		return NULL;
	
	/* "s" must be smaller or equal to "l" */
	if (haystackSize < needleSize)
		return NULL;
	
	/* special case where s_len == 1 */
	if (needleSize == 1)
		return memchr(haystack, (int)*cs, haystackSize);
	
	/* the last position where its possible to find "s" in "l" */
	last = (char*)cl + haystackSize - needleSize;
	
	for (cur = (char*)cl; cur <= last; cur++) {
		if (*cur == *cs && memcmp(cur, cs, needleSize) == 0)
			return cur;
	}
	
	return NULL;
}

void* MemMemCase(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize) {
	if (haystack == NULL || needle == NULL)
		return NULL;
	register char* cur, * last;
	char* cl = (char*)haystack;
	char* cs = (char*)needle;
	
	/* we need something to compare */
	if (haystackSize == 0 || needleSize == 0)
		return NULL;
	
	/* "s" must be smaller or equal to "l" */
	if (haystackSize < needleSize)
		return NULL;
	
	/* special case where s_len == 1 */
	if (needleSize == 1)
		return memchr(haystack, (int)*cs, haystackSize);
	
	/* the last position where its possible to find "s" in "l" */
	last = (char*)cl + haystackSize - needleSize;
	
	for (cur = (char*)cl; cur <= last; cur++) {
		if (tolower(*cur) == tolower(*cs) && String_CaseComp(cur, cs, needleSize) == 0)
			return cur;
	}
	
	return NULL;
}

void* MemMemAlign(u32 val, const void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	const u8* hay = haystack;
	const u8* nee = needle;
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize) {
		return NULL;
	}
	
	for (s32 i = 0;; i++) {
		if (i * val > haySize - needleSize)
			return NULL;
		
		if (hay[i * val] == nee[0]) {
			if (memcmp(&hay[i * val], nee, needleSize) == 0) {
				return (void*)&hay[i * val];
			}
		}
	}
}

void* MemMemU16(void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	u16* hay = haystack;
	const u16* nee = needle;
	const u16* neeEnd = nee + needleSize / sizeof(*nee);
	const u16* hayEnd = hay + ((haySize - needleSize) / sizeof(*hay));
	
	/* guarantee alignment */
	assert((((uPtr)haystack) & 0x1) == 0);
	assert((((uPtr)needle) & 0x1) == 0);
	assert((haySize & 0x1) == 0);
	assert((needleSize & 0x1) == 0);
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize)
		return NULL;
	
	while (hay <= hayEnd) {
		const u16* neeNow = nee;
		const u16* hayNow = hay;
		
		while (neeNow != neeEnd) {
			if (*neeNow != *hayNow)
				goto L_next;
			++neeNow;
			++hayNow;
		}
		
		return hay;
L_next:
		++hay;
	}
	
	return 0;
}

void* MemMemU32(void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	u32* hay = haystack;
	const u32* nee = needle;
	const u32* neeEnd = nee + needleSize / sizeof(*nee);
	const u32* hayEnd = hay + ((haySize - needleSize) / sizeof(*hay));
	
	/* guarantee alignment */
	assert((((uPtr)haystack) & 0x3) == 0);
	assert((((uPtr)needle) & 0x3) == 0);
	assert((haySize & 0x3) == 0);
	assert((needleSize & 0x3) == 0);
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize)
		return NULL;
	
	while (hay <= hayEnd) {
		const u32* neeNow = nee;
		const u32* hayNow = hay;
		
		while (neeNow != neeEnd) {
			if (*neeNow != *hayNow)
				goto L_next;
			++neeNow;
			++hayNow;
		}
		
		return hay;
L_next:
		++hay;
	}
	
	return 0;
}

void* MemMemU64(void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	u64* hay = haystack;
	const u64* nee = needle;
	const u64* neeEnd = nee + needleSize / sizeof(*nee);
	const u64* hayEnd = hay + ((haySize - needleSize) / sizeof(*hay));
	
	/* guarantee alignment */
	assert((((uPtr)haystack) & 0xf) == 0);
	assert((((uPtr)needle) & 0xf) == 0);
	assert((haySize & 0xf) == 0);
	assert((needleSize & 0xf) == 0);
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize)
		return NULL;
	
	while (hay <= hayEnd) {
		const u64* neeNow = nee;
		const u64* hayNow = hay;
		
		while (neeNow != neeEnd) {
			if (*neeNow != *hayNow)
				goto L_next;
			++neeNow;
			++hayNow;
		}
		
		return hay;
L_next:
		++hay;
	}
	
	return 0;
}

void ByteSwap(void* src, s32 size) {
	u32 buffer[64] = { 0 };
	u8* temp = (u8*)buffer;
	u8* srcp = src;
	
	for (s32 i = 0; i < size; i++) {
		temp[size - i - 1] = srcp[i];
	}
	
	for (s32 i = 0; i < size; i++) {
		srcp[i] = temp[i];
	}
}

void* Malloc(void* data, s32 size) {
	data = malloc(size);
	
	if (data == NULL) {
		printf_error("Could not allocate [0x%X] bytes.", size);
	}
	
	return data;
}

void* Calloc(void* data, s32 size) {
	data = Malloc(data, size);
	if (data != NULL) {
		memset(data, 0, size);
	}
	
	return data;
}

void* Realloc(void* data, s32 size) {
	data = realloc(data, size);
	
	if (data == NULL) {
		printf_error("Could not reallocate to [0x%X] bytes.", size);
	}
	
	return data;
}

void* Free(void* data) {
	if (data)
		free(data);
	
	return NULL;
}

s32 Touch(char* file) {
	MemFile mem = MemFile_Initialize();
	
	if (MemFile_LoadFile(&mem, file)) {
		return 1;
	}
	
	if (MemFile_SaveFile(&mem, file)) {
		MemFile_Free(&mem);
		
		return 1;
	}
	
	MemFile_Free(&mem);
	
	return 0;
}

s32 ParseArgs(char* argv[], char* arg, u32* parArg) {
	char* s = Tmp_Printf("%s", arg);
	char* ss = Tmp_Printf("-%s", arg);
	char* sss = Tmp_Printf("--%s", arg);
	char* tst[] = {
		s, ss, sss
	};
	
	for (s32 i = 1; argv[i] != NULL; i++) {
		for (s32 j = 0; j < ArrayCount(tst); j++) {
			if (strlen(argv[i]) == strlen(tst[j]))
				if (!strcmp(argv[i], tst[j])) {
					if (parArg != NULL)
						*parArg = i + 1;
					
					return i + 1;
				}
		}
	}
	
	return 0;
}

u32 Crc32(u8* s, u32 n) {
	u32 crc = 0xFFFFFFFF;
	
	for (u32 i = 0; i < n; i++) {
		u8 ch = s[i];
		for (s32 j = 0; j < 8; j++) {
			u32 b = (ch ^ crc) & 1;
			crc >>= 1;
			if (b) crc = crc ^ 0xEDB88320;
			ch >>= 1;
		}
	}
	
	return ~crc;
}

// Color
void Color_ToHSL(HSL8* dest, RGB8* src) {
	f32 r, g, b;
	f32 cmax, cmin, d;
	
	r = (f32)src->r / 255;
	g = (f32)src->g / 255;
	b = (f32)src->b / 255;
	
	cmax = fmax(r, (fmax(g, b)));
	cmin = fmin(r, (fmin(g, b)));
	dest->l = (cmax + cmin) / 2;
	d = cmax - cmin;
	
	if (cmax == cmin)
		dest->h = dest->s = 0;
	else {
		dest->s = dest->l > 0.5 ? d / (2 - cmax - cmin) : d / (cmax + cmin);
		
		if (cmax == r) {
			dest->h = (g - b) / d + (g < b ? 6 : 0);
		} else if (cmax == g) {
			dest->h = (b - r) / d + 2;
		} else if (cmax == b) {
			dest->h = (r - g) / d + 4;
		}
		dest->h /= 6.0;
	}
}

static f32 hue2rgb(f32 p, f32 q, f32 t) {
	if (t < 0.0) t += 1;
	if (t > 1.0) t -= 1;
	if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
	if (t < 1.0 / 2.0) return q;
	if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
	
	return p;
}

void Color_ToRGB(RGB8* dest, HSL8* src) {
	if (src->s == 0) {
		dest->r = dest->g = dest->b = src->l;
	} else {
		f32 q = src->l < 0.5 ? src->l * (1 + src->s) : src->l + src->s - src->l * src->s;
		f32 p = 2.0 * src->l - q;
		dest->r = hue2rgb(p, q, src->h + 1.0 / 3.0) * 255;
		dest->g = hue2rgb(p, q, src->h) * 255;
		dest->b = hue2rgb(p, q, src->h - 1.0 / 3.0) * 255;
	}
}

// File
void* File_Load(void* destSize, char* filepath) {
	s32 size;
	void* dest;
	s32* ss = destSize;
	
	FILE* file = fopen(filepath, "rb");
	
	if (file == NULL) {
		printf_error("Could not open file [%s]", filepath);
	}
	
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	dest = Malloc(0, size);
	if (dest == NULL) {
		printf_error("Failed to malloc [0x%X] bytes to store data from [%s].", size, filepath);
	}
	rewind(file);
	if (fread(dest, sizeof(char), size, file)) {
	}
	fclose(file);
	
	ss[0] = size;
	
	return dest;
}

void File_Save(char* filepath, void* src, s32 size) {
	FILE* file = fopen(filepath, "w");
	
	if (file == NULL) {
		printf_error("Failed to fopen file [%s].", filepath);
	}
	
	fwrite(src, sizeof(char), size, file);
	fclose(file);
}

void* File_Load_ReqExt(void* size, char* filepath, const char* ext) {
	if (MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		return File_Load(size, filepath);
	}
	printf_error("[%s] does not match extension [%s]", filepath, ext);
	
	return 0;
}

void File_Save_ReqExt(char* filepath, void* src, s32 size, const char* ext) {
	if (MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		File_Save(filepath, src, size);
		
		return;
	}
	
	printf_error("[%s] does not match extension [%s]", src, ext);
}

// MemFile
MemFile MemFile_Initialize() {
	return (MemFile) { .param.initKey = 0xD0E0A0D0B0E0E0F0 };
}

void MemFile_Params(MemFile* memFile, ...) {
	va_list args;
	u32 cmd;
	u32 arg;
	
	va_start(args, memFile);
	for (;;) {
		cmd = va_arg(args, uPtr);
		
		if (cmd == MEM_END) {
			break;
		}
		
		arg = va_arg(args, uPtr);
		
		if (cmd == MEM_CLEAR) {
			cmd = arg;
			arg = 0;
		}
		
		if (cmd == MEM_FILENAME) {
			memFile->param.getName = arg > 0 ? true : false;
		} else if (cmd == MEM_CRC32) {
			memFile->param.getCrc = arg > 0 ? true : false;
		} else if (cmd == MEM_ALIGN) {
			memFile->param.align = arg;
		} else if (cmd == MEM_REALLOC) {
			memFile->param.realloc = arg > 0 ? true : false;
		}
	}
	va_end(args);
}

void MemFile_Malloc(MemFile* memFile, u32 size) {
	if (memFile->param.initKey != 0xD0E0A0D0B0E0E0F0)
		*memFile = MemFile_Initialize();
	memFile->data = malloc(size);
	memset(memFile->data, 0, size);
	
	if (memFile->data == NULL) {
		printf_warning("Failed to malloc [0x%X] bytes.", size);
	}
	
	memFile->memSize = size;
}

void MemFile_Realloc(MemFile* memFile, u32 size) {
	if (memFile->memSize > size)
		return;
	
	// Make sure to have enough space
	if (size < memFile->memSize + 0x10000) {
		size += 0x10000;
	}
	
	memFile->data = realloc(memFile->data, size);
	memFile->memSize = size;
}

void MemFile_Rewind(MemFile* memFile) {
	memFile->seekPoint = 0;
}

s32 MemFile_Write(MemFile* dest, void* src, u32 size) {
	if (dest->seekPoint + size > dest->memSize) {
		if (!dest->param.realloc) {
			printf_warning_align(
				"MemSize exceeded",
				"%.2fkB / %.2fkB",
				BinToKb(dest->dataSize),
				BinToKb(dest->memSize)
			);
			
			return 1;
		}
		
		MemFile_Realloc(dest, dest->memSize * 2);
	}
	
	if (dest->seekPoint + size > dest->dataSize) {
		dest->dataSize = dest->seekPoint + size;
	}
	memcpy(&dest->cast.u8[dest->seekPoint], src, size);
	dest->seekPoint += size;
	
	if (dest->param.align) {
		if ((dest->seekPoint % dest->param.align) != 0) {
			MemFile_Align(dest, dest->param.align);
		}
	}
	
	return 0;
}

s32 MemFile_Append(MemFile* dest, MemFile* src) {
	return MemFile_Write(dest, src->data, src->dataSize);
}

void MemFile_Align(MemFile* src, u32 align) {
	if ((src->seekPoint % align) != 0) {
		u64 wow[2] = { 0 };
		u32 size = align - (src->seekPoint % align);
		
		MemFile_Write(src, wow, size);
	}
}

s32 MemFile_Printf(MemFile* dest, const char* fmt, ...) {
	char buffer[512];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(
		buffer,
		ArrayCount(buffer),
		fmt,
		args
	);
	va_end(args);
	
	return MemFile_Write(dest, buffer, strlen(buffer));
}

s32 MemFile_Read(MemFile* src, void* dest, u32 size) {
	if (src->seekPoint + size > src->dataSize) {
		return 1;
	}
	
	memcpy(dest, &src->cast.u8[src->seekPoint], size);
	src->seekPoint += size;
	
	return 0;
}

void* MemFile_Seek(MemFile* src, u32 seek) {
	if (seek > src->memSize) {
		printf_error("!");
	}
	src->seekPoint = seek;
	
	return (void*)&src->cast.u8[seek];
}

s32 MemFile_LoadFile(MemFile* memFile, char* filepath) {
	u32 tempSize;
	FILE* file = fopen(filepath, "rb");
	struct stat sta;
	
	if (file == NULL) {
		printf_warning("Failed to open file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	if (memFile->data == NULL) {
		MemFile_Malloc(memFile, tempSize);
		memFile->memSize = memFile->dataSize = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else {
		if (memFile->memSize < tempSize)
			MemFile_Realloc(memFile, tempSize * 2);
		memFile->dataSize = tempSize;
	}
	
	rewind(file);
	if (fread(memFile->data, 1, memFile->dataSize, file)) {
	}
	fclose(file);
	stat(filepath, &sta);
	memFile->info.age = sta.st_mtime;
	strcpy(memFile->info.name, filepath);
	
	if (memFile->param.getCrc) {
		memFile->info.crc32 = Crc32(memFile->data, memFile->dataSize);
	}
	
	return 0;
}

s32 MemFile_LoadFile_String(MemFile* memFile, char* filepath) {
	u32 tempSize;
	FILE* file = fopen(filepath, "r");
	struct stat sta;
	
	if (file == NULL) {
		printf_warning("Failed to open file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	if (memFile->data == NULL) {
		MemFile_Malloc(memFile, tempSize + 0x10);
		memFile->memSize = tempSize + 0x10;
		memFile->dataSize = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else {
		if (memFile->memSize < tempSize)
			MemFile_Realloc(memFile, tempSize * 2);
		memFile->dataSize = tempSize;
	}
	
	rewind(file);
	memFile->dataSize = fread(memFile->data, 1, memFile->dataSize, file);
	fclose(file);
	memFile->cast.u8[memFile->dataSize] = '\0';
	
	stat(filepath, &sta);
	memFile->info.age = sta.st_mtime;
	strcpy(memFile->info.name, filepath);
	
	if (memFile->param.getCrc) {
		memFile->info.crc32 = Crc32(memFile->data, memFile->dataSize);
	}
	
	return 0;
}

s32 MemFile_SaveFile(MemFile* memFile, char* filepath) {
	FILE* file = fopen(filepath, "wb");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	fwrite(memFile->data, sizeof(char), memFile->dataSize, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_SaveFile_String(MemFile* memFile, char* filepath) {
	FILE* file = fopen(filepath, "w");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	fwrite(memFile->data, sizeof(char), memFile->dataSize, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_LoadFile_ReqExt(MemFile* memFile, char* filepath, const char* ext) {
	if (MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		return MemFile_LoadFile(memFile, filepath);
	}
	printf_warning("[%s] does not match extension [%s]", filepath, ext);
	
	return 1;
}

s32 MemFile_SaveFile_ReqExt(MemFile* memFile, char* filepath, s32 size, const char* ext) {
	if (MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		return MemFile_SaveFile(memFile, filepath);
	}
	
	printf_warning("[%s] does not match extension [%s]", filepath, ext);
	
	return 1;
}

void MemFile_Free(MemFile* memFile) {
	if (memFile->data) {
		free(memFile->data);
		
		memset(memFile, 0, sizeof(struct MemFile));
	}
}

void MemFile_Reset(MemFile* memFile) {
	memFile->dataSize = 0;
	memFile->seekPoint = 0;
}

void MemFile_Clear(MemFile* memFile) {
	memset(memFile->data, 0, memFile->memSize);
	MemFile_Reset(memFile);
}

// String
u32 String_GetHexInt(char* string) {
	return strtoul(string, NULL, 16);
}

s32 String_GetInt(char* string) {
	if (!memcmp(string, "0x", 2)) {
		return strtoul(string, NULL, 16);
	} else {
		return strtol(string, NULL, 10);
	}
}

f32 String_GetFloat(char* string) {
	if (StrStr(string, ",")) {
		string = Tmp_String(string);
		String_Replace(string, ",", ".");
	}
	
	return strtod(string, NULL);
}

s32 String_GetLineCount(char* str) {
	s32 line = 1;
	s32 i = 0;
	
	while (str[i++] != '\0') {
		if (str[i] == '\n' && str[i + 1] != '\0')
			line++;
	}
	
	return line;
}

s32 String_CaseComp(char* a, char* b, u32 compSize) {
	u32 wow = 0;
	
	for (s32 i = 0; i < compSize; i++) {
		wow = tolower(a[i]) - tolower(b[i]);
		if (wow)
			return 1;
	}
	
	return 0;
}

static void __GetSlashAndPoint(const char* src, s32* slash, s32* point) {
	s32 strSize = strlen(src);
	
	for (s32 i = strSize; i > 0; i--) {
		if (*point == 0 && src[i] == '.') {
			*point = i;
		}
		if (src[i] == '/' || src[i] == '\\') {
			*slash = i;
			break;
		}
	}
}

char* String_GetLine(const char* str, s32 line) {
	char* buffer;
	s32 iLine = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != '\n') {
			while (str[i + j] != '\n' && str[i + j] != '\0') {
				j++;
			}
			
			iLine++;
			
			if (iLine == line) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	buffer = Tmp_Alloc(j + 1);
	
	memcpy(buffer, &str[i], j);
	buffer[j] = '\0';
	
	return buffer;
}

char* String_GetWord(const char* str, s32 word) {
	char* buffer;
	s32 iWord = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i + j] > ' ') {
			while (str[i + j] > ' ') {
				j++;
			}
			
			iWord++;
			
			if (iWord == word) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	buffer = Tmp_Alloc(j + 1);
	
	memcpy(buffer, &str[i], j);
	buffer[j] = '\0';
	
	return buffer;
}

char* String_GetPath(const char* src) {
	char* buffer;
	s32 point = 0;
	s32 slash = 0;
	
	if (src == NULL)
		return NULL;
	
	__GetSlashAndPoint(src, &slash, &point);
	
	if (slash == 0)
		slash = -1;
	
	buffer = Tmp_Alloc(slash + 1 + 1);
	
	memcpy(buffer, src, slash + 1);
	buffer[slash + 1] = '\0';
	
	return buffer;
}

char* String_GetBasename(const char* src) {
	char* buffer;
	s32 point = 0;
	s32 slash = 0;
	
	if (src == NULL)
		return NULL;
	
	__GetSlashAndPoint(src, &slash, &point);
	
	// Offset away from the slash
	if (slash > 0)
		slash++;
	
	if (point < slash || slash + point == 0) {
		point = slash + 1;
		while (src[point] > ' ') point++;
	}
	
	buffer = Tmp_Alloc(point - slash + 1);
	
	memcpy(buffer, &src[slash], point - slash);
	buffer[point - slash] = '\0';
	
	return buffer;
}

char* String_GetFilename(const char* src) {
	char* buffer;
	s32 point = 0;
	s32 slash = 0;
	s32 ext = 0;
	
	if (src == NULL)
		return NULL;
	
	__GetSlashAndPoint(src, &slash, &point);
	
	// Offset away from the slash
	if (slash > 0)
		slash++;
	
	if (src[point + ext] == '.') {
		ext++;
		while (isalnum(src[point + ext])) ext++;
	}
	
	if (point < slash || slash + point == 0) {
		point = slash + 1;
		while (src[point] > ' ') point++;
	}
	
	buffer = Tmp_Alloc(point - slash + ext + 1);
	
	memcpy(buffer, &src[slash], point - slash + ext);
	buffer[point - slash + ext] = '\0';
	
	return buffer;
}

s32 String_GetPathNum(const char* src) {
	s32 dir = -1;
	
	for (s32 i = 0; i < strlen(src); i++) {
		if (src[i] == '/')
			dir++;
	}
	
	return dir + 1;
}

char* String_GetFolder(const char* src, s32 num) {
	char* buffer;
	s32 start = -1;
	s32 end;
	
	if (src == NULL)
		return NULL;
	
	if (num < 0) {
		num = String_GetPathNum(src) - 1;
	}
	
	for (s32 temp = 0;;) {
		if (temp >= num)
			break;
		if (src[start + 1] == '/')
			temp++;
		start++;
	}
	start++;
	end = start + 1;
	
	while (src[end] != '/') {
		if (src[end] == '\0')
			printf_error("Could not solve folder for [%s]", src);
		end++;
	}
	end++;
	
	buffer = Tmp_Alloc(end - start + 1);
	
	memcpy(buffer, &src[start], end - start);
	buffer[end - start] = '\0';
	
	return buffer;
}

char* String_GetSpacedArg(char* argv[], s32 cur) {
	char tempBuf[512];
	s32 i = cur + 1;
	
	if (argv[i] && argv[i][0] != '-' && argv[i][1] != '-') {
		strcpy(tempBuf, argv[cur]);
		
		while (argv[i] && argv[i][0] != '-' && argv[i][1] != '-') {
			strcat(tempBuf, " ");
			strcat(tempBuf, argv[i++]);
		}
		
		return Tmp_String(tempBuf);
	}
	
	return argv[cur];
}

char* String_Line(char* str, s32 line) {
	s32 iLine = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != '\n') {
			while (str[i + j] != '\n' && str[i + j] != '\0') {
				j++;
			}
			
			iLine++;
			
			if (iLine == line) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	return &str[i];
}

char* String_Word(char* str, s32 word) {
	s32 iWord = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i + j] > ' ') {
			while (str[i + j] > ' ') {
				j++;
			}
			
			iWord++;
			
			if (iWord == word) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	return &str[i];
}

void String_CaseToLow(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'A' && s[k] <= 'Z') {
			s[k] = s[k] + 32;
		}
	}
}

void String_CaseToUp(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'a' && s[k] <= 'z') {
			s[k] = s[k] - 32;
		}
	}
}

void String_Insert(char* point, char* insert) {
	s32 insLen = strlen(insert);
	char* insEnd = point + insLen;
	s32 remLen = strlen(point);
	
	memmove(insEnd, point, remLen + 1);
	insEnd[remLen] = 0;
	memcpy(point, insert, insLen);
}

void String_InsertExt(char* origin, char* insert, s32 pos, s32 size) {
	s32 inslen = strlen(insert);
	
	if (pos >= size)
		return;
	
	if (size - pos - inslen > 0)
		memmove(&origin[pos + inslen], &origin[pos], size - pos - inslen);
	
	for (s32 j = 0; j < inslen; pos++, j++) {
		origin[pos] = insert[j];
	}
}

void String_Remove(char* point, s32 amount) {
	char* get = point + amount;
	s32 len = strlen(get);
	
	if (len)
		memcpy(point, get, strlen(get));
	point[len] = 0;
}

s32 String_Replace(char* src, char* word, char* replacement) {
	s32 diff = 0;
	char* ptr;
	
	if (strlen(word) == 1 && strlen(replacement) == 1) {
		for (s32 i = 0; i < strlen(src); i++) {
			if (src[i] == word[0]) {
				src[i] = replacement[0];
				break;
			}
		}
		
		return true;
	}
	
	ptr = StrStr(src, word);
	
	while (ptr != NULL) {
		String_Remove(ptr, strlen(word));
		String_Insert(ptr, replacement);
		ptr = StrStr(src, word);
		diff = true;
	}
	
	return diff;
}

void String_SwapExtension(char* dest, char* src, const char* ext) {
	strcpy(dest, String_GetPath(src));
	strcat(dest, String_GetBasename(src));
	strcat(dest, ext);
}

s32 sConfigSuppression;

void Config_SuppressNext(void) {
	sConfigSuppression = 1;
}
// Config
char* Config_Get(MemFile* memFile, char* name) {
	u32 lineCount = String_GetLineCount(memFile->data);
	
	for (s32 i = 0; i < lineCount; i++) {
		if (!strcmp(String_GetWord(String_GetLine(memFile->data, i), 0), name)) {
			char* word = String_Word(String_Line(memFile->data, i), 2);
			char* ret;
			
			if (word[0] == '"') {
				s32 j = 0;
				for (;; j++) {
					if (word[j + 1] == '"') {
						break;
					}
				}
				ret = Tmp_Alloc(j);
				memcpy(ret, &word[1], j);
			} else {
				word = String_GetWord(String_Line(memFile->data, i), 2);
				ret = Tmp_Alloc(strlen(word) + 1);
				strcpy(ret, word);
			}
			
			return ret;
		}
	}
	
	return NULL;
}

s32 Config_GetBool(MemFile* memFile, char* boolName) {
	char* ptr;
	
	ptr = Config_Get(memFile, boolName);
	if (ptr) {
		char* word = ptr;
		if (!strcmp(word, "true")) {
			return true;
		}
		if (!strcmp(word, "false")) {
			return false;
		}
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing bool [%s]", memFile->info.name, boolName);
	else sConfigSuppression++;
	
	return 0;
}

s32 Config_GetOption(MemFile* memFile, char* stringName, char* strList[]) {
	char* ptr;
	char* word;
	s32 i = 0;
	
	ptr = Config_Get(memFile, stringName);
	if (ptr) {
		word = ptr;
		while (strList[i] != NULL && !StrStr(word, strList[i]))
			i++;
		
		if (strList != NULL)
			return i;
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing option [%s]", memFile->info.name, stringName);
	else sConfigSuppression++;
	
	return 0;
}

s32 Config_GetInt(MemFile* memFile, char* intName) {
	char* ptr;
	
	ptr = Config_Get(memFile, intName);
	if (ptr) {
		return String_GetInt(ptr);
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing integer [%s]", memFile->info.name, intName);
	else sConfigSuppression++;
	
	return 0;
}

char* Config_GetString(MemFile* memFile, char* stringName) {
	char* ptr;
	
	ptr = Config_Get(memFile, stringName);
	if (ptr) {
		return ptr;
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing string [%s]", memFile->info.name, stringName);
	else sConfigSuppression++;
	
	return NULL;
}

f32 Config_GetFloat(MemFile* memFile, char* floatName) {
	char* ptr;
	
	ptr = Config_Get(memFile, floatName);
	if (ptr) {
		return String_GetFloat(ptr);
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing float [%s]", memFile->info.name, floatName);
	else sConfigSuppression++;
	
	return 0;
}

#include <signal.h>

#define FAULT_BUFFER_SIZE 256
#define FAULT_LOG_NUM     18

char* sLogMsg[FAULT_LOG_NUM];
char* sLogFunc[FAULT_LOG_NUM];
u32 sLogLine[FAULT_LOG_NUM];

static void Log_Signal(int arg) {
	const char* errorMsg[] = {
		"\a0",
		"\a1",
		"\aForced Close", // SIGINT
		"\a3",
		"\aSIGILL",
		"\a5",
		"\aSIGABRT_COMPAT",
		"\a7",
		"\aSIGFPE",
		"\a9",
		"\a10",
		"\aSegmentation Fault",
		"\a12",
		"\a13",
		"\a14",
		"\aSIGTERM",
		"\a16",
		"\a17",
		"\a18",
		"\a19",
		"\a20",
		"\aSIGBREAK",
		"\aSIGABRT",
		
		"\aUNDEFINED",
	};
	u32 msgsNum = 0;
	
	printf("\n");
	if (arg != 0xDEADBEEF)
		printf("" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ " PRNT_REDD "%s " PRNT_DGRY "]\n", errorMsg[ClampMax(arg, 23)]);
	else
		printf("" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ " PRNT_REDD "LOG " PRNT_DGRY "]\n");
	
	for (s32 i = FAULT_LOG_NUM - 1; i >= 0; i--) {
		if (strlen(sLogMsg[i]) > 0) {
			if (msgsNum == 0 || strcmp(sLogFunc[i], sLogFunc[i + 1]))
				printf("" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_GREN " %s"PRNT_RSET "();" PRNT_RSET "\n", sLogFunc[i]);
			printf(
				"" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ %4d ]" PRNT_RSET " %s" PRNT_RSET "\n",
				sLogLine[i],
				sLogMsg[i]
			);
			msgsNum++;
		}
	}
	
	if (arg != 0xDEADBEEF) {
		printf(
			"\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_YELW " Provide this log to the developer." PRNT_RSET "\n"
		);
		exit(EXIT_FAILURE);
	}
}

void Log_Init() {
	for (s32 i = 1; i < 1234; i++)
		signal(i, Log_Signal);
	
	for (s32 i = 0; i < FAULT_LOG_NUM; i++) {
		sLogMsg[i] = Calloc(0, FAULT_BUFFER_SIZE);
		sLogFunc[i] = Calloc(0, FAULT_BUFFER_SIZE * 0.25);
	}
}

void Log_Free() {
	for (s32 i = 0; i < FAULT_LOG_NUM; i++) {
		Free(sLogMsg[i]);
		Free(sLogFunc[i]);
	}
}

void Log_Print() {
	Log_Signal(0xDEADBEEF);
}

void Log(const char* func, u32 line, const char* txt, ...) {
	va_list args;
	
	for (s32 i = FAULT_LOG_NUM - 1; i > 0; i--) {
		strcpy(sLogMsg[i], sLogMsg[i - 1]);
		strcpy(sLogFunc[i], sLogFunc[i - 1]);
		sLogLine[i] = sLogLine[i - 1];
	}
	
	va_start(args, txt);
	vsnprintf(sLogMsg[0], FAULT_BUFFER_SIZE, txt, args);
	va_end(args);
	
	strcpy(sLogFunc[0], func);
	sLogLine[0] = line;
}

f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep) {
	if (*pValue != target) {
		f32 stepSize = (target - *pValue) * fraction;
		
		if ((stepSize >= minStep) || (stepSize <= -minStep)) {
			if (stepSize > step) {
				stepSize = step;
			}
			
			if (stepSize < -step) {
				stepSize = -step;
			}
			
			*pValue += stepSize;
		} else {
			if (stepSize < minStep) {
				*pValue += minStep;
				stepSize = minStep;
				
				if (target < *pValue) {
					*pValue = target;
				}
			}
			if (stepSize > -minStep) {
				*pValue += -minStep;
				
				if (*pValue < target) {
					*pValue = target;
				}
			}
		}
	}
	
	return fabsf(target - *pValue);
}

f32 Math_SplineFloat(f32 u, f32* res, f32* point0, f32* point1, f32* point2, f32* point3) {
	f32 coeff[4];
	
	coeff[0] = (1.0f - u) * (1.0f - u) * (1.0f - u) / 6.0f;
	coeff[1] = u * u * u / 2.0f - u * u + 2.0f / 3.0f;
	coeff[2] = -u * u * u / 2.0f + u * u / 2.0f + u / 2.0f + 1.0f / 6.0f;
	coeff[3] = u * u * u / 6.0f;
	
	if (res) {
		*res = (coeff[0] * *point0) + (coeff[1] * *point1) + (coeff[2] * *point2) + (coeff[3] * *point3);
		
		return *res;
	}
	
	return (coeff[0] * *point0) + (coeff[1] * *point1) + (coeff[2] * *point2) + (coeff[3] * *point3);
}