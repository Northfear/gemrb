#include "../../includes/win32def.h"
#include "stdio.h"
#include "stdlib.h"
#include "PluginMgr.h"
#include "Plugin.h"
#include "Interface.h"

#ifdef WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif

typedef unsigned long (* ulongvoid)(void);
typedef char * (* charvoid)(void);
typedef int (* intvoid)(void);
typedef ClassDesc *(* cdint)(int);

extern Interface * core;
extern HANDLE hConsole;

PluginMgr::PluginMgr(char * pluginpath)
{
	printMessage("PluginMgr", "Loading Plugins...\n", WHITE);
	char path[_MAX_PATH];
	strcpy(path, pluginpath);
	/*
  Since Win32 and Linux uses two different methods for loading external Dynamic Link Libraries,
  we have to take this in consideration using some #ifdef statements.

  Win32 uses the _findfirst/_findnext functions for iterating through all the files in the
  plugin directory.

  Linux simply uses a readdir function to walk through the directory and a fnmatch function to
  search files with ".so" extension.
	*/
#ifdef WIN32
	//The windows _findfirst/_findnext functions allow the use of wildcards so we'll use them :)
	struct _finddata_t c_file;
	long hFile;
	strcat(path, "plugins\\*.dll");
	printMessage("PluginMgr", "Searching for plugins in: ", WHITE);
	printf("%s\n", path);
	if((hFile = _findfirst(path, &c_file)) == -1L) //If there is no file matching our search
#else
	strcat(path, "plugins");
	printMessage("PluginMgr", "Searching for plugins in: ", WHITE);
	printf("%s\n", path);
	DIR * dir = opendir(path);
	if(dir == NULL) //If we cannot open the Directory
#endif
		return; //Simply return
#ifndef WIN32  //Linux Statement
	struct dirent * de = readdir(dir);  //Lookup the first entry in the Directory
	if(de == NULL) //If no entry exists just return
		return;
#endif
	ulongvoid LibVersion;
	charvoid LibDescription;
	intvoid   LibNumberClasses;
	cdint	  LibClassDesc;
	do { //Iterate through all the available modules to load
#ifdef WIN32
		strcpy(path, pluginpath);
		strcat(path, "plugins\\");
		strcat(path, c_file.name);
		printBracket("PluginMgr", LIGHT_WHITE);
		printf(": Loading: ");
		textcolor(LIGHT_WHITE);
		printf("%s", c_file.name);
		textcolor(WHITE);
		printf("...");
		HMODULE hMod = LoadLibrary(path); //Try to load the Module
		if(hMod == NULL) {
			printBracket("ERROR", LIGHT_RED);
			printf("\nCannot Load Module, Skipping...\n");
			continue;
		}
		printStatus("OK", LIGHT_GREEN);
		LibVersion = (ulongvoid)GetProcAddress(hMod, "LibVersion");
		LibDescription = (charvoid)GetProcAddress(hMod, "LibDescription");
		LibNumberClasses = (intvoid)GetProcAddress(hMod, "LibNumberClasses");
		LibClassDesc = (cdint)GetProcAddress(hMod, "LibClassDesc");
#else
		if(fnmatch("*.so", de->d_name, 0) != 0) //If the current file has no ".so" extension, skip it
			continue;
		strcpy(path, pluginpath);
		strcat(path, "plugins/");
		strcat(path, de->d_name);
		printBracket("PluginMgr", LIGHT_WHITE);
		printf(": Loading: ");
		textcolor(LIGHT_WHITE);
		printf("%s", path);
		textcolor(WHITE);
		printf("...");
		void* hMod = dlopen(path, RTLD_NOW); //Try to load the Module
		if(hMod == NULL) {
			printBracket("ERROR", LIGHT_RED);
			printf("\nCannot Load Module, Skipping...\n");
			continue;
		}
		printStatus("OK", LIGHT_GREEN);
		/*
		GCC Version 3.2.x has changed the Export Names of the DLLs this statement is a simple
		hack to make GemRB run on every version.
		*/
#ifdef GCC_OLD
		LibVersion = (ulongvoid)dlsym(hMod, "LibVersion__Fv");
		LibDescription = (charvoid)dlsym(hMod, "LibDescription__Fv");
		LibNumberClasses = (intvoid)dlsym(hMod, "LibNumberClasses__Fv");
		LibClassDesc = (cdint)dlsym(hMod, "LibClassDesc__Fi");
#else
		LibVersion = (ulongvoid)dlsym(hMod, "_Z10LibVersionv");
		LibDescription = (charvoid)dlsym(hMod, "_Z14LibDescriptionv");
		LibNumberClasses = (intvoid)dlsym(hMod, "_Z16LibNumberClassesv");
		LibClassDesc = (cdint)dlsym(hMod, "_Z12LibClassDesci");
#endif
#endif
		printMessage("PluginMgr", "Checking Plugin Version...", WHITE);
		if(LibVersion() != VERSION_GEMRB) {
			printStatus("ERROR", LIGHT_RED);
			printf("Plug-in Version not valid, Skipping...\n");
			continue;
		}
		printStatus("OK", LIGHT_GREEN);
		printMessage("PluginMgr", "Loading Exports for ", WHITE);
		textcolor(LIGHT_WHITE);
		printf("%s", LibDescription());
		textcolor(WHITE);
		printf("...");
		int count = LibNumberClasses();
		printStatus("OK", LIGHT_GREEN);
		bool error = false;
		for(int i = 0; i < count; i++) {
			ClassDesc *plug = LibClassDesc(i);
			if(plug == NULL) {
				printStatus("ERROR", LIGHT_RED);
				printf("Plug-in Exports Error\n");			
				continue;
			}
			for(unsigned int x = 0; x < plugins.size(); x++) {
				if(plugins[x]->ClassID() == plug->ClassID()) {
					printStatus("SKIPPING", YELLOW);
					printf("Plug-in Already Loaded\n");
					error = true;
					break;
				}
				if(plugins[x]->SuperClassID() == plug->SuperClassID()) {
					printStatus("SKIPPING", YELLOW);
					printf("Duplicate Plug-in\n");
					error = true;
					break;
				}
			}
			if(error)
				break;
			plugins.push_back(plug);
		}
#ifdef WIN32  //Win32 while condition uses the _findnext function
	} while(_findnext(hFile, &c_file) == 0);
	_findclose(hFile);
#else  //Linux uses the readdir function
	} while((de = readdir(dir)) != NULL);
	closedir(dir);  //No other files in the directory, close it
#endif
}

PluginMgr::~PluginMgr(void)
{
}

bool PluginMgr::IsAvailable(SClass_ID plugintype)
{
	for(unsigned int i = 0; i < plugins.size(); i++) {
		if(plugins[i]->SuperClassID() == plugintype)
			return true;
	}
	return false;
}

void * PluginMgr::GetPlugin(SClass_ID plugintype)
{
  std::vector<ClassDesc*>::iterator plugs;
  for(plugs = plugins.begin(); plugs != plugins.end(); ++plugs) {
    if((*plugs)->SuperClassID() == plugintype)
      return (*plugs)->Create();
  }
  return NULL;
}

void PluginMgr::FreePlugin(void * ptr)
{
	((Plugin*)ptr)->release();
}
