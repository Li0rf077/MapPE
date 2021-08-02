# MapPE
mapping a PE and showing all its imports

great explenation from stackoverflow:
https://stackoverflow.com/questions/7673754/pe-format-iat-questions

To get to the Relative Virtual Address (address at runtime, aka RVA) of the IAT:

Start with the base address of the binary. That is a IMAGE_DOS_HEADER structure.
Follow the e_lfanew field to get to the IMAGE_NT_HEADERS structure. From offset 0, jump 0x3c down and dereference to get the start of the IMAGE_NT_HEADERS.
Follow the OptionalHeader (contained by, and therefore contiguous to, the IMAGE_NT_HEADERS). To get to the IMAGE_OPTIONAL_HEADER structure (despite its name, it's no longer optional), know that its the third struct within the IMAGE_NT_HEADER. To get to the OptionalHeader, add 0x18 to the value you dereferenced earlier
From the OptionalHeader, dereference into the DataDirectory. The DataDirectory is an array within the OptionalHeader, which is within the IMAGE_NT_HEADERS. Follow the 24th (if 0 is first, as in 0, 1, 2...) entry in the DataDirectory array to the IMAGE_DIRECTORY_ENTRY_IAT. Add 0xc0 to the address you are currently at to get the Import Address Table Directory
If you want to traverse a list of the DLL's and their function addresses, there is some background:

3 fields of importance in the IMAGE_IMPORT_DESCRIPTOR structure: OriginalFirstThunk (the RVA of the Import Lookup Table), Name (RVA of a null-terminated ASCII Dll name), and FirstThunk (RVA of IAT/array of linear addresses built by the loader).
Two arrays are needed because one is the array of names of imported routines (ILT), and the other is the array of imported routine addresses (IAT). Routines imported from a DLL can be imported by their name or their ordinal number. To tell if a routine is an ordinal import, check a flag set in the Ordinal field of the IMAGE_THUNK_DATA structure in the ILT array.
Each function imported by the module at load time will be represented by an IMAGE_THUNK_DATA structure. both the Original First Thunk and the First Thunk point to an array of IMAGE_THUNK_DATA. The IMAGE_THUNK_DATA struct however is a union, which contains another struct, IMAGE_IMPORT_BY_NAME. This is important to know because the Original First Thunk uses the IMAGE_IMPORT_BY_NAME struct, and the First Thunk uses the Function field within the IMAGE_THUNK_DATA struct's internal union.
RVA's specified on disk will not allow you to traverse the file because the RVA represents the address when the binary is loaded into memory; to traverse the binary on the disk you will need to convert the RVA values into a correct form. The formula is simple; HMODULE + RVA = linear address of PE element. HMODULE is also known as a base address. But getting the base address actually requires an algorithm that's somewhat lengthy, and is dependent on what the value of the RVA you have in question actually is. To obtain the value of the base address for a given RVA in order to calculate the linear address of the PE element on disk:

Obtain the sectionHeader; to do this, walk through the list of sections (such as .data, .text, ect) until you find a section where the RVA of question is within the currentSection.VirtualAddress and currentSection.VirtualAddress + currentSection.size.

1.1) First, find the number of sections within the FileHeader within the NT_HEADERS structure. It's the 2 bytes after the 2 byte Machine number within the FileHeader. *To do this manually: add 0x6 to the value dereferenced from the e_lfanew; so jump 0x3c from offset of 0, dereference the value, and 0x6 to that. Then read two bytes and interpret as an integer.

1.2) Find the location of the first section; it's contiguous to the OptionalHeader. Remember that within the OptionalHeader is the DataDirectories array. The OptionalHeader is 216 bytes long, plus the 2 words at the end that denote it's ending; so take 224 in hex (0xe0) and add it to the value dereferenced at 0x3c from the start to get the first section location.

1.3) To find the section header that your RVA resides within, continually perform this test with respect to the current section you are at. If the test fails, move to the next section. If you iterate through all sections and find that you reach the ending NULL words, then the file should be corrupt or you have made a mistake. The test is as follows: compare the RVA that you want to convert to a usable pointer to the section's virtual Address; the RVA should be >= to the section's virtual address, and < the summ of the section's virtual address and virtual size. The section's virtual address can be found by adding 12 to the address of the section. And the section's virtual size can be found by 8 to the section's address. To summarize: pass if - (section.virtualAddress+section.virtualSize) > RVA >= section.virtualAddress. *To iterate to the next section, the length of a section description is 0x28; you can just add 0x28 to the current section pointer to get to the next one. The last section is a Null byte to signify the end.

From the section header obtained, perform the following: (baseAddress+RVA) - (sectionHeader.virtualAddress - sectionhHeader.PointerToRawData). *The sectionHeader's virtualAddress is 12 bytes away from the sectionHeader itself as calculated above. The PointerToRawData is 20 away from the section header.

The value obtained represents the actual pointer to the data desired/represented by the RVA. You can use it to find the actual, in file location of the data you want.
That was a mouthful. If you want to recap, you should read pages 257-60 of Chapter 5 (Hooking Call Tables) in The Rootkit Arsenal, but for a more easy to understand graphical, check out the openrce.org pdf link I gave near the top.

To do it, however start by...:

Get to the OptionalHeader as described above. The OptionalHeader contains an array of (DataDirectory) IMAGE_DATA_DIRECTORY elements as it's last element. The second element within that array is the IMAGE_DIRECTORY_ENTRY_IMPORT, which locates the IAT. So, to clarify, the IMAGE_NT_HEADER contains the OptionalHeader array, which contains the DataDirectory array. The last entry in this array will be zeroed out.
From the OptionalHeader dereference into the IMAGE_DIRECTORY_ENTRY_IMPORT. The next word over is the size of the Import Directory. From the offset in the file, jump 0x68 down.
This value is the RVA of the Import Directory, which is an array of structures of type IMAGE_IMPORT_DESCRIPTOR (one for each DLL imported by the module), the last of which has its fields set to zero. The third word within the IMAGE_IMPORT_DESCRIPTOR contains a FirstThunk pointer to an IMAGE_THUNK_DATA.
Using the algorithm described above, convert the RVA of the Import Directory to a usable pointer, and use the following algorithm to iterate through the import directory array.

4.1) For the importDescriptor, convert the name field RVA to a pointer to obtain the name. It may be null

4.2) To obtain the name and address of each routine imported, obtain both the OriginalFirstThunk and the FirstThunk RVA entries of the import descriptor. Each of the OFT and FT's may be null, which would indicate that its empty, so check that.

4.3) Convert the OFT RVA's into pointers; the OFT corresponds to the ILT and the FT corresponds to the IAT. Either the ILT and the IAT may be null, indicating they are empty.

4.4) Obtain the name of the function imported from the ILT pointer, and the address of the function from the IAT pointer. To move to the next imported function, remember that the ILT and the IAT are arrays; the next element is a constant distance away.

4.5) Check that the new ILT & IAT pointer values obtained are not zero; if they are not zero, repeat. If either is zero, you have hit the end of the list of functions imported for that dll; the import descriptor is also is a repeating array, so the offset to the next dll imported is constant. Essentially, you are iterating through the dll's and for each dll you are iterating through the functions imported in this manner. 19
