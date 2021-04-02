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

#include "Interface.h"
#include "VitaLogger.h"

#include <malloc.h>
#include <python2.7/Python.h>

#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/apputil.h>
#include <psp2/fios2.h>

#ifdef VITA_CACHE
#include "../platforms/vita/VitaCache.h"
#endif

#define MAX_PATH_LENGTH 256
#define RAMCACHEBLOCKNUM 32
#define RAMCACHEBLOCKSIZE 128*1024
#define DATA_PATH "ux0:data/GemRB"

static int64_t g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(1024) / sizeof(int64_t) + 1];
static int64_t g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(1024, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_DHStorage[SCE_FIOS_DH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];

static SceFiosRamCacheContext g_RamCacheContext = SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER;
static void *g_RamCacheWorkBuffer;

// allocating memory for application on Vita
int _newlib_heap_size_user = 335 * 1024 * 1024;

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

	params.threadAffinity[SCE_FIOS_IO_THREAD] = 0x10000;
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

void fios_terminate()
{
	sceFiosIOFilterRemove(0);
	sceFiosTerminate();
	free(g_RamCacheWorkBuffer);
}

using namespace GemRB;

int main(int argc, char* argv[])
{
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	// OpenAL fails to init with priority 64
	sceKernelChangeThreadPriority(0, 128);
	//sceKernelChangeThreadCpuAffinityMask(0, 0x40000);

	mallopt(M_TRIM_THRESHOLD, 20 * 1024);

	fios_init();

	// Selecting game config from init params
	VitaSetArguments(&argc, &argv);

	setlocale(LC_ALL, "");
	
	AddLogWriter(createVitaLogger());
	ToggleLogging(true);

	Interface::SanityCheck(VERSION_GEMRB);
	
	//Py_Initialize crashes on Vita otherwise
	Py_NoSiteFlag = 1;

	core = new Interface();
	CFGConfig* config = new CFGConfig(argc, argv);

	if (core->Init(config) == GEM_ERROR) {
		delete config;
		delete(core);
		Log(MESSAGE, "Main", "Aborting due to fatal error...");
		ToggleLogging(false);
		return sceKernelExitProcess(-1);
	}

	delete config;

#ifdef VITA_CACHE
	VitaCache::Init();
#endif

	core->Main();
	delete(core);
	ToggleLogging(false);

	fios_terminate();
	return sceKernelExitProcess(0);
}
