/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// GemRB.cpp : Defines the entry point for the application.

#include "win32def.h" // logging
#include <clocale> //language encoding

#include "Interface.h"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef VITA
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/apputil.h>

#include <psp2/fios2.h>
#include <psp2/kernel/clib.h>

// allocating memory for application on Vita
int _newlib_heap_size_user = 330 * 1024 * 1024;
int sceLibcHeapSize = 8 * 1024 * 1024;
char *vitaArgv[3];
char configPath[25];

void VitaSetArguments(int *argc, char **argv[])
{
	SceAppUtilInitParam appUtilParam;
	SceAppUtilBootParam appUtilBootParam;
	memset(&appUtilParam, 0, sizeof(SceAppUtilInitParam));
	memset(&appUtilBootParam, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&appUtilParam, &appUtilBootParam);
	SceAppUtilAppEventParam eventParam;
	memset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
	sceAppUtilReceiveAppEvent(&eventParam);
	int vitaArgc = 1;
	vitaArgv[0] = (char*)"";
	// 0x05 probably corresponds to psla event sent from launcher screen of the app in LiveArea
	if (eventParam.type == 0x05) {
		SceAppUtilLiveAreaParam appUtilLiveParam;
		memset(&appUtilLiveParam, 0, sizeof(SceAppUtilLiveAreaParam));
		sceAppUtilAppEventParseLiveArea(&eventParam, &appUtilLiveParam);
		vitaArgv[1] = (char*)"-c";
		sprintf(configPath, "ux0:data/GemRB/%s.cfg", appUtilLiveParam.param);
		vitaArgv[2] = configPath;
		vitaArgc = 3;
	}
	*argc = vitaArgc;
	*argv = vitaArgv;
}

#define MAX_PATH_LENGTH 256
#define RAMCACHEBLOCKNUM 512
#define RAMCACHEBLOCKSIZE 64*1024
#define DATA_PATH "ux0:data/GemRB"

static int64_t g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(1024) / sizeof(int64_t) + 1];
static int64_t g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(1024, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_DHStorage[SCE_FIOS_DH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];

static SceFiosRamCacheContext g_RamCacheContext = SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER;
static void *g_RamCacheWorkBuffer;

int fios_init(void)
{
	int res;

	SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
	params.opStorage.pPtr = g_OpStorage;
	params.opStorage.length = sizeof(g_OpStorage);
	params.chunkStorage.pPtr = g_ChunkStorage;
	params.chunkStorage.length = sizeof(g_ChunkStorage);
	params.fhStorage.pPtr = g_FHStorage;
	params.fhStorage.length = sizeof(g_FHStorage);
	params.dhStorage.pPtr = g_DHStorage;
	params.dhStorage.length = sizeof(g_DHStorage);
	params.pathMax = MAX_PATH_LENGTH;

	params.threadAffinity[SCE_FIOS_IO_THREAD] = 0x20000;
	params.threadAffinity[SCE_FIOS_CALLBACK_THREAD] = 0;
	params.threadAffinity[SCE_FIOS_DECOMPRESSOR_THREAD] = 0;

	params.threadPriority[SCE_FIOS_IO_THREAD] = 64;
	params.threadPriority[SCE_FIOS_CALLBACK_THREAD] = 191;
	params.threadPriority[SCE_FIOS_DECOMPRESSOR_THREAD] = 191;

	res = sceFiosInitialize(&params);
	if (res < 0)
	return res;

	g_RamCacheWorkBuffer = memalign(8, RAMCACHEBLOCKNUM * RAMCACHEBLOCKSIZE);
	if (!g_RamCacheWorkBuffer)
	return -1;

	g_RamCacheContext.pPath = DATA_PATH;
	g_RamCacheContext.pWorkBuffer = g_RamCacheWorkBuffer;
	g_RamCacheContext.workBufferSize = RAMCACHEBLOCKNUM * RAMCACHEBLOCKSIZE;
	g_RamCacheContext.blockSize = RAMCACHEBLOCKSIZE;
	res = sceFiosIOFilterAdd(0, sceFiosIOFilterCache, &g_RamCacheContext);
	if (res < 0)
	return res;

	return 0;
}

void fios_terminate() {
	sceFiosIOFilterRemove(0);
	sceFiosTerminate();
	free(g_RamCacheWorkBuffer);
}
#endif

using namespace GemRB;

#ifdef ANDROID
#include <SDL.h>
// if/when android moves to SDL 1.3 remove these special functions.
// SDL 1.3 fires window events for these conditions that are handled in SDLVideo.cpp.
// see SDL_WINDOWEVENT_MINIMIZED and SDL_WINDOWEVENT_RESTORED
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
#include "Audio.h"

// pause audio playing if app goes in background
static void appPutToBackground()
{
  core->GetAudioDrv()->Pause();
}
// resume audio playing if app return to foreground
static void appPutToForeground()
{
  core->GetAudioDrv()->Resume();
}
#endif
#endif

int main(int argc, char* argv[])
{
#ifdef VITA
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	sceKernelChangeThreadPriority(0, 64);
	sceKernelChangeThreadCpuAffinityMask(0, 0x40000);

	//fios_init();

	// Selecting game config from init params
	VitaSetArguments(&argc, &argv);
#endif

	setlocale(LC_ALL, "");
#ifdef HAVE_SETENV
	setenv("SDL_VIDEO_X11_WMCLASS", argv[0], 0);
#	ifdef ANDROID
		setenv("GEM_DATA", SDL_AndroidGetExternalStoragePath(), 1);
#	endif
#endif

#ifdef M_TRIM_THRESHOLD
// Prevent fragmentation of the heap by malloc (glibc).
//
// The default threshold is 128*1024, which can result in a large memory usage
// due to fragmentation since we use a lot of small objects. On the other hand
// if the threshold is too low, free() starts to permanently ask the kernel
// about shrinking the heap.
	#if defined(HAVE_UNISTD_H) && !defined(VITA)
		int pagesize = sysconf(_SC_PAGESIZE);
	#else
		int pagesize = 4*1024;
	#endif
	mallopt(M_TRIM_THRESHOLD, 5*pagesize);
#endif

	Interface::SanityCheck(VERSION_GEMRB);

	core = new Interface();
	CFGConfig* config = new CFGConfig(argc, argv);
	if (core->Init( config ) == GEM_ERROR) {
		delete config;
		delete( core );
		InitializeLogging();
		Log(MESSAGE, "Main", "Aborting due to fatal error...");
		ShutdownLogging();
#ifdef VITA
		sceKernelExitProcess(0);
#endif
		return -1;
	}
	InitializeLogging();
	delete config;
#ifdef ANDROID
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
    SDL_ANDROID_SetApplicationPutToBackgroundCallback(&appPutToBackground, &appPutToForeground);
#endif
#endif
	core->Main();
	delete( core );
	ShutdownLogging();
#ifdef VITA
	//fios_terminate();
	sceKernelExitProcess(0);
#endif
	return 0;
}
