#ifndef _TYPES_H_
#define _TYPES_H_

#ifndef _WIN32
#define HANDLE FILE*
#define DWORD size_t
#define INVALID_HANDLE_VALUE NULL
#define CloseHandle fclose
#endif

#endif
