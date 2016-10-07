#include <Windows.h>
#include <string>

//Store found addresses.
PVOID invoke_address = nullptr;
PVOID fscommand_address = nullptr;

//Fingerprints for finding the functions in the .text section.
const char INVOKE_FINGERPRINT[] = {0x8B, 0x4C, 0x24, 0x04, 0x8D, 0x44, 0x24, 0x10, 0x50, 0xFF, 0x74, 0x24, 0x10, 0x6A, 0x00, 0xFF, 0x74, 0x24, 0x14};
const char FSCOMMAND_FINGERPRINT[] = {0x8B, 0x55, 0x10, 0x8B, 0x7D, 0x08, 0x8B, 0x75, 0x0C, 0x89, 0x7C, 0x24, 0x14, 0x80, 0x3A, 0x00, 0xC7, 0x44, 0x24, 0x44, 0x0F, 0x00, 0x00, 0x00, 0xC7, 0x44, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x44, 0x24, 0x30, 0x00};

void FindFunctions(){

	//Get base address of process.
	HINSTANCE base_address = GetModuleHandle(NULL);

	//Find addresses of DOS and NT headers.
	PIMAGE_DOS_HEADER pdosheader = (PIMAGE_DOS_HEADER)base_address;
	PIMAGE_NT_HEADERS pntheaders = (PIMAGE_NT_HEADERS)((DWORD)base_address + pdosheader->e_lfanew);

	//Get address of first section (start of section table) by jumping over the optional header.
	PIMAGE_SECTION_HEADER pSection_header = (PIMAGE_SECTION_HEADER)(((char*)&pntheaders->OptionalHeader) + pntheaders->FileHeader.SizeOfOptionalHeader);

	//Loop through sections searching for the .text section.
	DWORD text_section = 0x0;
	DWORD text_section_size = 0;
	for (int i = 0; i < pntheaders->FileHeader.NumberOfSections - 1; i++){
		if (std::string((char*)&(pSection_header + i)->Name[0]) == ".text"){
			text_section = (DWORD)base_address + (pSection_header + i)->VirtualAddress;
			text_section_size = (pSection_header + i)->Misc.VirtualSize;
			break;
		}
	}

	//TODO: Make cleaner. Support any n-number of functions to search for, instead of copy/pasting loops.
	//TODO: Remove hard-coded offsets to function fingerprints.
	//Search through .text section to find both functions
	for (int i = 0; i < text_section_size - sizeof(INVOKE_FINGERPRINT); i++){
		if (0 == memcmp(reinterpret_cast<void*>(text_section + i), &INVOKE_FINGERPRINT[0], sizeof(INVOKE_FINGERPRINT))){
			invoke_address = (LPVOID)(text_section + i);
		}
	}
	for (int i = 0; i < text_section_size - sizeof(FSCOMMAND_FINGERPRINT); i++){
		if (0 == memcmp(reinterpret_cast<void*>(text_section + i), &FSCOMMAND_FINGERPRINT[0], sizeof(FSCOMMAND_FINGERPRINT))){
			fscommand_address = (LPVOID)(text_section + i - 0x3B); //BAD
		}
	}


}
