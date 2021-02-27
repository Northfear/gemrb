#include "FileCache.h"

const std::string EVERYTHING = "*";

// load files ignoring file size restriction and forbid from unloading them from cache
std::vector<std::string> FileCache::whitelist = { "CHAAnim.bif", "OBJAnim.bif", "Cache2" };
// files from whitelist that can be unloaded
std::vector<std::string> FileCache::unloadlist = { "Cache2" };
// never load into cache
std::vector<std::string> FileCache::blacklist = { };

std::unordered_map<std::string, CachedFile*> FileCache::cachedFiles;
std::vector<CachedFile*> FileCache::garbageList;
size_t FileCache::cacheSize = 0;


bool FileCache::IsInWhiteList(std::string filename)
{
	for (uint32_t i = 0; i < whitelist.size(); ++i) {
		if (filename.find(whitelist[i]) != std::string::npos) {
			return true;
		}
	}
	return false;
}

bool FileCache::IsInBlackList(std::string filename)
{
	for (uint32_t i = 0; i < blacklist.size(); ++i) {
		if (blacklist[i] == EVERYTHING || filename.find(blacklist[i]) != std::string::npos) {
			return true;
		}
	}
	return false;
}

bool FileCache::IsInUnloadList(std::string filename)
{
	for (uint32_t i = 0; i < unloadlist.size(); ++i) {
		if (unloadlist[i] == EVERYTHING || filename.find(unloadlist[i]) != std::string::npos) {
			return true;
		}
	}
	return false;
}

bool FileCache::FreeFileCache(size_t targetSize)
{
	// removes files with the earliest acces time
	while(cacheSize > targetSize && !garbageList.empty()) {
		time_t minAccessTime = garbageList[0]->GetAccessTime();
		int minIndex = 0;

		for (uint32_t i = 1; i < garbageList.size(); ++i) {
			if (garbageList[i]->GetAccessTime() < minAccessTime) {
				minAccessTime = garbageList[i]->GetAccessTime();
				minIndex = i;
			}
		}

		CachedFile* lowestPriorityFile = garbageList[minIndex];
		cacheSize -= lowestPriorityFile->fileSize;
		cachedFiles.erase(lowestPriorityFile->fileName);
		garbageList.erase(garbageList.begin() + minIndex);
		delete lowestPriorityFile;
	}

	return cacheSize < targetSize;
}

size_t FileCache::GetFileCacheSize()
{
	return cacheSize;
}

CachedFile* FileCache::GetCachedFileByName(std::string name)
{
	CachedFile* file = cachedFiles[name];

	if (file) {
		// remove from garbage list if it's there, since it's used again
		if (!file->InUse()) {
			for (uint32_t i = 0; i < garbageList.size(); ++i) {
				if (garbageList[i] == file) {
					garbageList.erase(garbageList.begin() + i);
					break;
				}
			}
		}
		file->AccessFile();
	}

	return file;
}

void FileCache::ReleaseFile(CachedFile* file)
{
	file->ReleaseFile();
	// place into garbage list if it's not used by anyone
	if (!file->InUse()) {
		garbageList.emplace_back(file);
	}
}

void FileCache::TryRemoveFile(CachedFile* file)
{
	// file might not be removed if it's still used by someone. could be a problem if it's not updated after some write (no idea if this could happen tho)
	ReleaseFile(file);
	if (file->InUse()) {
		return;
	}

	for (uint32_t i = 0; i < garbageList.size(); ++i) {
		if (garbageList[i] == file) {
			cacheSize -= file->fileSize;
			cachedFiles.erase(file->fileName);
			garbageList.erase(garbageList.begin() + i);
			delete file;
			break;
		}
	}
}

CachedFile* FileCache::AddCachedFile(std::string name, size_t size)
{
	if (size > FILE_CACHE_SIZE) {
		return nullptr;
	}

	bool allowUnload = true;

	// whitelist ignores max file size
	if (IsInWhiteList(name)) {
		allowUnload = IsInUnloadList(name);
	} else if (size > MAX_CACHED_FILE_SIZE || IsInBlackList(name)) {
		return nullptr;
	}

	if (cacheSize + size > FILE_CACHE_SIZE) {
		if (!FreeFileCache(FILE_CACHE_SIZE - size)) {
			return nullptr;
		}
	}

	CachedFile *cachedFile = new CachedFile(name, size, allowUnload);
	cachedFile->AccessFile();
	cachedFiles[name] = cachedFile;
	cacheSize += size;

	return cachedFile;
}
