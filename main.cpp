#include <Windows.h>
#include <stdio.h>
#include "mhook.h"
#include "Shlwapi.h"

#include <openvr.h>

#include "Extras\OVR_CAPI_Util.h"
#include "OVR_Version.h"

/// This is the Windows Named Event name that is used to check for HMD connected state.
#define REV_HMD_CONNECTED_EVENT_NAME L"ReviveHMDConnected"

typedef HMODULE(__stdcall* _LoadLibrary)(LPCWSTR lpFileName);
typedef HANDLE(__stdcall* _OpenEvent)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);

_LoadLibrary TrueLoadLibrary = (_LoadLibrary)GetProcAddress(GetModuleHandle(L"kernel32"), "LoadLibraryW");
_OpenEvent TrueOpenEvent = (_OpenEvent)GetProcAddress(GetModuleHandle(L"kernel32"), "OpenEventW");

HANDLE ReviveModule;

HMODULE WINAPI HookLoadLibrary(LPCWSTR lpFileName)
{
#if defined(_WIN64)
	const char* pBitDepth = "64";
#else
	const char* pBitDepth = "32";
#endif

	LPCWSTR fileName = PathFindFileNameW(lpFileName);
	WCHAR libraryName[MAX_PATH];
	swprintf(libraryName, MAX_PATH, L"LibOVRRT%hs_%d.dll", pBitDepth, OVR_MAJOR_VERSION);
	if (wcscmp(fileName, libraryName) == 0)
		return (HMODULE)ReviveModule;

	return TrueLoadLibrary(lpFileName);
}

HANDLE WINAPI HookOpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
{
	if (wcscmp(lpName, OVR_HMD_CONNECTED_EVENT_NAME) == 0)
		return ::CreateEventW(NULL, TRUE, TRUE, REV_HMD_CONNECTED_EVENT_NAME);

	return TrueOpenEvent(dwDesiredAccess, bInheritHandle, lpName);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	ReviveModule = hModule;

    switch(ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
			Mhook_SetHook((PVOID*)&TrueLoadLibrary, HookLoadLibrary);
			Mhook_SetHook((PVOID*)&TrueOpenEvent, HookOpenEvent);
            break;

		case DLL_PROCESS_DETACH:
			Mhook_Unhook((PVOID*)&HookLoadLibrary);
			Mhook_Unhook((PVOID*)&HookOpenEvent);

        default:
            break;
    }
    return TRUE;
}
