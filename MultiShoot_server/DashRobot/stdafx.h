#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "WS2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <WinSock2.h>
#include <Windows.h>
#include <atomic>

#include "RingBuffer.h"
#include "DRStack.h"
#include "DRQueue.h"
#include "DRObjectPool.h"
#include "DRPacket.h"
#include "Rect.hpp"
#include "Vector.hpp"

#include <list>
