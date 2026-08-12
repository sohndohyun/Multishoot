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
		while (base::pop(&temp))
			delete temp;
	}

public:
	T* Alloc()
	{
		T* temp;
		if (base::pop(&temp))
			return temp;
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

