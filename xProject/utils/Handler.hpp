#pragma once

#include "network"
#include "PeecStructMessage.hpp"
#include "PeecThreadPool.hpp"

namespace Net {
	template<typename T>
	struct MessageHandler {
		virtual Net::Message<T> handle(Net::OWN_MSG_PTR<T> _msg) = 0;
		virtual ~MessageHandler() = default;
	};

	template<typename T>
	class HandlerMediator {
	private:
		std::unordered_map<T, std::unique_ptr<MessageHandler<T>>> mapHandler;
		Pool::ThreadPool threadPool;
	public:
		
		explicit HandlerMediator(const std::uint8_t& _threadCount) : threadPool{ _threadCount } {}

		void RegisterHandler(T _msgType, std::unique_ptr<MessageHandler<T>> _handler) {
			mapHandler[_msgType] = std::move(_handler);
		}

		void HandleMessage(Net::OWN_MSG_PTR<T> _ownMsg) {
			if (mapHandler.find(_ownMsg->remoteMsg.GetType()) != mapHandler.end()) {
				threadPool.Submit(&HandlerMediator::RunHandlers, this, _ownMsg);
			}
		}

	private:

		void RunHandlers(Net::OWN_MSG_PTR<T> _ownMsg) {
			spdlog::info("Thread: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));

			Net::Message<T> replMsg = mapHandler[_ownMsg->remoteMsg.GetType()]->handle(_ownMsg);
			_ownMsg->remoteConnection->Send(replMsg);
		}
	};

}