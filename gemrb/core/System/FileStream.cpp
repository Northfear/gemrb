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

#include "System/FileStream.h"

#include "win32def.h"

#include "Interface.h"
#include "../platforms/vita/libc_bridge.h"

namespace GemRB {

#ifdef _DEBUG
int FileStream::FileStreamPtrCount = 0;
#endif

#ifdef WIN32

#define TCHAR_NAME(name) \
	TCHAR t_name[MAX_PATH] = {0}; \
	mbstowcs(t_name, name, MAX_PATH - 1);

struct FileStream::File {
private:
	HANDLE file;
public:
	File() : file() {}
	void Close() { CloseHandle(file); }
	size_t Length() {
		LARGE_INTEGER size;
		DWORD high;
		DWORD low = GetFileSize(file, &high);
		if (low != 0xFFFFFFFF || GetLastError() == NO_ERROR) {
			size.LowPart = low;
			size.HighPart = high;
			return size.QuadPart;
		}
		return 0;
	}
	bool OpenRO(const char *name) {
		TCHAR_NAME(name)
		file = CreateFile(t_name,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		return (file != INVALID_HANDLE_VALUE);
	}
	bool OpenRW(const char *name) {
		TCHAR_NAME(name)
		file = CreateFile(t_name,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		return (file != INVALID_HANDLE_VALUE);
	}
	bool OpenNew(const char *name) {
		TCHAR_NAME(name)
		file = CreateFile(t_name,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
		return (file != INVALID_HANDLE_VALUE);
	}
	size_t Read(void* ptr, size_t length) {
		unsigned long read;
		if (!ReadFile(file, ptr, length, &read, NULL))
			return 0;
		return read;
	}
	size_t Write(const void* ptr, size_t length) {
		unsigned long wrote;
		if (!WriteFile(file, ptr, length, &wrote, NULL))
			return 0;
		return wrote;
	}
	bool SeekStart(int offset)
	{
		return SetFilePointer(file, offset, NULL, FILE_BEGIN) != 0xffffffff;
	}
	bool SeekCurrent(int offset)
	{
		return SetFilePointer(file, offset, NULL, FILE_CURRENT) != 0xffffffff;
	}
	bool SeekEnd(int offset)
	{
		return SetFilePointer(file, offset, NULL, FILE_END) != 0xffffffff;
	}
};
#elif defined (VITA_LIBC)
struct FileStream::File {
private:
	FILE* file;
public:
	File() : file(NULL) {}
	void Close() { sceLibcBridge_fclose(file); }
	size_t Length() {
		sceLibcBridge_fseek(file, 0, SEEK_END);
		size_t size = sceLibcBridge_ftell(file);
		sceLibcBridge_fseek(file, 0, SEEK_SET);
		return size;
	}
	bool OpenRO(const char *name) {
		return (file = sceLibcBridge_fopen(name, "rb"));
	}
	bool OpenRW(const char *name) {
		return (file = sceLibcBridge_fopen(name, "r+b"));
	}
	bool OpenNew(const char *name) {
		return (file = sceLibcBridge_fopen(name, "wb"));
	}
	size_t Read(void* ptr, size_t length) {
		return sceLibcBridge_fread(ptr, 1, length, file);
	}
	size_t Write(const void* ptr, size_t length) {
		return sceLibcBridge_fwrite(ptr, 1, length, file);
	}
	bool SeekStart(int offset)
	{
		return !sceLibcBridge_fseek(file, offset, SEEK_SET);
	}
	bool SeekCurrent(int offset)
	{
		return !sceLibcBridge_fseek(file, offset, SEEK_CUR);
	}
	bool SeekEnd(int offset)
	{
		return !sceLibcBridge_fseek(file, offset, SEEK_END);
	}
};
#elif defined (VITA_FIOS)
#include <psp2/fios2.h>
#include <psp2/kernel/clib.h> 

struct FileStream::File {
private:
	SceFiosFH file;

public:
	File() : file(0) {}
	void Close() {
		sceFiosFHCloseSync(NULL, file);
	}
	size_t Length() {
		sceFiosFHSeek(file, 0, SCE_FIOS_SEEK_END);
		size_t size = sceFiosFHTell(file);
		sceFiosFHSeek(file, 0, SCE_FIOS_SEEK_SET);
		return size;
	}
	bool OpenRO(const char *name) {
		int res;
		SceFiosOpenParams params = SCE_FIOS_OPENPARAMS_INITIALIZER;
		params.openFlags = SCE_FIOS_O_RDONLY;
		res = sceFiosFHOpenSync(NULL, &file, name, &params);
		if (res != SCE_FIOS_OK) {
			return false;
		}
		return true;
	}
	bool OpenRW(const char *name) {
		int res;
		SceFiosOpenParams params = SCE_FIOS_OPENPARAMS_INITIALIZER;
		params.openFlags = SCE_FIOS_O_RDWR;
		res = sceFiosFHOpenSync(NULL, &file, name, &params);
		if (res != SCE_FIOS_OK) {
			return false;
  		}
		return true;
	}
	bool OpenNew(const char *name) {
		int res;
		SceFiosOpenParams params = SCE_FIOS_OPENPARAMS_INITIALIZER;
		params.openFlags = SCE_FIOS_O_WRONLY | SCE_FIOS_O_CREAT | SCE_FIOS_O_TRUNC;
		res = sceFiosFHOpenSync(NULL, &file, name, &params);
		if (res != SCE_FIOS_OK) {
			return false;
  		}
		return true;
	}
	size_t Read(void* ptr, size_t length) {
		SceFiosOpAttr attr = SCE_FIOS_OPATTR_INITIALIZER;
		attr.deadline = SCE_FIOS_TIME_EARLIEST;
		attr.priority = SCE_FIOS_PRIO_MAX;
		//attr.opflags = SCE_FIOS_OPFLAG_NOCACHE;
		return sceFiosFHReadSync(&attr, file, ptr, (SceFiosSize)length);
	}
	size_t Write(const void* ptr, size_t length) {
		return sceFiosFHWriteSync(NULL, file, ptr, (SceFiosSize)length);
	}
	bool SeekStart(int offset)
	{
		return !sceFiosFHSeek(file, static_cast<SceFiosOffset>(offset), SCE_FIOS_SEEK_SET);
	}
	bool SeekCurrent(int offset)
	{
		return !sceFiosFHSeek(file, static_cast<SceFiosOffset>(offset), SCE_FIOS_SEEK_CUR);
	}
	bool SeekEnd(int offset)
	{
		return !sceFiosFHSeek(file, static_cast<SceFiosOffset>(offset), SCE_FIOS_SEEK_END);
	}
};
#elif defined (VITA_CACHED)
#include <psp2/kernel/clib.h> 

#define FILE_CACHE_SIZE 128 * 1024 * 1024
#define MAX_CACHED_FILE_SIZE 16 * 1024 * 1024

struct CachedFile
{
	char fileName[80];
	size_t fileSize;
	char *fileContent;
	int inUse = 0;
};

std::vector<CachedFile*> cachedFiles;

void FreeFileCache()
{
	for (uint32_t i = 0; i < cachedFiles.size(); i++)
	{
		if (cachedFiles[i]->inUse <= 0)
		{
			free(cachedFiles[i]->fileContent);
			free(cachedFiles[i]);
			break;
		}
	}
}

size_t GetFileCacheSize()
{
	size_t totalSize = 0;
	for (uint32_t i = 0; i < cachedFiles.size(); i++) {
		totalSize += cachedFiles[i]->fileSize;
	}
	return totalSize;
}

CachedFile* GetCachedFileByName(const char *name)
{
	for (uint32_t i = 0; i < cachedFiles.size(); i++)
	{
		if (strcmp(cachedFiles[i]->fileName, name) == 0)
		{
			return cachedFiles[i];
		}
	}
	return nullptr;
}

void AddCachedFile(const char *name, char *content, size_t size)
{
	CachedFile* cachedFile = new CachedFile();
	strcpy(cachedFile->fileName, name);
	cachedFile->fileContent = content;
	cachedFile->fileSize = size;
	cachedFiles.push_back(cachedFile);
}

struct FileStream::File {
private:
	FILE *file;
	bool cached = false;
	char *fileContent;
	size_t cachedOffset = 0;
	size_t fileSize = 0;
	CachedFile* cachedFile;
public:
	File() : file(NULL) {}
	void Close() { 
		if (cached) {
			//sceClibPrintf("FILE FREED!\n");
			cachedFile->inUse--;
		}
		fclose(file); 
	}
	size_t Length() {
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		return size;
	}
	bool OpenRO(const char *name) {
		file = fopen(name, "rb");
		fileSize = Length();
		if (fileSize <= MAX_CACHED_FILE_SIZE)
		{
			cachedFile = GetCachedFileByName(name);
			if (cachedFile) {
				fileContent = cachedFile->fileContent;
				//sceClibPrintf("FILE FOUND IN CACHE! %s\n", name);
			} else {
				fileContent = (char*)malloc(fileSize + 1);
				fread(fileContent, 1, fileSize, file);
				AddCachedFile(name, fileContent, fileSize);
				cachedFile = GetCachedFileByName(name);
				//sceClibPrintf("FILE ADDED TO CACHE! %s\n", name);
			}

			cachedFile->inUse++;
			cached = true;
			cachedOffset = 0;
			//sceClibPrintf("FILE CACHED: %s SIZE: %zu\n", name, fileSize);
			//sceClibPrintf("TOTAL CACHE SIZE: %zu\n", GetFileCacheSize());
		}
		return file;
	}
	bool OpenRW(const char *name) {
		return (file = fopen(name, "r+b"));
	}
	bool OpenNew(const char *name) {
		return (file = fopen(name, "wb"));
	}
	size_t Read(void* ptr, size_t length) {
		if (cached) {
			if (length + cachedOffset > fileSize)
				length = fileSize - cachedOffset;
			memcpy( ptr, fileContent + cachedOffset, length );
			cachedOffset += length;
			return length;
		} else {
			return fread(ptr, 1, length, file);
		}
	}
	size_t Write(const void* ptr, size_t length) {
		return fwrite(ptr, 1, length, file);
	}
	bool SeekStart(int offset)
	{
		if (cached) {
			cachedOffset = offset;
			return true;
		}
		else {
			return !fseek(file, offset, SEEK_SET);
		}
	}
	bool SeekCurrent(int offset)
	{
		if (cached) {
			cachedOffset += offset;
			if (cachedOffset > fileSize)
				cachedOffset = fileSize;
			return true;
		}
		else {
			return !fseek(file, offset, SEEK_CUR);
		}
	}
	bool SeekEnd(int offset)
	{
		if (cached) {
			cachedOffset = fileSize + offset;
			if (cachedOffset > fileSize)
				cachedOffset = fileSize;
			return true;
		}
		else {
			return !fseek(file, offset, SEEK_END);
		}
	}
};
#else
struct FileStream::File {
private:
	FILE* file;
public:
	File() : file(NULL) {}
	void Close() { fclose(file); }
	size_t Length() {
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		return size;
	}
	bool OpenRO(const char *name) {
		return (file = fopen(name, "rb"));
	}
	bool OpenRW(const char *name) {
		return (file = fopen(name, "r+b"));
	}
	bool OpenNew(const char *name) {
		return (file = fopen(name, "wb"));
	}
	size_t Read(void* ptr, size_t length) {
		return fread(ptr, 1, length, file);
	}
	size_t Write(const void* ptr, size_t length) {
		return fwrite(ptr, 1, length, file);
	}
	bool SeekStart(int offset)
	{
		return !fseek(file, offset, SEEK_SET);
	}
	bool SeekCurrent(int offset)
	{
		return !fseek(file, offset, SEEK_CUR);
	}
	bool SeekEnd(int offset)
	{
		return !fseek(file, offset, SEEK_END);
	}
};
#endif

FileStream::FileStream(void)
{
	opened = false;
	created = false;
	str = new File();
}

DataStream* FileStream::Clone()
{
	return OpenFile(originalfile);
}

FileStream::~FileStream(void)
{
	Close();
	delete str;
}

void FileStream::Close()
{
	if (opened) {
#ifdef _DEBUG
		FileStreamPtrCount--;
#endif
		str->Close();
	}
	opened = false;
	created = false;
}

void FileStream::FindLength()
{
	size = str->Length();
	Pos = 0;
}

bool FileStream::Open(const char* fname)
{
	Close();

	if (!file_exists(fname)) {
		return false;
	}

	if (!str->OpenRO(fname)) {
		return false;
	}
#ifdef _DEBUG
	FileStreamPtrCount++;
#endif
	opened = true;
	created = false;
	FindLength();
	ExtractFileFromPath( filename, fname );
	strlcpy( originalfile, fname, _MAX_PATH);
	return true;
}

bool FileStream::Modify(const char* fname)
{
	Close();

	if (!str->OpenRW(fname)) {
		return false;
	}
#ifdef _DEBUG
	FileStreamPtrCount++;
#endif
	opened = true;
	created = true;
	FindLength();
	ExtractFileFromPath( filename, fname );
	strlcpy( originalfile, fname, _MAX_PATH);
	Pos = 0;
	return true;
}

//Creating file in the cache
bool FileStream::Create(const char* fname, SClass_ID ClassID)
{
	return Create(core->CachePath, fname, ClassID);
}

bool FileStream::Create(const char *folder, const char* fname, SClass_ID ClassID)
{
	char path[_MAX_PATH];
	char filename[_MAX_PATH];
	ExtractFileFromPath( filename, fname );
	PathJoinExt(path, folder, filename, core->TypeExt(ClassID));
	return Create(path);
}

//Creating file outside of the cache
bool FileStream::Create(const char *path)
{
	Close();

	ExtractFileFromPath( filename, path );
	strlcpy(originalfile, path, _MAX_PATH);

	if (!str->OpenNew(originalfile)) {
		return false;
	}
	opened = true;
	created = true;
	Pos = 0;
	size = 0;
	return true;
}

int FileStream::Read(void* dest, unsigned int length)
{
	if (!opened) {
		return GEM_ERROR;
	}
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos+length>size ) {
		return GEM_ERROR;
	}
	size_t c = str->Read(dest, length);
	if (c != length) {
		return GEM_ERROR;
	}
	if (Encrypted) {
		ReadDecrypted( dest, c );
	}
	Pos += c;
	return c;
}

int FileStream::Write(const void* src, unsigned int length)
{
	if (!created) {
		return GEM_ERROR;
	}
	// do encryption here if needed

	size_t c = str->Write(src, length);
	if (c != length) {
		return GEM_ERROR;
	}
	Pos += c;
	if (Pos>size) {
		size = Pos;
	}
	return c;
}

int FileStream::Seek(int newpos, int type)
{
	if (!opened && !created) {
		return GEM_ERROR;
	}
	switch (type) {
		case GEM_STREAM_END:
			str->SeekStart(size - newpos);
			Pos = size - newpos;
			break;
		case GEM_CURRENT_POS:
			str->SeekCurrent(newpos);
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			str->SeekStart(newpos);
			Pos = newpos;
			break;

		default:
			return GEM_ERROR;
	}
	if (Pos>size) {
		print("[Streams]: Invalid seek position %ld in file %s(limit: %ld)", Pos, filename, size);
		return GEM_ERROR;
	}
	return GEM_OK;
}

FileStream* FileStream::OpenFile(const char* filename)
{
	FileStream *fs = new FileStream();
	if (fs->Open(filename))
		return fs;

	delete fs;
	return NULL;
}

}
