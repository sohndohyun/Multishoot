#include "Packet.hpp"

using namespace std;

Packet::Packet(){
	Init();
}

Packet::~Packet(){

}

void Packet::Init(){
	_body.clear();
	_size = -1;
}

void Packet::Add(string const &str){
	_leftStr.append(str);
	if (_size == -1)
	{
		if (_leftStr.size() <= 4)
			return;
		_size = *((int*)_leftStr.substr(0, 4).c_str());
		_leftStr = _leftStr.substr(4);
	}
	if (_leftStr.size() >= _size){
		_body = _leftStr.substr(0, _size);
		_leftStr = _leftStr.substr(_size);
	}
}

void Packet::Add(char *buf, size_t size){
	_leftStr.append(buf, size);
	if (_size == -1)
	{
		if (_leftStr.size() <= 4)
			return;
		_size = *((int*)_leftStr.substr(0, 4).c_str());
		_leftStr = _leftStr.substr(4);
	}
	if (_leftStr.size() >= _size){
		_body = _leftStr.substr(0, _size);
		_leftStr = _leftStr.substr(_size);
	}
}

bool Packet::Made(){
	if (_size != -1 && _body.size() == _size)
		return true;
	return false;
}

string Packet::MakePacket(string const &str){
	int size = str.size();
	char* ch = (char*)&size;
	string rt;
	rt.append(ch, 4);
	rt.append(str);
	return rt;
}