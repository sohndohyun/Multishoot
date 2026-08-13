#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "WS2_32.lib")

#include "DRPacket.h"
#include "Rect.hpp"
#include "RingBuffer.h"
#include "Vector.hpp"
#include "mpsc_channel.hpp"
#include "object_pool.hpp"

#include <WinSock2.h>
#include <Windows.h>
#include <atomic>
#include <list>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
