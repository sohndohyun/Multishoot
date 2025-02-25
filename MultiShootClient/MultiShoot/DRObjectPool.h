#pragma once
#include "DRStack.h"

template<class T>
class DRObjectPool : protected DRStack<T*>
{
	typedef DRStack<T*> base;
public:
	DRObjectPool() { }
	~DRObjectPool() 
	{ 
		T* temp;
		while (GetSize())
		{
			base::pop(&temp);
			delete temp;
		}
	}

public:
	T* Alloc()
	{
		if (base::size() > 0)
		{
			T* temp;
			base::pop(&temp);
			return temp;
		}
		return new T;
	}

	void Dealloc(T* mem)
	{
		base::push(mem);
	}

	size_t GetSize()
	{
		return base::size();
	}
};

