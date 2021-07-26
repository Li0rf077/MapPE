#include <Windows.h>
#include <winnt.h>
#include <stdio.h>
#include <processthreadsapi.h>
#include <tchar.h>
#include <string>
#include <iostream>

int print_fields(PIMAGE_DOS_HEADER dos_h, PIMAGE_DOS_HEADER pblock)
{
	// Get pointer to NT HEADER
	PIMAGE_NT_HEADERS nt_header = (PIMAGE_NT_HEADERS)(dos_h->e_lfanew + (DWORDLONG)pblock);
	printf_s("nt header -> %p\n", nt_header);
	DWORD64 pblockk = (DWORD64)pblock;
	DWORD64 elfanew = dos_h->e_lfanew;
	DWORD64 address = (dos_h->e_lfanew + (DWORDLONG)pblock);

	// Get FILE HEADER
	IMAGE_FILE_HEADER file_header = nt_header->FileHeader;
	printf_s("file header - number of sections -> %x\n", file_header.NumberOfSections);
	printf_s("file header - size of optional header -> %x\n", file_header.SizeOfOptionalHeader);

	// GET OPTIONAL HEADER
	IMAGE_OPTIONAL_HEADER opt_header = nt_header->OptionalHeader;
	printf_s("optional header - address of entry point -> %x\n", opt_header.AddressOfEntryPoint);
	printf_s("optional header - Checksum -> %x\n", opt_header.CheckSum);
	printf_s("optional header - image base -> %x\n", opt_header.SizeOfImage);

	// Get ImportDirectory struct, which contains all imports
	IMAGE_DATA_DIRECTORY datadir_imports = opt_header.DataDirectory[1];
	printf_s("data dir 1 - virtual address -> %x\n", datadir_imports.VirtualAddress);

	// get import directory RVA
	DWORD64 importdirRVA = (DWORD64)pblock + opt_header.DataDirectory[1].VirtualAddress;

	// First, find .rdata section from section headers
	//DWORD64 sectionslocation = (DWORD64)(nt_header + (sizeof(DWORD64) + (DWORD64)(sizeof(IMAGE_FILE_HEADER)) + (DWORD64)nt_header->FileHeader.SizeOfOptionalHeader));
	DWORD64 sectionsize = (DWORD64)sizeof(IMAGE_SECTION_HEADER);

	DWORD64 size = sizeof(DWORD64);
	printf_s("nt header -> %p\n", nt_header);
	DWORD64 sizefileh = (DWORD64)(sizeof(IMAGE_FILE_HEADER));
	DWORD64 sizeopth = (DWORD64)nt_header->FileHeader.SizeOfOptionalHeader;
	PIMAGE_SECTION_HEADER location = (PIMAGE_SECTION_HEADER)(dos_h->e_lfanew + (DWORDLONG)pblock + size + sizefileh + sizeopth);

	for (int i = 0; i < file_header.NumberOfSections; i++)
	{
		PIMAGE_SECTION_HEADER section = (PIMAGE_SECTION_HEADER)(location);
		printf_s("section NAME -> %s, section size -> %ld\n", section->Name, section->SizeOfRawData);

		location += sectionsize;
	}




	return 1;
}

int main() {

	HANDLE hfile;
	CHAR filepath[35] = "C:\\Windows\\System32\\notepad.exe";
	LARGE_INTEGER * size = NULL;
	LPVOID pblock;
	DWORD nbytedread = NULL;

	// Get handle for file
	hfile = CreateFileA(filepath, GENERIC_READ, 0 , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD lasterror = GetLastError();
	if (hfile != INVALID_HANDLE_VALUE)
	{
		// get file size
		DWORD size = GetFileSize(hfile, NULL);
		if (size)
		{
			// allocate Heap space for file
			pblock = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size);

			// read file to heap
			bool success = ReadFile(hfile, pblock, size, &nbytedread, NULL);

			if (success)
			{
				PIMAGE_DOS_HEADER dos_h;
				dos_h = (PIMAGE_DOS_HEADER)pblock;

				if (dos_h)
				{
					print_fields(dos_h, (PIMAGE_DOS_HEADER)pblock);			
				}
			}
		}
		CloseHandle(hfile);
	}
}