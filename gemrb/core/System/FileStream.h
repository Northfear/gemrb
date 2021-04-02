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

/**
 * @file FileStream.h
 * Declares FileStream class, stream reading/writing data from/to a file in a filesystem.
 * @author The GemRB Project
 */


#ifndef FILESTREAM_H
#define FILESTREAM_H

#include "System/DataStream.h"

#include "exports.h"
#include "globals.h"

#ifdef VITA
#include <psp2/kernel/clib.h>
#include "../platforms/vita/libc_bridge.h"
#ifdef VITA_CACHE
#include "../platforms/vita/VitaCache.h"
#endif
#endif

namespace GemRB {

/**
 * @class FileStream
 * Reads and writes data from/to files on a filesystem
 */

#if defined(VITA) && !defined(VITA_CACHE)
struct File {
private:
	FILE* file = nullptr;
	bool opened = false;
public:
	File(FILE* f) : file(f) {}
	File() = default;
	File(const File&) = delete;
	File(File&& f) noexcept {
		file = f.file;
		f.file = nullptr;
	}
	~File() {
		if (file) sceLibcBridge_fclose(file);
	}
	
	File& operator=(const File&) = delete;
	File& operator=(File&& f) noexcept {
		if (&f != this) {
			std::swap(file, f.file);
		}
		return *this;
	}

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
#elif defined(VITA_CACHE)
struct File {
private:
	FILE *file = nullptr;
	bool cached = false;
	size_t cachedOffset = 0;
	CachedFile *cachedFile = nullptr;
public:
	File(FILE* f) : file(f) {}
	File() = default;
	File(const File&) = delete;
	File(File&& f) noexcept {
		file = f.file;
		f.file = nullptr;
	}
	~File() {
		if (cached) {
			VitaCache::ReleaseFile(cachedFile);
		}
		if (file) sceLibcBridge_fclose(file); 
	}
	
	File& operator=(const File&) = delete;
	File& operator=(File&& f) noexcept {
		if (&f != this) {
			std::swap(file, f.file);
		}
		return *this;
	}

	size_t Length() {
		if (cached) {
			size_t size =  cachedFile->fileSize;
			cachedOffset = 0;
			return size;
		}

		sceLibcBridge_fseek(file, 0, SEEK_END);
		size_t size = sceLibcBridge_ftell(file);
		sceLibcBridge_fseek(file, 0, SEEK_SET);
		return size;
	}
	bool OpenRO(const char *name) {
		file = sceLibcBridge_fopen(name, "rb");

		if (cached) {
			// Reopened? After writes? Better remove it from cache and read again..
			VitaCache::ReleaseFile(cachedFile);
			VitaCache::TryRemoveFile(cachedFile);
			cached = false;
		}

		if (file) {
			std::string stringName = std::string(name);
			size_t fileSize = Length();
			cachedFile = VitaCache::GetCachedFileByName(stringName);

			if (cachedFile) {
				if (cachedFile->fileSize != fileSize) {
					VitaCache::ReleaseFile(cachedFile);
					VitaCache::TryRemoveFile(cachedFile);
					cachedFile = nullptr;
				}
			}

			if (!cachedFile) {
				cachedFile = VitaCache::AddCachedFile(stringName, fileSize);
				if (cachedFile) {
					sceLibcBridge_fread(cachedFile->fileContent, 1, fileSize, file);
				}
			}

			if (cachedFile) {
				cached = true;
				cachedOffset = 0;
			}
		}

		return file;
	}
	bool OpenRW(const char *name) {
		if (VitaCache::IsCached(name)) {
			VitaCache::TryRemoveFile(name);
		}
		return (file = sceLibcBridge_fopen(name, "r+b"));
	}
	bool OpenNew(const char *name) {
		if (VitaCache::IsCached(name)) {
			VitaCache::TryRemoveFile(name);
		}
		return (file = sceLibcBridge_fopen(name, "wb"));
	}
	size_t Read(void* ptr, size_t length) {
		if (cached) {
			if (cachedOffset + length > cachedFile->fileSize)
				length = cachedFile->fileSize - cachedOffset;
			sceClibMemcpy(ptr, cachedFile->fileContent + cachedOffset, length);
			cachedOffset += length;
			cachedFile->UpdateAccessTime();
			return length;
		} else {
			return sceLibcBridge_fread(ptr, 1, length, file);
		}
	}
	size_t Write(const void* ptr, size_t length) {
		return sceLibcBridge_fwrite(ptr, 1, length, file);
	}
	bool SeekStart(int offset)
	{
		if (cached) {
			cachedOffset = offset;
			return true;
		} else {
			return !sceLibcBridge_fseek(file, offset, SEEK_SET);
		}
	}
	bool SeekCurrent(int offset)
	{
		if (cached) {
			cachedOffset += offset;
			return true;
		} else {
			return !sceLibcBridge_fseek(file, offset, SEEK_CUR);
		}
	}
	bool SeekEnd(int offset)
	{
		if (cached) {
			cachedOffset = cachedFile->fileSize + offset;
			return true;
		} else {
			return !sceLibcBridge_fseek(file, offset, SEEK_END);
		}
	}
};
#else
struct File {
private:
	FILE* file = nullptr;
public:
	File(FILE* f) : file(f) {}
	File() = default;
	File(const File&) = delete;
	File(File&& f) noexcept {
		file = f.file;
		f.file = nullptr;
	}
	~File() {
		if (file) fclose(file);
	}
	
	File& operator=(const File&) = delete;
	File& operator=(File&& f) noexcept {
		if (&f != this) {
			std::swap(file, f.file);
		}
		return *this;
	}

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

class GEM_EXPORT FileStream : public DataStream {
private:
	File str;
	bool opened, created;
public:
	FileStream(File&&);
	FileStream(void);

	DataStream* Clone();

	bool Open(const char* filename);
	bool Modify(const char* filename);
	bool Create(const char* folder, const char* filename, SClass_ID ClassID);
	bool Create(const char* filename, SClass_ID ClassID);
	bool Create(const char* filename);
	int Read(void* dest, unsigned int length);
	int Write(const void* src, unsigned int length);
	int Seek(int pos, int startpos);

	void Close();
public:
	/** Opens the specifed file.
	 *
	 *  Returns NULL, if the file can't be opened.
	 */
	static FileStream* OpenFile(const char* filename);
private:
	void FindLength();
};

}

#endif  // ! FILESTREAM_H
