#include <Windows.h>
#include <winnt.h>
#include <stdio.h>
#include <processthreadsapi.h>

int main() {

	HANDLE hfile;
	CHAR filepath[35] = "C:\\Windows\\System32\\notepad.exe";
	LARGE_INTEGER size;
	HANDLE hHeap = GetProcessHeap();
	LPVOID pblock;
	LPDWORD nbytedread = NULL;

	// Get handle for file
	hfile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		// get file size
		if (GetFileSizeEx(hfile, &size))
		{
			// allocate Heap space for file
			pblock = HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, size.QuadPart);

			// read file to heap
			bool success = ReadFile(hfile, pblock, size.QuadPart, nbytedread, NULL);
			DWORD lasterror = GetLastError();

			if (success)
			{
				PIMAGE_DOS_HEADER dos_h;
				dos_h = (PIMAGE_DOS_HEADER)pblock;

				if (dos_h)
				{
					printf_s("e_magic -> %c", dos_h->e_magic);
				}
			}
		}
		CloseHandle(hfile);
	}
}