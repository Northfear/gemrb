#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <time.h>

#define FILE_CACHE_SIZE 224 * 1024 * 1024
#define MAX_CACHED_FILE_SIZE 16 * 1024 * 1024

struct CachedFile
{
	std::string fileName;
	size_t fileSize = 0;
	char *fileContent;
	int usedBy = 0;
	time_t lastAccessTime = 0;
	bool allowUnload = true;

	CachedFile(std::string newName, size_t newfileFize, bool newAllowUnload)
	{
		fileName = newName;
		fileSize = newfileFize;
		allowUnload = newAllowUnload;
		fileContent = new char[fileSize];
	}

	~CachedFile()
	{
		delete[] fileContent;
	}

	bool InUse() const
	{
		return !allowUnload || usedBy > 0;
	}

	void UpdateAccessTime()
	{
		time(&lastAccessTime);
	}

	void AccessFile()
	{
		++usedBy;
		UpdateAccessTime();
	}

	void ReleaseFile()
	{
		--usedBy;
	}

	time_t GetAccessTime() const
	{
		return lastAccessTime;
	}
};

class FileCache
{
public:
	static CachedFile* AddCachedFile(std::string name, size_t size);
	static CachedFile* GetCachedFileByName(std::string name);
	static void ReleaseFile(CachedFile* file);
	static void TryRemoveFile(CachedFile* file);
	static size_t GetFileCacheSize();

private:
	static bool FreeFileCache(size_t targetSize = FILE_CACHE_SIZE);
	static bool IsInWhiteList(std::string filename);
	static bool IsInBlackList(std::string filename);
	static bool IsInUnloadList(std::string filename);

	static std::unordered_map<std::string, CachedFile*> cachedFiles;
	static std::vector<CachedFile*> garbageList;
	static size_t cacheSize;
	static std::vector<std::string> whitelist;
	static std::vector<std::string> unloadlist;
	static std::vector<std::string> blacklist;
};
