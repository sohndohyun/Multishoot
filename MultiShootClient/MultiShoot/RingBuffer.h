#pragma once
#include <WinSock2.h>

class RingBuffer
{
public:
	enum e_AYA_STREAM_SQ
	{
		eBUFFER_DEFAULT = 20960,		// ������ �⺻ ũ��.
		eBUFFER_BLANK = 8				// Ȯ���� ������ ���� 8Byte �� �����.
	};

public:

	RingBuffer(void);
	RingBuffer(int iBufferSize);

	virtual	~RingBuffer(void);
	void	Initial(int iBufferSize);
	int		GetBufferSize(void);
	int		GetUseSize(void);
	int		GetFreeSize(void);
	int		GetNotBrokenGetSize(void);
	int		GetNotBrokenPutSize(void);
	int		Put(char* chpData, int iSize);
	int		Get(char* chpDest, int iSize);
	int		Peek(char* chpDest, int iSize);



	void	RemoveData(int iSize);
	void	ClearBuffer(void);
	char* GetBufferPtr(void);
	char* GetReadBufferPtr(void);
	char* GetWriteBufferPtr(void);
	void	Lock(void);
	void	Unlock(void);


protected:

	char*				m_chpBuffer;
	int					m_iBufferSize;
	int					m_iReadPos;
	int					m_iWritePos;
	CRITICAL_SECTION	m_csQueue;
};
