#pragma once
class DRPacket
{
public: 
	struct Header
	{
		char code;
		int size;
	};

	static int head_size() { return sizeof(Header); }


	DRPacket();

	void init();
	bool put(char* data, int size);
	bool call(char* data, int size);
	int callPacket(char* data);
	bool putPacket(char* data, int size);
	Header* header();
	int size();
	int full_size() { return size() + sizeof(Header);}
	char* getBuf() { return buffer; }
	char* getCallP() { return pCall; }
	bool movep(int size);

private:

	char buffer[BUFSIZ];
	Header* head;  
	char* pPut;
	char* pCall;
};

