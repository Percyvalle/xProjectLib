#pragma once

#include "Connection.hpp"

#include <spdlog/spdlog.h>
#include <deque>

namespace Net {

	template<typename MessageIMPL>
	class ServerInterface {
	protected:
		std::deque<std::shared_ptr<Net::Connection<MessageIMPL>>> connections;
		Utils::QueueLF<std::shared_ptr<Net::OwnerMessage<MessageIMPL>>> msgQueueIn;

		asio::io_service connectContext;
		asio::ip::tcp::acceptor acceptor;

		std::thread threadContext;

	public:
		ServerInterface(const uint16_t& _port) : acceptor(connectContext, ENDPOINT(asio::ip::address_v4::from_string("127.0.0.1"), _port)) {}
		~ServerInterface() { Stop(); }

		void Start() 
		{
			WaitForClientConnection();
			threadContext = std::thread([this]() { connectContext.run(); });

			spdlog::info("Server Endpoint: {0}:{1}", acceptor.local_endpoint().address().to_string(), acceptor.local_endpoint().port());
		}

		void Stop() 
		{
			connectContext.stop();
			if (threadContext.joinable()) { threadContext.join(); }
		}

		void WaitForClientConnection() 
		{
			acceptor.async_accept(
				[this](ERROR_CODE _error_code, SOCKET _socket)
				{
					if (!_error_code)
					{
						std::shared_ptr<Net::Connection<MessageIMPL>> clientConnection = std::make_shared<Net::Connection<MessageIMPL>>(Net::Connection<MessageIMPL>::OwnerConnection::Server, connectContext, std::move(_socket), msgQueueIn);

						OnConnect(clientConnection);

						connections.push_back(clientConnection);
						connections.back()->ConnectToClient();
					}
					else {
						// spdlog::warn("[Server] New Connection Error: {0}", _error_code.message());
					}

					WaitForClientConnection();
				}
			);
		}

		void Update(bool _wait = false) 
		{
			if (_wait) 
			{
				msgQueueIn.wait();
			}

			while (!msgQueueIn.empty()) 
			{

				std::shared_ptr<Net::OwnerMessage<MessageIMPL>> ownerMessage = msgQueueIn.pop_front();

				OnMessage(ownerMessage);
			}
		}

		void CheckClientConnection() 
		{
			bool invalidClient = false;
			for (auto& client : connections) 
			{
				if (client.get() == nullptr)
				{
					invalidClient = true;
				}
				else
				{
					if (!client->IsConnected()) 
					{
						OnDisconnect(client);
						client.reset();

						invalidClient = true;
					}
				}
			}

			if (invalidClient)
			{
				connections.erase(std::remove(connections.begin(), connections.end(), nullptr), connections.end());
			}
		}

		std::string GetAddress() const
		{
			return acceptor.local_endpoint().address().to_string();
		}

		std::uint16_t GetPort() const
		{
			return acceptor.local_endpoint().port();
		}

	protected:
		virtual void OnMessage(std::shared_ptr<Net::OwnerMessage<MessageIMPL>> _ownMsg) = 0;

		virtual void OnConnect(std::shared_ptr<Net::Connection<MessageIMPL>> _handleClient) = 0;

		virtual void OnDisconnect(std::shared_ptr<Net::Connection<MessageIMPL>> _handleClient) = 0;
	};

}