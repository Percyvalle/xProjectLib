#pragma once

#include "network/StructMessage.hpp"
#include "utils/ThreadPool.hpp"

namespace Net {
	template<typename TypeMsg, typename TypeMsgStatus>
	struct MessageHandler {
		virtual Net::Message<TypeMsg, TypeMsgStatus> handle(Net::OWN_MSG_PTR<TypeMsg, TypeMsgStatus> _msg) = 0;
		virtual ~MessageHandler() = default;
	};

	template<typename TypeMsg, typename TypeMsgStatus>
	class HandlerMediator {
	private:
		std::unordered_map<TypeMsg, std::unique_ptr<MessageHandler<TypeMsg, TypeMsgStatus>>> mapHandler;
		Pool::ThreadPool threadPool;
	public:
		
		explicit HandlerMediator(const std::uint8_t& _threadCount) : threadPool{ _threadCount } {}

		void RegisterHandler(TypeMsg _msgType, std::unique_ptr<MessageHandler<TypeMsg, TypeMsgStatus>> _handler) {
			mapHandler[_msgType] = std::move(_handler);
		}

		void HandleMessage(Net::OWN_MSG_PTR<TypeMsg, TypeMsgStatus> _ownMsg) {
			if (mapHandler.find(_ownMsg->remoteMsg.GetType()) != mapHandler.end()) {
				threadPool.Submit(&HandlerMediator::RunHandlers, this, _ownMsg);
			}
		}

	private:

		void RunHandlers(Net::OWN_MSG_PTR<TypeMsg, TypeMsgStatus> _ownMsg) {
			spdlog::info("Thread: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));

			Net::Message<TypeMsg, TypeMsgStatus> replMsg = mapHandler[_ownMsg->remoteMsg.GetType()]->handle(_ownMsg);
			_ownMsg->remoteConnection->Send(replMsg);
		}
	};

}