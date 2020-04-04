#include "disk.h"

#ifdef DEBUG_PRINT
#include <stdio.h>
extern void debug_print(const char* format, ...);
#else
#define debug_print(...)
#endif

void disk_init(struct disk *disk)
{
	debug_print("disk_init\n");
	memset(disk, 0, sizeof(struct disk));
	disk->handle = INVALID_HANDLE_VALUE;
}

bool disk_open(struct disk *disk, const char *name, const char *flags)
{
	HANDLE handle;
	DWORD access = 0;

	debug_print("disk_open | %s | %s\n", name, flags);

	if (disk->handle != INVALID_HANDLE_VALUE)
	{
		debug_print("disk already open\n");
		return false;
	}

#ifdef _WIN32
	access |= strchr(flags, 'r') ? GENERIC_READ : 0;
	access |= strchr(flags, 'w') ? GENERIC_WRITE : 0;
	handle = CreateFileA(name, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
#else
	handle = fopen(name, flags);
#endif
	if (handle == INVALID_HANDLE_VALUE)
	{
		debug_print("CreateFileA error %x\n", GetLastError());
		return false;
	}

	disk->handle = handle;

	return true;
}

bool disk_close(struct disk *disk)
{
	debug_print("disk_close\n");

	if (disk->handle == INVALID_HANDLE_VALUE)
	{
		debug_print("disk not open\n");
		return false;
	}

	if (!CloseHandle(disk->handle))
	{
		debug_print("CloseHandle error %x\n", GetLastError());
		return false;
	}

	disk->handle = INVALID_HANDLE_VALUE;

	return true;
}

bool disk_read_sector(struct disk *disk, uint64_t sector, uint8_t *data)
{
#ifdef _WIN32
	LARGE_INTEGER distanceToMove;
#endif
	DWORD numberOfBytesRead;
	bool result;

	debug_print("disk_read_sector | %llu\n", sector);

	if (disk->handle == INVALID_HANDLE_VALUE)
	{
		debug_print("disk not open\n");
		return false;
	}
#ifdef _WIN32
	distanceToMove.QuadPart = sector * SECTOR_SIZE;
	result = (SetFilePointer(disk->handle, distanceToMove.LowPart, &distanceToMove.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR);
#else
	result = fseek(disk->handle, sector * SECTOR_SIZE, SEEK_SET) != 0;
#endif
	if (result)
	{
		debug_print("SetFilePointer error %x\n", GetLastError());
		return false;
	}

#ifdef _WIN32
	result = ReadFile(disk->handle, data, SECTOR_SIZE, &numberOfBytesRead, NULL);
#else
	numberOfBytesRead = fread(data, SECTOR_SIZE, 1, disk->handle);
	result = (numberOfBytesRead == SECTOR_SIZE);
#endif
	if (!result)
	{
		debug_print("ReadFile error %x\n", GetLastError());
		return false;
	}

	return true;
}

bool disk_write_sector(struct disk *disk, uint64_t sector, const uint8_t *data)
{
#ifdef _WIN32
	LARGE_INTEGER distanceToMove;
#endif
	DWORD numberOfBytesWritten;
	bool result;

	debug_print("disk_write_sector | %llu\n", sector);

	if (disk->handle == INVALID_HANDLE_VALUE)
	{
		debug_print("disk not open\n");
		return false;
	}

#ifdef _WIN32
	distanceToMove.QuadPart = sector * SECTOR_SIZE;
	result = (SetFilePointer(disk->handle, distanceToMove.LowPart, &distanceToMove.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR);
#else
	result = fseek(disk->handle, sector * SECTOR_SIZE, SEEK_SET) != 0;
#endif
	if (result)
	{
		debug_print("SetFilePointer error %x\n", GetLastError());
		return false;
	}

#ifdef _WIN32
	result = WriteFile(disk->handle, data, SECTOR_SIZE, &numberOfBytesWritten, NULL);
#else
	numberOfBytesWritten = fwrite(data, SECTOR_SIZE, 1, disk->handle);
	result = (numberOfBytesWritten == SECTOR_SIZE);
#endif
	if (!result)
	{
		debug_print("WriteFile error %x\n", GetLastError());
		return false;
	}

	return true;
}
