#pragma once

#include "Connection.hpp"

namespace Net {

	template<typename MessageIMPL>
	class ClientInterface {
	protected:
		std::unique_ptr<Net::Connection<MessageIMPL>> connection;
		asio::io_service connectContext;

		std::thread threadContext;
	private:
		Utils::QueueLF<std::shared_ptr<Net::OwnerMessage<MessageIMPL>>> msgQueueIn;

	public:
		ClientInterface() = default;
		~ClientInterface() { Disconnect(); }

		void Connect(const std::string& _host, const uint16_t& _port) {
			asio::ip::tcp::resolver resolver(connectContext);
			asio::ip::tcp::resolver::results_type endpoint = resolver.resolve(_host, std::to_string(_port));

			connection = std::make_unique<Net::Connection<MessageIMPL>>(Net::Connection<MessageIMPL>::OwnerConnection::Client, connectContext, SOCKET(connectContext), msgQueueIn);

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

		void Send(const MessageIMPL& _msg) {
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

		Utils::QueueLF<std::shared_ptr<OwnerMessage<MessageIMPL>>>& Incoming() {
			return msgQueueIn;
		}
	};

}