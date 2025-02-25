#pragma once

#include <atomic>

template<class T>
class DRStack
{
private:
	struct Node {
		T value;
		Node* next;
	};

	struct Head {
		uintptr_t aba;
		Node* node;
	};

public:
	DRStack()
	{
		head.exchange({ 0, nullptr });
		sz = 0;
	}

	virtual ~DRStack() {}

	void push(T value)
	{
		Node* node = new Node;
		node->value = value;
		node->next = nullptr;
		

		push(&head, node);
		sz.fetch_add(1);
	}

	void pop(T* out)
	{
		Node* node = pop(&head);
		if (node == nullptr)
		{
			out = nullptr;
		}
		else
		{
			sz.fetch_sub(1);
			T value = node->value;
			delete node;
			*out = value;
		}
	}

	size_t size()
	{
		return sz.load();
	}

private:
	void push(std::atomic<Head>* head, Node* node)
	{
		Head next;
		Head orig; 
		do {
			orig = head->load();
			node->next = orig.node;
			next.aba = orig.aba + 1;
			next.node = node;
		} while (!head->compare_exchange_strong(orig, next));
	}

	Node* pop(std::atomic<Head>* head)
	{
		Head next;
		Head orig;
		do {
			orig = head->load();
			if (orig.node == nullptr)
				return nullptr;

			next.aba = orig.aba + 1;
			next.node = orig.node->next;

		} while (!head->compare_exchange_strong(orig, next));
		return orig.node;
	}

	std::atomic<Head> head;
	std::atomic_size_t sz;

};

