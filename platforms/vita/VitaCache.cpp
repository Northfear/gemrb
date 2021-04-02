#include "VitaCache.h"
#include "Interface.h"
#include <fstream>

const std::string EVERYTHING = "*";

// load files ignoring file size restriction and forbids from unloading them from cache
std::vector<std::string> VitaCache::whitelist = { };
// files from whitelist that can be unloaded
std::vector<std::string> VitaCache::unloadlist = { };
// never load into cache. Ignore Cache2, since files with the same name (and even size) may be generated there on different savegames
std::vector<std::string> VitaCache::blacklist = { ".wav", ".WAV" };

bool VitaCache::initialized = false;
size_t VitaCache::maxCacheSize = 192 * 1024 * 1024;
size_t VitaCache::maxFileSize = 8 * 1024 * 1024;
size_t VitaCache::cacheSize = 0;
#ifdef VECTOR_CACHE
std::vector<CachedFile*> VitaCache::cachedFiles;
#else
std::unordered_map<std::string, CachedFile*> VitaCache::cachedFiles;
#endif

using namespace GemRB;


void SeparateAndAddToList(std::string items, std::vector<std::string> &list, bool clearList = false)
{
	if (clearList) {
		list.clear();
	}

	std::string delimiter = ",";
	size_t pos = 0;
	std::string listItem;

	while ((pos = items.find(delimiter)) != std::string::npos) {
		listItem = items.substr(0, pos);
		list.emplace_back(listItem);
		items.erase(0, pos + delimiter.length());
	}
	
	if (items.length() > 0) {
		list.emplace_back(items);
	}
}

void VitaCache::AddCachedFileInternal(CachedFile *cachedFile)
{
	cachedFile->AccessFile();
	cacheSize += cachedFile->fileSize;
#ifdef VECTOR_CACHE
	cachedFiles.emplace_back(cachedFile);
#else
	cachedFiles[cachedFile->fileName] = cachedFile;
#endif
}

void VitaCache::RemoveCachedFileInternal(CachedFile *file)
{
#ifdef VECTOR_CACHE
	for (uint32_t i = 0; i < cachedFiles.size(); ++i) {
		if (cachedFiles[i] == file) {
			cacheSize -= file->fileSize;
			cachedFiles.erase(cachedFiles.begin() + i);
			delete file;
			break;
		}
	}
#else
	cacheSize -= file->fileSize;
	cachedFiles.erase(file->fileName);
	delete file;
#endif
}

CachedFile* VitaCache::GetCachedFileInternal(std::string name)
{
#ifdef VECTOR_CACHE
	for (uint32_t i = 0; i < cachedFiles.size(); ++i) {
		if (cachedFiles[i]->fileName == name) {
			return cachedFiles[i];
		}
	}
	return nullptr;
#else
	if (cachedFiles.find(name) == cachedFiles.end()) {
		return nullptr;
	}
	return cachedFiles[name];
#endif
}

void VitaCache::Init()
{
	if (core->VitaFileCache) {
		char cacheConfigPath[35];
		sprintf(cacheConfigPath, "ux0:data/GemRB/%s_cache.cfg", core->GameType);
		std::ifstream file(cacheConfigPath);

		if (file.is_open()) {
			std::string line;
			while (std::getline(file, line)) {
				std::string option = line.substr(0, line.find("="));
				line.erase(0, line.find("=") + 1);

				if (option == "MaxCacheSize") {
					maxCacheSize = atoi(line.c_str()) * 1024 * 1024;
				} else if (option == "MaxFileSize") {
					maxFileSize = atoi(line.c_str()) * 1024 * 1024;
				} else if (option == "WhiteList") {
					SeparateAndAddToList(line, whitelist);
				} else if (option == "UnloadList") {
					SeparateAndAddToList(line, unloadlist);
				} else if (option == "BlackList") {
					SeparateAndAddToList(line, blacklist);
				}
			}
			file.close();
			initialized = true;
		}

		//cachedFiles.reserve(400);
	}
}

bool VitaCache::IsInWhiteList(std::string filename)
{
	for (uint32_t i = 0; i < whitelist.size(); ++i) {
		if (filename.find(whitelist[i]) != std::string::npos) {
			return true;
		}
	}
	return false;
}

bool VitaCache::IsInBlackList(std::string filename)
{
	for (uint32_t i = 0; i < blacklist.size(); ++i) {
		if (blacklist[i] == EVERYTHING || filename.find(blacklist[i]) != std::string::npos) {
			return true;
		}
	}
	return false;
}

bool VitaCache::IsInUnloadList(std::string filename)
{
	for (uint32_t i = 0; i < unloadlist.size(); ++i) {
		if (unloadlist[i] == EVERYTHING || filename.find(unloadlist[i]) != std::string::npos) {
			return true;
		}
	}
	return false;
}

bool VitaCache::FreeFileCache(size_t targetSize)
{
#ifdef VECTOR_CACHE
	while(cacheSize > targetSize) {
		time_t minAccessTime;
		int minIndex;
		bool foundFileToRemove = false;

		for (uint32_t i = 0; i < cachedFiles.size(); ++i) {
			if (!cachedFiles[i]->InUse()) {
				minAccessTime = cachedFiles[i]->GetAccessTime();
				minIndex = i;
				foundFileToRemove = true;
				break;
			}
		}

		if (!foundFileToRemove)
			break;

		for (uint32_t i = minIndex + 1; i < cachedFiles.size(); ++i) {
			if (!cachedFiles[i]->InUse() && cachedFiles[i]->GetAccessTime() < minAccessTime) {
				minAccessTime = cachedFiles[i]->GetAccessTime();
				minIndex = i;
			}
		}

		CachedFile* lowestPriorityFile = cachedFiles[minIndex];
		cacheSize -= lowestPriorityFile->fileSize;
		cachedFiles.erase(cachedFiles.begin() + minIndex);
		delete lowestPriorityFile;
	}
#else
	while(cacheSize > targetSize) {
		time_t minAccessTime;
		auto minIt = cachedFiles.begin();
		bool foundFileToRemove = false;

		for (auto it = cachedFiles.begin(); it != cachedFiles.end(); ++it) {
			if (!it->second->InUse()) {
				minAccessTime = it->second->GetAccessTime();
				minIt = it;
				foundFileToRemove = true;
				break;
			}
		}

		if (!foundFileToRemove)
			break;

		for (auto it = minIt; it != cachedFiles.end(); ++it) {
			if (!it->second->InUse() && it->second->GetAccessTime() < minAccessTime) {
				minAccessTime = it->second->GetAccessTime();
				minIt = it;
			}
		}

		CachedFile* lowestPriorityFile = minIt->second;
		cacheSize -= lowestPriorityFile->fileSize;
		cachedFiles.erase(minIt);
		delete lowestPriorityFile;
	}
#endif

	return cacheSize < targetSize;
}

CachedFile* VitaCache::GetCachedFileByName(std::string name)
{
	if (!initialized) {
		return nullptr;
	}

	CachedFile* file = GetCachedFileInternal(name);
	if (file) {
		file->AccessFile();
	}

	return file;
}

void VitaCache::ReleaseFile(CachedFile* file)
{
	file->ReleaseFile();
	if (file->flaggedForRemoval && !file->InUse()) {
		RemoveCachedFileInternal(file);
	}
}

void VitaCache::TryRemoveFile(CachedFile* file)
{
	if (file->InUse()) {
		file->flaggedForRemoval = true;
		return;
	}
	RemoveCachedFileInternal(file);
}

void VitaCache::TryRemoveFile(const char *name)
{
	std::string stringName = std::string(name);
	CachedFile *cachedFile = GetCachedFileInternal(stringName);
	if (cachedFile != nullptr) {
		TryRemoveFile(cachedFile);
	}
}

bool VitaCache::IsCached(const char *name)
{
	if (initialized) {
		return false;
	}
	std::string stringName = std::string(name);
	return GetCachedFileInternal(stringName) != nullptr;
}

CachedFile* VitaCache::AddCachedFile(std::string name, size_t size)
{
	if (!initialized || size > maxCacheSize) {
		return nullptr;
	}

	bool allowUnload = true;

	// whitelist ignores max file size
	if (IsInWhiteList(name)) {
		allowUnload = IsInUnloadList(name);
	} else if (size > maxFileSize || IsInBlackList(name)) {
		return nullptr;
	}

	if (cacheSize + size > maxCacheSize) {
		if (!FreeFileCache(maxCacheSize - size)) {
			return nullptr;
		}
	}

	CachedFile *cachedFile = new CachedFile(name, size, allowUnload);
	AddCachedFileInternal(cachedFile);

	return cachedFile;
}
