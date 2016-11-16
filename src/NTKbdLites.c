/*
** NTKbdLites.c 
**
**  Copyright 1999 Mark J. McGinty, All Rights Reserved
**	 Free Usage granted to the public domain.
**
*/

#include <windows.h>
#include <winioctl.h>
#include "NTKbdLites.h"

// Code can be built as either a DLL or a static link library
//
#ifdef STATIC_LIBRARY
#define DECLSPEC 
#else
#define DECLSPEC __declspec(dllexport)
#endif




// FlashKeyboardLight
//
// Flashes the keyboard indicator, specified by LightFlag, one time, 
// at the rate indicated by Duration. All lights are left in their
// previous states when this call returns.
//
// Possible LightFlags:
//		KEYBOARD_CAPS_LOCK_ON   
//		KEYBOARD_NUM_LOCK_ON    
//		KEYBOARD_SCROLL_LOCK_ON

int DECLSPEC FlashKeyboardLight(HANDLE hKbdDev, UINT LedFlag, int Duration)
{
	KEYBOARD_INDICATOR_PARAMETERS InputBuffer;	  // Input buffer for DeviceIoControl
	KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;	  // Output buffer for DeviceIoControl
	UINT				LedFlagsMask;
	BOOL				Toggle;
	ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
	ULONG				ReturnedLength; // Number of bytes returned in output buffer
	int				i;

	InputBuffer.UnitId = 0;
	OutputBuffer.UnitId = 0;

	// Preserve current indicators' state
	//
	if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
				&InputBuffer, DataLength,
				&OutputBuffer, DataLength,
				&ReturnedLength, NULL))
		return GetLastError();

	// Mask bit for light to be manipulated
	//
	LedFlagsMask = (OutputBuffer.LedFlags & (~LedFlag));

	// Set toggle variable to reflect current state.
	//
	Toggle = (OutputBuffer.LedFlags & LedFlag);

	for (i = 0; i < 2; i++)
	{
		Toggle ^= 1;
		InputBuffer.LedFlags = (LedFlagsMask | (LedFlag * Toggle));

		if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
					&InputBuffer, DataLength,
					NULL,	0,	&ReturnedLength, NULL))
			return GetLastError();

		Sleep(Duration);
	}
	return 0;
}



HANDLE DECLSPEC OpenKeyboardDevice(int *ErrorNumber)
{
	HANDLE	hndKbdDev;
	int		*LocalErrorNumber;
	int		Dummy;

	if (ErrorNumber == NULL)
		LocalErrorNumber = &Dummy;
	else
		LocalErrorNumber = ErrorNumber;

	*LocalErrorNumber = 0;
	
	if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
				"\\Device\\KeyboardClass0"))
	{
		*LocalErrorNumber = GetLastError();
		return INVALID_HANDLE_VALUE;
	}

	hndKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
				NULL,	OPEN_EXISTING,	0,	NULL);
	
	if (hndKbdDev == INVALID_HANDLE_VALUE)
		*LocalErrorNumber = GetLastError();

	return hndKbdDev;
}





int DECLSPEC CloseKeyboardDevice(HANDLE hndKbdDev)
{
	int e = 0;

	if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
		e = GetLastError();

	if (!CloseHandle(hndKbdDev))					
		e = GetLastError();

	return e;
}


// Thread procedure to make a light flash continuously.
//
DWORD WINAPI FlashKeyboardLightThd(LPVOID lpv)
{
	LPFLASH_KBD_THD_INIT pInit = (LPFLASH_KBD_THD_INIT)lpv;
	FLASH_KBD_THD_INIT Init = *pInit;
	HANDLE	hndKbdDev;
	HANDLE	heventCancel = OpenEvent(EVENT_ALL_ACCESS, FALSE, Init.EventName);

	if (heventCancel == NULL)
		ExitThread(-1);

	hndKbdDev = OpenKeyboardDevice(NULL);
	if (hndKbdDev == INVALID_HANDLE_VALUE)
	{
		CloseHandle(heventCancel);
		ExitThread(-1);
	}

	for (;;)
	{
		FlashKeyboardLight(hndKbdDev, Init.LightFlag, Init.Duration);
		
		if (WaitForSingleObject(heventCancel, Init.Duration) != WAIT_TIMEOUT)
			break;
	}

	Sleep(Init.Duration);

	CloseHandle(heventCancel);
	CloseKeyboardDevice(hndKbdDev);
	
	ExitThread(0);
	return 0;
}


// Builds structure and creates thread, to flash light continuously
//
HANDLE DECLSPEC FlashKeyboardLightInThread(UINT LightFlag, int Duration, LPSTR EventName)
{
	DWORD ThreadId;
	static FLASH_KBD_THD_INIT FlashInit;

	FlashInit.LightFlag = LightFlag;
	FlashInit.Duration = Duration;
	lstrcpyn(FlashInit.EventName, EventName, 128);

	return CreateThread(NULL, 0, FlashKeyboardLightThd, (LPVOID)&FlashInit, 0, &ThreadId);
 
}
