#pragma once

#include <atomic>

// Lock-free multi-producer, single-consumer queue.
template <class T>
class DRQueue
{
private:
	struct Node
	{
		T value;
		std::atomic<Node*> next;

		Node() : next(nullptr) {}
		explicit Node(const T& value) : value(value), next(nullptr) {}
	};

public:
	DRQueue()
	{
		head = new Node;
		tail.store(head);
		sz.store(0);
	}

	~DRQueue()
	{
		while (head != nullptr)
		{
			auto next = head->next.load();
			delete head;
			head = next;
		}
	}

	void push(const T& value)
	{
		auto node = new Node(value);
		auto previous = tail.exchange(node, std::memory_order_acq_rel);
		sz.fetch_add(1, std::memory_order_release);
		previous->next.store(node, std::memory_order_release);
	}

	bool pop(T* value)
	{
		auto next = head->next.load(std::memory_order_acquire);
		if (next == nullptr)
			return false;

		*value = next->value;
		delete head;
		head = next;
		sz.fetch_sub(1, std::memory_order_release);
		return true;
	}

	size_t size() const
	{
		return sz.load(std::memory_order_acquire);
	}

private:
	Node* head;
	std::atomic<Node*> tail;
	std::atomic_size_t sz;
};
