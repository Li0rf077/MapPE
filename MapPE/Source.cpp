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
	DWORD64 importdirRVA = (DWORD64)nt_header + opt_header.DataDirectory[1].VirtualAddress;

	// First, find .rdata section from section headers
	DWORD64 sectionsize = (DWORD64)sizeof(IMAGE_SECTION_HEADER);

	// Find first section
	PIMAGE_SECTION_HEADER sectionheader = IMAGE_FIRST_SECTION(nt_header);

	// Save value of section address
	PIMAGE_SECTION_HEADER sectionaddress = 0;
	DWORD64 sectionVA = 0;

	for (int i = 0; i < file_header.NumberOfSections; i++, ++sectionheader)
	{
		// calaculate addresses of section addresses + addresses when they are finished
		DWORD64 va = (DWORD64)sectionheader->VirtualAddress + (DWORD64)nt_header + (DWORD64)file_header.SizeOfOptionalHeader + (DWORD64)sizeof(IMAGE_FILE_HEADER) + 8;
		DWORD64 va_size = (DWORD64)sectionheader->VirtualAddress + (DWORD64)sectionheader->Misc.VirtualSize + (DWORD64)nt_header + (DWORD64)file_header.SizeOfOptionalHeader + (DWORD64)sizeof(IMAGE_FILE_HEADER) + 8;
		
		// Get section where import table is
		if (importdirRVA >= va && importdirRVA <= va_size)
		{
			printf_s("FOUND IMPORT SECTION! -> %s\n", sectionheader->Name);
			sectionaddress = sectionheader;
			sectionVA = va;
		}
		printf_s("%s\n", sectionheader->Name);
	}

	// calculate offset of import table
	// formula -> nt_header + section offset from EP(sectionheader->viraddress) + (import dir - section address)
	if (sectionaddress && sectionVA)
	{
		// get file offset to import table
		DWORD64 rawOffset = (DWORD64)pblock + sectionaddress->PointerToRawData;
		DWORD64 importVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

		// Calculate import table address
		PIMAGE_IMPORT_DESCRIPTOR import_table = (PIMAGE_IMPORT_DESCRIPTOR)(rawOffset + (importVA - sectionaddress->VirtualAddress));

		for (; import_table->Name != 0; import_table++) {

			// Print current dll name. calculation: import table + offset to name only.
			printf_s("%s\n", rawOffset + (import_table->Name - sectionaddress->VirtualAddress));

			DWORD fst = import_table->OriginalFirstThunk;

			PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)(rawOffset + (import_table->OriginalFirstThunk - sectionaddress->VirtualAddress));

			for (; thunk->u1.AddressOfData != 0; thunk++)
			{
				if (thunk->u1.AddressOfData > 0x80000000) {
					//show lower bits of the value to get the ordinal ¯\_(ツ)_/¯
					printf("\tOrdinal: %x\n", (WORD)thunk->u1.AddressOfData);
				}
				else {
					printf("\t%s\n", rawOffset + (thunk->u1.AddressOfData - sectionaddress->VirtualAddress + 2));
				}
			}	
		}
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
