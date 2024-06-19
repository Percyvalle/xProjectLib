#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>

#include "collections/QeueuLockfree.hpp"

#include "StructMessage.hpp"

namespace Net {

	namespace asio = boost::asio;

	using SOCKET = asio::ip::tcp::socket;
	using ENDPOINT = asio::ip::tcp::endpoint;
	using ERROR_CODE = boost::system::error_code;

	template<typename TypeMsg, typename TypeMsgStatus>
	class Connection : public std::enable_shared_from_this<Connection<TypeMsg, TypeMsgStatus>> {
	public:
		enum class OwnerConnection {
			Server, Client
		};
	
	private:
		SOCKET connectSocket;
		asio::io_service& connectContext;

		Utils::QueueLF<Net::Message<TypeMsg, TypeMsgStatus>> msgQueueOut;
		Utils::QueueLF<std::shared_ptr<Net::OwnerMessage<TypeMsg, TypeMsgStatus>>>& msgQueueIn;

		Net::Message<TypeMsg, TypeMsgStatus> temporaryMessage;

		OwnerConnection owner;
	public:
		Connection(OwnerConnection _owner, asio::io_service& _context, SOCKET _socket, Utils::QueueLF<std::shared_ptr<Net::OwnerMessage<TypeMsg, TypeMsgStatus>>>& _msgIn)
			: connectContext(_context), connectSocket(std::move(_socket)), msgQueueIn(_msgIn) 
		{
			owner = _owner;
		}
		~Connection() {};

		std::string GetAddressRemote() const { return connectSocket.remote_endpoint().address().to_string(); }
		std::uint16_t GetPortRemote() const { return connectSocket.remote_endpoint().port(); }

		std::string GetAddressLocal() const { return connectSocket.local_endpoint().address().to_string(); }
		std::uint16_t GetPortLocal() const { return connectSocket.local_endpoint().port(); }

		void ConnectToClient() {
			if (owner == OwnerConnection::Server) {
				if (IsConnected()) {
					ReadHeader();
				}
			}
		}

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& _endpoint) {
			if (owner == OwnerConnection::Client) {
				asio::async_connect(connectSocket, _endpoint,
					[this](ERROR_CODE _error_code, ENDPOINT _endpoint) {
						if (!_error_code) {
							ReadHeader();
						}
					}
				);
			}
		}

		void Disconnect() {
			if (IsConnected()) {
				boost::asio::post(connectContext,
					[this]() { connectSocket.close(); }
				);
			}
		}
		bool IsConnected() {
			return connectSocket.is_open();
		}

		void Send(const Net::Message<TypeMsg, TypeMsgStatus>& _msg) {
			boost::asio::post(connectContext,
				[this, _msg]()
				{
					bool messageIsEmpty = msgQueueOut.empty();
					msgQueueOut.push_back(_msg);
					if (messageIsEmpty) {
						WriteHeader();
					}
				}
			);
		}
	private:
		void ReadHeader() {
			asio::async_read(connectSocket, boost::asio::buffer(&temporaryMessage.header, sizeof(Net::HeaderMessage<TypeMsg, TypeMsgStatus>)),
				[this](ERROR_CODE _error_code, std::size_t _length)
				{
					if (!_error_code) {
						if (temporaryMessage.header.sizeData > 0) {
							temporaryMessage.body.data.resize(temporaryMessage.header.sizeData);
							ReadBody();
						}
						else {
							AddMessageToQueue();
						}
					}
					else {
						connectSocket.close();
					}
				}
			);
		}
		void ReadBody() {
			asio::async_read(connectSocket, boost::asio::buffer(temporaryMessage.body.data.data(), temporaryMessage.header.sizeData),
				[this](boost::system::error_code _error_code, std::size_t length)
				{
					if (!_error_code) {
						AddMessageToQueue();
					}
					else {
						connectSocket.close();
					}
				}
			);
		}

		void WriteHeader() {
			asio::async_write(connectSocket, boost::asio::buffer(&msgQueueOut.front().header, sizeof(Net::HeaderMessage<TypeMsg, TypeMsgStatus>)),
				[this](boost::system::error_code _error_code, std::size_t _length)
				{
					if (!_error_code) {
						if (msgQueueOut.front().body.data.size() > 0) {
							WriteBody();
						}
						else {
							msgQueueOut.pop_front();

							if (!msgQueueOut.empty()) {
								WriteHeader();
							}
						}
					}
					else {
						connectSocket.close();
					}
				}
			);
		}
		void WriteBody() {
			asio::async_write(connectSocket, boost::asio::buffer(msgQueueOut.front().body.data.data(), msgQueueOut.front().body.data.size()),
				[this](boost::system::error_code _error_code, std::size_t length)
				{
					if (!_error_code) {
						msgQueueOut.pop_front();

						if (!msgQueueOut.empty()) {
							WriteHeader();
						}
					}
					else {
						connectSocket.close();
					}
				}
			);
		}

		void AddMessageToQueue() {
			if (owner == OwnerConnection::Server) {
				msgQueueIn.push_back(std::make_shared<Net::OwnerMessage<TypeMsg, TypeMsgStatus>>(this->shared_from_this(), temporaryMessage));
			}
			else {
				msgQueueIn.push_back(std::make_shared<Net::OwnerMessage<TypeMsg, TypeMsgStatus>>(nullptr, temporaryMessage));
			}
			temporaryMessage.Clear();

			ReadHeader();
		}
	};

}