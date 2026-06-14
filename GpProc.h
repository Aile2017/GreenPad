// GpProc.h
// External process launching helper shared between GpMain.cpp and
// GpExFilter.cpp. Wraps CreateProcess so that GreenPad's own directory is
// prepended to the child process PATH (see BuildExternalProcessEnvironment
// in GpMain.cpp).
#pragma once
#include <windows.h>

// Launch a process with GreenPad's directory added to PATH.
// Signature mirrors the relevant CreateProcess parameters.
// Defined in GpMain.cpp.
BOOL CreateExternalProcess(
	LPTSTR cmdLine, BOOL inheritHandles, DWORD creationFlags,
	LPSTARTUPINFO startupInfo, LPPROCESS_INFORMATION processInfo);
