#pragma once

#include <string>

class Packet{
private:
	std::string _leftStr;
	int _size;
	std::string _body;
public:
	Packet();
	~Packet();

	void Init();
	void Add(std::string const &str);
	void Add(char *buf, size_t size);
	bool Made();
	std::string const &GetBody(){return _body;}

	static std::string MakePacket(std::string const &str);
};