/*
** NTKbdLites.h 
**
**  Copyright 1999 Mark J. McGinty, All Rights Reserved
**	 Free Usage granted to the public domain.
**
*/


#include <windows.h>

// (some definitions borrowed from ntkbdio.h)

//
// Define the keyboard indicators.
//

#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;		// Unit identifier.
    USHORT LedFlags;		// LED indicator state.

} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1



#ifdef STATIC_LIBRARY
#define DECLSPEC 
#else
#define DECLSPEC __declspec(dllexport)
#endif




int DECLSPEC FlashKeyboardLight(HANDLE hKbdDev, UINT LightFlag, int Duration);
HANDLE DECLSPEC OpenKeyboardDevice(int *ErrorNumber);
int DECLSPEC CloseKeyboardDevice(HANDLE hndKbdDev);
HANDLE DECLSPEC FlashKeyboardLightInThread(UINT, int, LPSTR);


typedef struct {
	UINT		LightFlag;
	int		Duration;
	char		EventName[128];
} FLASH_KBD_THD_INIT, *LPFLASH_KBD_THD_INIT;




