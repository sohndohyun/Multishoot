#pragma once
#include <cstddef>

#ifndef BUFSIZ
#define BUFSIZ 512
#endif

class DRPacket
{
public: 
	struct Header
	{
		char code;
		int size;
	};

	static constexpr char CODE = 12;
	static int head_size() { return sizeof(Header); }
	static int max_data_size() { return BUFSIZ - sizeof(Header); }


	DRPacket();

	void init();
	bool put(char* data, int size);
	int callPacket(char* data);
	bool putPacket(char* data, int size);
	bool call(char* data, int size);
	Header* header();
	int size();
	int full_size() { return size() + sizeof(Header); }
	char* getBuf() { return buffer; }
	char* getCallP() { return pCall; }
	bool movep(int size);


private:

	alignas(std::max_align_t) char buffer[BUFSIZ];
	Header* head;  
	char* pPut;
	char* pCall;
};

