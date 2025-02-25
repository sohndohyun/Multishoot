#include "RingBuffer.h"

RingBuffer::RingBuffer()
{
	InitializeCriticalSection(&m_csQueue);

	m_chpBuffer = NULL;
	m_iBufferSize = 0;

	m_iReadPos = 0;
	m_iWritePos = 0;

	Initial(eBUFFER_DEFAULT);

}
RingBuffer::RingBuffer(int iBufferSize)
{
	InitializeCriticalSection(&m_csQueue);

	m_chpBuffer = NULL;
	m_iBufferSize = 0;

	m_iReadPos = 0;
	m_iWritePos = 0;

	Initial(iBufferSize);
}
RingBuffer::~RingBuffer()
{
	DeleteCriticalSection(&m_csQueue);

	if (NULL != m_chpBuffer)
		delete[] m_chpBuffer;

	m_chpBuffer = NULL;
	m_iBufferSize = 0;

	m_iReadPos = 0;
	m_iWritePos = 0;

}

void RingBuffer::Initial(int iBufferSize)
{
	if (NULL != m_chpBuffer)
		delete[] m_chpBuffer;

	if (0 >= iBufferSize) return;

	m_iBufferSize = iBufferSize;

	m_iReadPos = 0;
	m_iWritePos = 0;

	m_chpBuffer = new char[iBufferSize];

}


int	RingBuffer::GetBufferSize(void)
{
	if (NULL != m_chpBuffer)
	{
		return m_iBufferSize - eBUFFER_BLANK;
	}

	return 0;
}

int RingBuffer::GetUseSize(void)
{
	if (m_iReadPos <= m_iWritePos)
	{
		return m_iWritePos - m_iReadPos;
	}
	else
	{
		return m_iBufferSize - m_iReadPos + m_iWritePos;
	}

}

int RingBuffer::GetFreeSize(void)
{
	return m_iBufferSize - (GetUseSize() + eBUFFER_BLANK);
}

int RingBuffer::GetNotBrokenGetSize(void)
{
	if (m_iReadPos <= m_iWritePos)
	{
		return m_iWritePos - m_iReadPos;
	}
	else
	{
		return m_iBufferSize - m_iReadPos;
	}
}

int RingBuffer::GetNotBrokenPutSize(void)
{
	if (m_iWritePos <= m_iReadPos)
	{
		return (m_iReadPos - m_iWritePos) - eBUFFER_BLANK;
	}
	else
	{
		if (m_iReadPos < eBUFFER_BLANK)
		{
			return (m_iBufferSize - m_iWritePos) - (eBUFFER_BLANK - m_iReadPos);
		}
		else
		{
			return m_iBufferSize - m_iWritePos;
		}
	}
}

int RingBuffer::Put(char* chpData, int iSize)
{
	int iWrite;

	if (GetFreeSize() < iSize)
	{
		return 0;
		iSize = GetFreeSize();
	}

	if (0 >= iSize)
		return 0;

	if (m_iReadPos <= m_iWritePos)
	{
		iWrite = m_iBufferSize - m_iWritePos;

		if (iWrite >= iSize)
		{
			memcpy(m_chpBuffer + m_iWritePos, chpData, iSize);
			m_iWritePos += iSize;
		}
		else
		{
			memcpy(m_chpBuffer + m_iWritePos, chpData, iWrite);
			memcpy(m_chpBuffer, chpData + iWrite, iSize - iWrite);
			m_iWritePos = iSize - iWrite;
		}
	}
	else
	{
		memcpy(m_chpBuffer + m_iWritePos, chpData, iSize);
		m_iWritePos += iSize;
	}

	m_iWritePos = m_iWritePos == m_iBufferSize ? 0 : m_iWritePos;

	return iSize;
}
int RingBuffer::Get(char* chpDest, int iSize)
{
	int iRead;

	if (GetUseSize() < iSize)
		iSize = GetUseSize();

	if (0 >= iSize)
		return 0;

	if (m_iReadPos <= m_iWritePos)
	{
		memcpy(chpDest, m_chpBuffer + m_iReadPos, iSize);
		m_iReadPos += iSize;
	}
	else
	{
		iRead = m_iBufferSize - m_iReadPos;

		if (iRead >= iSize)
		{
			memcpy(chpDest, m_chpBuffer + m_iReadPos, iSize);
			m_iReadPos += iSize;
		}
		else
		{
			memcpy(chpDest, m_chpBuffer + m_iReadPos, iRead);
			memcpy(chpDest + iRead, m_chpBuffer, iSize - iRead);
			m_iReadPos = iSize - iRead;
		}
	}

	return iSize;
}
int	RingBuffer::Peek(char* chpDest, int iSize)
{
	int iRead;
	if (GetUseSize() < iSize)
		iSize = GetUseSize();

	if (0 >= iSize)
		return 0;

	if (m_iReadPos <= m_iWritePos)
	{
		memcpy(chpDest, m_chpBuffer + m_iReadPos, iSize);
	}
	else
	{
		iRead = m_iBufferSize - m_iReadPos;
		if (iRead >= iSize)
		{
			memcpy(chpDest, m_chpBuffer + m_iReadPos, iSize);
		}
		else
		{
			memcpy(chpDest, m_chpBuffer + m_iReadPos, iRead);
			memcpy(chpDest + iRead, m_chpBuffer, iSize - iRead);
		}
	}

	return iSize;

}

void RingBuffer::RemoveData(int iSize)
{
	int iRead;

	if (GetUseSize() < iSize)
		iSize = GetUseSize();

	if (0 >= iSize)
		return;

	if (m_iReadPos < m_iWritePos)
	{
		m_iReadPos += iSize;
	}
	else
	{
		iRead = m_iBufferSize - m_iReadPos;

		if (iRead >= iSize)
		{
			m_iReadPos += iSize;
		}
		else
		{
			m_iReadPos = iSize - iRead;
		}
	}

	m_iReadPos = m_iReadPos == m_iBufferSize ? 0 : m_iReadPos;
}

void RingBuffer::ClearBuffer(void)
{
	m_iReadPos = 0;
	m_iWritePos = 0;
}


char* RingBuffer::GetBufferPtr(void)
{
	return m_chpBuffer;
}

char* RingBuffer::GetReadBufferPtr(void)
{
	return m_chpBuffer + m_iReadPos;
}

char* RingBuffer::GetWriteBufferPtr(void)
{
	return m_chpBuffer + m_iWritePos;
}

void RingBuffer::Lock(void)
{
	EnterCriticalSection(&m_csQueue);
}

void RingBuffer::Unlock(void)
{
	LeaveCriticalSection(&m_csQueue);
}
