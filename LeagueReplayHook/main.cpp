#include <windows.h>
#include <vector>
#include <string>
#include <iostream>

#include "MiscFunctions.h"
#include "MemoryScanner.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

//Typedef for the Invoke Function.
typedef	void(__cdecl *INVOKE)(DWORD param1, DWORD function_name, DWORD symbol, DWORD data);

//Storage for bytes we lose by patching.
char lost_invoke_bytes[10];
char lost_fs_command_bytes[10];

//FsCommands
const char* CONTROL_STRING = "replayui_setLockCameraEnabled";
BOOL FsCommandQueued = false;
char* FSCOMMAND = "";
char* FSARGUMENT = "";

//Socket for listening for FSCommand instructions
SOCKET RecvSocket;
SOCKET SendSocket;
sockaddr_in RecvAddr;
std::vector<char> UDPBuffer;
const int LISTEN_PORT = 6000;
const int SEND_PORT = 7000;

//Prevent main entry point from being ran twice.
bool injected = false;

void ApplyHook(PVOID function_address, PVOID hook_address, char* lost_bytes){

	//Remove Read-Only protection from function_address.
	DWORD tmp;
	int retn = VirtualProtect(function_address, 1, PAGE_EXECUTE_READWRITE, &tmp);

	//Patch bytes for FAR JMP to our hook.
	char patch_bytes[] = {
		0xB8, 0xFF, 0xFF, 0xFF, 0xFF, //MOV EAX, 0xFFFFFFFF
		0xFF, 0xE0 //JMP EAX
	};

	//Write in the correct address to JMP to.
	memcpy(&patch_bytes[1], &hook_address, sizeof(PVOID));

	//Save the bytes we're going to overwrite.
	memcpy(lost_bytes, function_address, sizeof(patch_bytes));

	//Write in the hook.
	memcpy(function_address, &patch_bytes, sizeof(patch_bytes));

}
void RemoveHook(PVOID function_address, char* lost_bytes){
	memcpy(function_address, lost_bytes, 7); //TODO: Remove magic constant.
}

void    __cdecl   CustomInvoke(DWORD param1, DWORD function_name_param, DWORD symbol_param, DWORD data_param){
	
	//TODO: Create proper hook.
	//Remove the patch so that we can call stuff.	
	RemoveHook(invoke_address, lost_invoke_bytes);

	//Get the major function name. "Update" "UpdatePlayerDataIcons"...etc
	string function_name((char*)function_name_param);

	//Get the symbol representing the param data type.
	string sym_string((char*)symbol_param);

	//Extract the data paramater into a wide string.
	string string_data;
	if (sym_string == "%s") //Narrow string param.
		string_data = string((char*)data_param);
	else if (sym_string == "%ls"){ //Wide string param.
		wstring wide((WCHAR*)data_param);
		string_data = string(wide.begin(), wide.end());
	}
	else if (sym_string == "%d") //Decimal type.
		string_data = to_string(data_param);
	else if (sym_string == "%f") //Double
		string_data = to_string((double)data_param);
	else if (sym_string == "%hf") //Float
		string_data = to_string((float)data_param);
	else if (sym_string == "%u") //Undefined
		string_data = "undefined";

	//Create string to store all data.
	string total(function_name + "," + string_data);

	//Send to UDP socket.
	int ret = sendto(SendSocket, total.c_str(), total.length(), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));

#ifdef _DEBUG
	cout << function_name << "," << string_data << '\n';
#endif

	//Check if we got some new data on UDP port.
	fd_set readFDs = { 0 };
	TIMEVAL tv = { 0 };
	tv.tv_usec = 1; // microseconds
	FD_ZERO(&readFDs);
	FD_SET(RecvSocket, &readFDs);
	if (select(0, &readFDs, NULL, NULL, &tv)){

		//Read the data into our vector.
		char buff[1024];
		int read = recv(RecvSocket, buff, 1024, 0);
		for (int i = 0; i < read; i++){
			UDPBuffer.push_back(buff[i]);
		}

		//Process all valid commands in our buffer.
		int vector_size = UDPBuffer.size();
		for (int i = 0; i < vector_size; i++){

			//Found a command terminator (,)
			if (UDPBuffer[i] == ','){

				//Extract the command.
				string command = string(&UDPBuffer[0], i);

				//Split the command into function name and paramater.
				std::vector<string> tmp = split(command, "=", true);

				//Check if we found both function and paramater.
				if (tmp.size() == 2){

					//Set globals to be used in FSCommand hook.
					FSCOMMAND = (char*)tmp[0].c_str();
					FSARGUMENT = (char*)tmp[1].c_str();

					//Set flag to trigger hook next time it's called.
					FsCommandQueued = true;

					//Bounce a CallbackLockCamera call off of UI swf movie. Ty Rito.
					string function("Update");
					string sym("%s");
					string value("CallbackLockCamera,1");
					((INVOKE)invoke_address)(param1, reinterpret_cast<DWORD>(function.c_str()), reinterpret_cast<DWORD>(sym.c_str()), reinterpret_cast<DWORD>(value.c_str()));

				}

				//Remove the string from the buffer.
				UDPBuffer.erase(UDPBuffer.begin(), UDPBuffer.begin() + i + 1);

				//Restart loop.
				vector_size = UDPBuffer.size();
				i = -1;

			}
		}
	}

	//Make the real call.
	((INVOKE)invoke_address)(param1, function_name_param, symbol_param, data_param);

	//Re-Apply patch.
	ApplyHook(invoke_address, &CustomInvoke, lost_invoke_bytes);

}
void    __declspec(naked) CustomFsCommand(){

	//Injecting Scaleform->Engine commands was found to be too difficult due to the presence
	//of other (global) variables that are hard to find. Instead, we call dummy
	//Invokes() with the "CallbackLockCamera" function which always triggers a
	//return "replayui_setLockCameraEnabled" fscommand (see flash GFX flash files).
	//So we can intercept the "replayui_setLockCameraEnabled" and overwrite without our own command.

	//TODO: Don't write it in assembly.

#define PUSHAD 0x20
	__asm{

		//Save all registers.
		pushad;

		//Erase registers we're going to use.
		xor eax, eax;
		xor ebx, ebx;
		xor edi, edi; //Use for counting.

		//Check if intercept flag is set.
		cmp FsCommandQueued, 0;
		jz finish;
	}

	//Move the fscommand paramater (pointer to string) into ebx.
	__asm{
		mov eax, [esp + PUSHAD + 8];
	}

loop:
	__asm{

		//Move single character pointed to by eax into ebx.
		mov bl, [eax + edi];

		//Compare strings.
		push eax;
		mov eax, [CONTROL_STRING];
		cmp bl, byte ptr[eax + edi];
		pop eax;

		//End if not equal.
		jne finish;

		//Inc edi for next loop (if it happens). (inc resets ZF)
		inc edi;

		//Since equal, check if null terminator.
		cmp bl, 0;

		//Continue loop if not equal (not null terminator yet)
		jne loop;

		//Made it to the null terminator of both. Proceed to replace.
		mov eax, FSCOMMAND;
		mov[esp + PUSHAD + 8], eax;
		mov eax, FSARGUMENT;
		mov[esp + PUSHAD + 12], eax;
		mov FsCommandQueued, 0; //Ensure single-intercept only.
	}

finish:
	__asm{

		//Restore all registers.
		popad;

		//Run lost instructions since we're undoing hook.
		//10 bytes.
		mov edi, edi;
		push ebp;
		mov ebp, esp; 
		and esp, -0x8;
		push -1;

		mov eax, fscommand_address;
		add eax, 10; //Jump over our hook
		jmp eax;//Jump over hook and execute instruction like normal. Hopefully we preserved stack frame.
	}

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved){

	if (!injected){

		//Only run once.
		injected = true;

		//Debugging stuff.
#ifdef _DEBUG
		MessageBoxA(0, "Injected", "Debug Mode", 0);
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
#endif

		//Get addresses of all needed functions.
		FindFunctions();
		if (fscommand_address == nullptr || invoke_address == nullptr){
			Error("Could not find needed functions, check fingerprints / offsets.", true);
		}

		//Appply the patches to the functions.
		ApplyHook(invoke_address, &CustomInvoke, lost_invoke_bytes);
		ApplyHook(fscommand_address, &CustomFsCommand, lost_fs_command_bytes);

		//Initialize Winsock
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR){
			Error("WSAStartup() Failure.", true);
		}

		//Initialize recieve socket.
		RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (RecvSocket == INVALID_SOCKET){
			Error("Failure creating receive socket.", true);
		}

		//Bind recieve socket.
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(LISTEN_PORT);
		RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(RecvSocket, (SOCKADDR *)&RecvAddr, sizeof(RecvAddr))){
			Error("Failure binding receive socket", true);
		}

		//Create send socket.
		SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (SendSocket == INVALID_SOCKET){
			Error("Failure creating send socket.", true);
		}

		//Reuse RecvAddr struct for sending UDP data.
		RecvAddr.sin_port = htons(SEND_PORT);
		RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	}

	return true;
}