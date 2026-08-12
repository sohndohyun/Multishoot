#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <atomic>

template<class T>
class DRStack
{
private:
	struct alignas(MEMORY_ALLOCATION_ALIGNMENT) Node
	{
		SLIST_ENTRY entry;
		T value;
	};

public:
	DRStack()
	{
		InitializeSListHead(&head);
		sz.store(0);
	}

	virtual ~DRStack()
	{
		while (auto entry = InterlockedPopEntrySList(&head))
			delete reinterpret_cast<Node*>(entry);
	}

	void push(T value)
	{
		auto node = new Node;
		node->value = value;
		sz.fetch_add(1);
		InterlockedPushEntrySList(&head, &node->entry);
	}

	bool pop(T* out)
	{
		auto entry = InterlockedPopEntrySList(&head);
		if (entry == nullptr)
			return false;

		auto node = reinterpret_cast<Node*>(entry);
		*out = node->value;
		delete node;
		sz.fetch_sub(1);
		return true;
	}

	size_t size() const
	{
		return sz.load();
	}

private:
	SLIST_HEADER head;
	std::atomic_size_t sz;
};
