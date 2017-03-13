#pragma once

#include <stdio.h>
#include <tchar.h>
#include <iostream>

#include <vector>
#include <list>
#include <queue>
#include <unordered_map>
#include <algorithm>

//socket
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>

// TODO: reference additional headers your program requires here
#include "NetworkDefine.h"

#pragma comment(lib, "ws2_32.lib")