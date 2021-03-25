#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <time.h>

#define VECTOR_CACHE 1

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

class VitaCache
{
public:
	static void Init();
	static CachedFile* AddCachedFile(std::string name, size_t size);
	static CachedFile* GetCachedFileByName(std::string name);
	static void ReleaseFile(CachedFile* file);
	static void TryRemoveFile(CachedFile* file);

private:
	static bool FreeFileCache(size_t targetSize);
	static bool IsInWhiteList(std::string filename);
	static bool IsInBlackList(std::string filename);
	static bool IsInUnloadList(std::string filename);
	static void AddCachedFileInternal(CachedFile *cachedFile);
	static void RemoveCachedFileInternal(CachedFile *file);
	static CachedFile* GetCachedFileInternal(std::string name);

	static bool initialized;
	static size_t maxCacheSize;
	static size_t maxFileSize;
	static size_t cacheSize;
#ifdef VECTOR_CACHE
	static std::vector<CachedFile*> cachedFiles;
#else
	static std::unordered_map<std::string, CachedFile*> cachedFiles;
#endif
	static std::vector<std::string> whitelist;
	static std::vector<std::string> unloadlist;
	static std::vector<std::string> blacklist;
};
