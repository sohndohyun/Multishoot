#pragma once
#include "DRClient.h"
#include "DRQueue.h"
#include "GameController.h"

class MultiShootClient : public DRClient
{
protected:
	virtual void OnUpdate();
	virtual void OnConnected();
	virtual void OnSend(int size);
	virtual void OnRecv(char* data, int size);
	virtual void OnDisconnected();
public:
	MultiShootClient();
	virtual ~MultiShootClient();
public:
	DRQueue<PacketType*> dataQ;
};

