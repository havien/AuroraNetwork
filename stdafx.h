// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <iostream>

//socket
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
//#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// TODO: reference additional headers your program requires here
#include "NetworkDefine.h"

#include "../AprilUtility/TempDefine.h"
#include "../AprilUtility/AprilSingleton.h"
#include "../AprilUtility/WindowsUtilityManager.h"