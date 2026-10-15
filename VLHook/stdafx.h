// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#define _CRT_RAND_S
#define _CRT_SECURE_NO_WARNINGS
#define _USE_32BIT_TIME_T

#ifndef WINVER					// Allow use of features specific to Windows 2000 or later.
#define WINVER 0x0501			// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT			// Allow use of features specific to Windows 2000 or later.                   
#define _WIN32_WINNT 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS			// Allow use of features specific to Windows 2000 or later.
#define _WIN32_WINDOWS 0x0501	// Change this to the appropriate value to target Windows 2000 or later.
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <tchar.h>

#include "User32.h"
