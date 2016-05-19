// inject.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <stdio.h>
#include <windows.h>

#define DLL_EXPORT extern "C" __declspec(dllexport)


struct hooker 
{
	BYTE PatchType;
	DWORD address;
};

typedef BOOL (WINAPI *textout)(HDC, int, int, UINT, CONST RECT*, LPCWSTR, int, CONST INT*);
// faker api
BOOL WINAPI my_TextOutA(HDC, int, int, LPCWSTR, int);
BOOL WINAPI my_TextOutW(HDC, int, int, LPCWSTR, int);
BOOL WINAPI my_ExtTextOutA(HDC, int, int, UINT, CONST RECT*, LPCWSTR, int, CONST INT*);
BOOL WINAPI my_ExtTextOutW(HDC, int, int, UINT, CONST RECT*, LPCWSTR, int, CONST INT*);

// trampoline
BOOL (WINAPI *Tram_TextOutA)(HDC, int, int, LPCWSTR, int);
BOOL (WINAPI *Tram_TextOutW)(HDC, int, int, LPCWSTR, int);
BOOL (WINAPI *Tram_ExtTextOutA)(HDC, int, int, UINT, CONST RECT*, LPCWSTR, int, CONST INT*);
BOOL (WINAPI *Tram_ExtTextOutW)(HDC, int, int, UINT, CONST RECT*, LPCWSTR, int, CONST INT*);


BYTE HookCode[8];
BYTE SourceCode[8];
FARPROC CodeAdress;
DWORD SourceOld;

BYTE SourceFunc[10];

FILE *fp;


//FUNCTION EXPORTS
DLL_EXPORT void Test();

// commom functions
BOOL Prepare();
void HookOn(DWORD address, bool*,  BYTE*);
BOOL Patch(DWORD address, bool*, BYTE*, DWORD*);
BOOL HookOff();
void show_hello_message();


BOOL Prepare()
{
	fp = fopen("out.txt", "w+");

	// TextOUT function address
	DWORD TextOutAAddress = NULL;
	DWORD TextOutWAddress = NULL;
	DWORD ExtTextOutAAddress = NULL;
	DWORD ExtTextOutWAddress = NULL;

    HMODULE hModule = GetModuleHandle(TEXT("gdi32.dll"));

    TextOutAAddress = (DWORD)GetProcAddress(hModule, "TextOutA");
	if(TextOutAAddress) {
		HookOn(TextOutAAddress, (bool*)(&my_TextOutA), SourceFunc);
	}

    TextOutWAddress = (DWORD)GetProcAddress(hModule, "TextOutW");
	if(TextOutWAddress) {
		HookOn(TextOutWAddress, (bool*)(&my_TextOutW), SourceFunc);
	}

	ExtTextOutAAddress = (DWORD)GetProcAddress(hModule, "ExtTextOutA");
	if(ExtTextOutAAddress) {
		HookOn(ExtTextOutAAddress, (bool*)(&my_ExtTextOutA), SourceFunc);
	}

    ExtTextOutWAddress = (DWORD)GetProcAddress(hModule, "ExtTextOutW");
	if(ExtTextOutWAddress) {
		HookOn(ExtTextOutWAddress, (bool*)(&my_ExtTextOutW), SourceFunc);
	}


    if(TextOutAAddress == NULL && TextOutWAddress == NULL && ExtTextOutAAddress == NULL && ExtTextOutWAddress == NULL ) {
		MessageBox(NULL, "This is not GUI Programer.", "Hook message", MB_OK);
		return false;
	}

	// for debug
	// char str[80];
    // sprintf(str, "Process: [ %u ]", FuncAddress);
    MessageBox(NULL, "After HOOK", "Pid message", MB_OK);
    return true;
}

void HookOn(DWORD address, const bool *myfunc,  BYTE *cache)
{
	

	DWORD length = 0;
	DWORD originLength = 0;
	BYTE replace[10];
	

	if(Patch(address, myfunc, cache, &length))
	{
		Tram_ExtTextOutW = (textout)VirtualAlloc(NULL, length << 2, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		memcpy((void*)Tram_ExtTextOutW, cache, length);
		Patch((DWORD)Tram_ExtTextOutW+length, (bool*)(address+length), cache, &originLength);
		
		// for debug
		char str[80];
		sprintf(str, "cache address: [ %p ]", cache);
		MessageBox(NULL, str, "Hook Message", MB_OK);
	}
	
}

BOOL Patch(DWORD address, bool *myfunc, BYTE *cache, DWORD *length)
{
	DWORD oldvalue, temp;
	struct hooker pwrite = 
	{
		0xE9,
		(DWORD)myfunc - (address + sizeof(DWORD) + sizeof(BYTE))
	};
	VirtualProtect((LPVOID)address, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &oldvalue);
	ReadProcessMemory(GetCurrentProcess(), (LPCVOID)address, (LPVOID)cache, sizeof(pwrite), length);
	BOOL ret = WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, cache, sizeof(pwrite), NULL);
	VirtualProtect((LPVOID)address, sizeof(DWORD), oldvalue, &temp);
	
	return ret;
}

BOOL HookOff()
{
    VirtualProtect(CodeAdress, 32, PAGE_READWRITE, &SourceOld);
    _asm
    {
        mov  eax, CodeAdress
        movq mm0, SourceCode
        movq eax, mm0
        emms
    }
    VirtualProtect(CodeAdress, 32, SourceOld, &SourceOld);
    VirtualUnlock(CodeAdress, 32);
	MessageBox(NULL, "HookOff()", "Hook message", MB_OK);
	return(true);
}


// Faker APIS
BOOL WINAPI my_TextOutA( HDC hdc,
						    int X,
							int Y,
							LPCWSTR lpString,
							int cbString)
{
	return true;
}
BOOL WINAPI my_TextOutW( HDC hdc,
						    int X,
							int Y,
							LPCWSTR lpString,
							int cbString)
{
	return true;
}
BOOL WINAPI my_ExtTextOutA( HDC hdc,
						    int X,
							int Y,
							UINT fuOptions,
							CONST RECT *lprc,
							LPCWSTR lpString,
							int cbString,
							CONST INT *lpDx)
{
	return true;
}
BOOL WINAPI my_ExtTextOutW( HDC hdc,
						    int X,
							int Y,
							UINT fuOptions,
							CONST RECT *lprc,
							LPCWSTR lpString,
							int cbString,
							CONST INT *lpDx)
{
	
	fprintf(fp, "text: %d", lpString);
    BOOL retval = Tram_ExtTextOutW(hdc, X, Y, fuOptions, lprc, lpString, cbString, lpDx);
    return(retval);
}



BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			Prepare();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			HookOff();
			fclose(fp);
			break;
    }
    return TRUE;
}


void show_hello_message()
{
    char str[80];
    int id = GetCurrentProcessId();
    sprintf(str, "Process: [ %d ]", id);
    MessageBox(NULL, str, "Pid message", MB_OK);
}


DLL_EXPORT void Test()
{
	//show_hello_message();
}
