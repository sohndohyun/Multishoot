#include "stdafx.h"
#include "DRPacket.h"

DRPacket::DRPacket()
{ 

}

void DRPacket::init()
{
	memset(buffer, 0, BUFSIZ);
	head = (Header*)buffer;
	pPut = pCall = buffer + sizeof(Header);
}

bool DRPacket::put(char* data, int size)
{
	if (pPut - buffer + size > BUFSIZ)
		return false;

	memcpy(pPut, data, size);
	pPut += size;
	return true;
}

bool DRPacket::call(char* data, int size)
{
	if (pPut - pCall < size)
		return false;

	memcpy(data, pCall, size);
	pCall += size;
	return true;
}

int DRPacket::callPacket(char* data)
{
	memcpy(data, buffer, size() + sizeof(Header));
	return size() + sizeof(Header);
}

bool DRPacket::putPacket(char* data, int size)
{
	if (size < sizeof(Header) || size > BUFSIZ)
		return false;

	auto temp = size - sizeof(Header);

	memcpy(buffer, data, size);
	pPut += temp;

	return true;
}

DRPacket::Header* DRPacket::header()
{
	return head;
}

int DRPacket::size()
{
	return pPut - pCall;
}

bool DRPacket::movep(int size)
{
	if(size < head_size())
		return false;
	if (size > BUFSIZ)
		return false;

	pPut = buffer + size;
	return true;
}
