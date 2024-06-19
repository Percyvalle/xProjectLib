#pragma once

#include "Connection.hpp"

namespace Net {

	template<typename TypeMsg, typename TypeMsgStatus>
	class ClientInterface {
	protected:
		std::unique_ptr<Net::Connection<TypeMsg, TypeMsgStatus>> connection;
		asio::io_service connectContext;

		std::thread threadContext;
	private:
		Utils::QueueLF<std::shared_ptr<Net::OwnerMessage<TypeMsg, TypeMsgStatus>>> msgQueueIn;

	public:
		ClientInterface() = default;
		~ClientInterface() { Disconnect(); }

		void Connect(const std::string& _host, const uint16_t& _port) {
			asio::ip::tcp::resolver resolver(connectContext);
			asio::ip::tcp::resolver::results_type endpoint = resolver.resolve(_host, std::to_string(_port));

			connection = std::make_unique<Net::Connection<TypeMsg, TypeMsgStatus>>(Net::Connection<TypeMsg, TypeMsgStatus>::OwnerConnection::Client, connectContext, SOCKET(connectContext), msgQueueIn);

			connection->ConnectToServer(endpoint);

			threadContext = std::thread([this]() { connectContext.run(); });
		}

		void Disconnect() {
			if (IsConnected()) {
				connection->Disconnect();
			}

			connectContext.stop();
			if (threadContext.joinable()) {
				threadContext.join();
			}

			connection.release();
		}

		bool IsConnected() {
			if (connection) {
				return connection->IsConnected();
			}
			else {
				return false;
			}
		}

		void Send(const Net::Message<TypeMsg, TypeMsgStatus>& _msg) {
			if (IsConnected()) {
				connection->Send(_msg);
			}
		}

		std::string GetAddress() const
		{
			return connection->GetAddressLocal();
		}

		std::uint16_t GetPort() const
		{
			return connection->GetPortLocal();
		}

		Utils::QueueLF<std::shared_ptr<OwnerMessage<TypeMsg, TypeMsgStatus>>>& Incoming() {
			return msgQueueIn;
		}
	};

}