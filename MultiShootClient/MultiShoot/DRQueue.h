#pragma once
#include <atomic>

using std::atomic;

template <class T>
class DRQueue
{
private:
	struct Node;

	struct Pointer
	{
		uintptr_t volatile aba;
		Node* volatile node;

		bool operator==(Pointer cmp)
		{
			if (aba == cmp.aba && node == cmp.node)
				return true;
			else
				return false;
		}
	};

	struct Node
	{
		T value;
		atomic<Pointer> next;
	};


public:
	DRQueue()
	{
		auto node = new Node;
		Pointer p;
		p.aba = 0;
		p.node = nullptr;
		node->next.exchange(p);
		head.exchange({0, node});
		tail.exchange({0, node});
		sz.exchange(0);
	}

	~DRQueue()
	{
		
	}

	void push(T value)
	{
		Pointer orig;
		Pointer next;
		Pointer temp;

		auto node = new Node;
		node->value = value;
		node->next.exchange({0, nullptr});
		do
		{
			orig = tail.load();
			next = orig.node->next.load();
			if (orig == tail.load())
			{
				if (next.node == nullptr)
				{
					temp.aba = next.aba + 1;
					temp.node = node;
					if (orig.node->next.compare_exchange_strong(next, temp))
						break;
				}
				else
				{
					temp.node = next.node;
					temp.aba = orig.aba + 1;
					tail.compare_exchange_strong(orig, temp);
				}
			}
		} while (true);

		temp.node = node;
		temp.aba = orig.aba + 1;
		tail.compare_exchange_strong(orig, temp);

		sz.fetch_add(1);
	}

	bool pop(T* value)
	{
		Pointer horig;
		Pointer torig;
		Pointer next;
		Pointer temp;
		do
		{
			horig = head.load();
			torig = tail.load();
			next = horig.node->next.load();

			if (horig == head.load())
			{
				if (horig.node == torig.node)
				{
					if (next.node == nullptr)
					{
						value = nullptr;
						return false;
					}
					temp.node = next.node;
					temp.aba = torig.aba + 1;
					tail.compare_exchange_strong(torig, temp);
				}
				else
				{
					*value = next.node->value;
					temp.node = next.node;
					temp.aba = horig.aba + 1;
					if (head.compare_exchange_strong(horig, temp))
					{
						break;
					}
				}
			}
		} while (true);

		delete horig.node;

		sz.fetch_sub(1);
		return true;
	}

	size_t size()
	{
		return sz.load();
	}
private:

	atomic<Pointer> head;
	atomic<Pointer> tail;

	atomic<size_t> sz;
};