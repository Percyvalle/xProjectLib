#pragma once

#include <atomic>
#include <memory>

namespace Utils
{

	template<typename TypeVal>
	struct Node
	{
		TypeVal value;
		std::atomic<Node*> next = nullptr;
		
		explicit Node(const TypeVal& _val) : value(_val){}
		explicit Node(TypeVal&& _val) : value(std::forward<TypeVal>(_val)) {}
		Node() = default;
		Node(const Node&) = default;
		Node(Node&&) = default;
		Node& operator=(Node&&) = default;
		Node& operator=(const Node&) = default;

	};

	template<typename TypeVal>
	class QueueLF
	{

	private:
		std::atomic<Node<TypeVal>*> head;
		std::atomic<Node<TypeVal>*> tail;
		std::atomic_bool stopWait = false;

		std::mutex m_blockingQueue;
		std::condition_variable m_cvQueue;
	public:
		QueueLF()
		{
			Node<TypeVal>* dummy = new Node<TypeVal>;
			head.store(dummy, std::memory_order_release);
			tail.store(dummy, std::memory_order_release);
		}

		~QueueLF()
		{
			stop_wait();
			clear();
		}

		inline TypeVal& front()
		{
			if (!empty())
			{
				return head.load(std::memory_order_acquire)->next.load(std::memory_order_acquire)->value;
			}
			return head.load(std::memory_order_acquire)->value;
		}

		inline void push_back(const TypeVal& _val)
		{
			Node<TypeVal>* newTail = new Node<TypeVal>(std::move(_val));
			Node<TypeVal>* expected = nullptr;
			while (true)
			{
				Node<TypeVal>* curTail = tail.load(std::memory_order_acquire);
				if (curTail->next.compare_exchange_strong(expected, newTail))
				{
					if (tail.compare_exchange_strong(curTail, newTail))
					{
						m_cvQueue.notify_one();
					}
					return;
				}
				else 
				{
					if (tail.compare_exchange_strong(curTail, curTail->next))
					{
						m_cvQueue.notify_one();
					}
				}
			}
		}

		inline void push_back(TypeVal&& _val)
		{
			Node<TypeVal>* newTail = new Node<TypeVal>(std::forward<TypeVal>(_val));
			Node<TypeVal>* expected = nullptr;
			while (true)
			{
				Node<TypeVal>* curTail = tail.load(std::memory_order_acquire);
				if (curTail->next.compare_exchange_strong(expected, newTail))
				{
					if (tail.compare_exchange_strong(curTail, newTail))
					{
						m_cvQueue.notify_one();
					}
					return;
				}
				else
				{
					if (tail.compare_exchange_strong(curTail, curTail->next))
					{
						m_cvQueue.notify_one();
					}
				}
			}
		}

		inline TypeVal pop_front()
		{
			while (true)
			{
				Node<TypeVal>* curHead = head.load(std::memory_order_acquire);
				Node<TypeVal>* curTail = tail.load(std::memory_order_acquire);
				Node<TypeVal>* nextHead = head.load(std::memory_order_acquire)->next.load(std::memory_order_acquire);
				if (curHead == curTail)
				{
					if (nextHead == nullptr)
					{
						return std::move(head.load(std::memory_order_acquire)->value);
					}
					else
					{
						tail.compare_exchange_strong(curTail, nextHead);
						if (curHead != nullptr)
						{
							delete curHead;
						}
					}
				}
				else
				{
					TypeVal result = std::move(nextHead->value);
					if (head.compare_exchange_strong(curHead, nextHead))
					{
						if (curHead != nullptr)
						{
							delete curHead;
						}
						return result;
					}
				}
			}
		}

		inline bool empty()
		{
			return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
		}

		inline void clear()
		{
			while (!empty())
			{
				pop_front();
			}
		}

		inline void stop_wait()
		{
			stopWait.store(true, std::memory_order_release);
			m_cvQueue.notify_all();
		}

		inline void wait() {
			std::unique_lock<std::mutex> lock(m_blockingQueue);
			m_cvQueue.wait(lock, [this]() { return !empty() || stopWait.load(std::memory_order_acquire); });
		}
	};
}