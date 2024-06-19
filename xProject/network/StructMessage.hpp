#pragma once

#include <boost/format.hpp>
#include <boost/date_time.hpp>

#include "xProject_pch.hpp"

namespace Net {

	template<typename TypeMsg, typename TypeMsgStatus>
	struct HeaderMessage {
		TypeMsg type;
		TypeMsgStatus status;
		uint32_t sizeData = 0;
	};

	struct BodyMessage {
		std::vector<uint8_t> data;
	};
	
	template<typename T, typename = void>
	struct has_data_method : std::false_type {};

	template<typename T>
	struct has_data_method<T, std::void_t<decltype(std::declval<T>().data())>> : std::true_type {};

	template<typename T, typename = void>
	struct has_size_method : std::false_type {};

	template<typename T>
	struct has_size_method<T, std::void_t<decltype(std::declval<T>().size())>> : std::true_type {};

	template<typename T>
	constexpr bool has_data_and_size_v = has_data_method<T>::value && has_size_method<T>::value;


	template<typename TypeMsg, typename TypeMsgStatus>
	struct Message {
		HeaderMessage<TypeMsg, TypeMsgStatus> header;
		BodyMessage body;

		TypeMsg GetType() { return header.type; }
		void SetType(TypeMsg _type) { header.type = _type; }

		TypeMsgStatus GetStatus() { return header.status; }
		void SetStatus(TypeMsgStatus _status) { header.status = _status; }

		void Clear()
		{
			body.data.clear();
			header.sizeData = body.data.size();
		}

		bool IsValidate() const
		{
			return JSON::accept(GetStrData());
		}

		std::string GetStrData() const 
		{
			if (!body.data.empty()) {
				std::string ret;
				ret.resize(body.data.size());
				memcpy(ret.data(), body.data.data(), body.data.size());

				return ret;
			}

			return "";
		}

		JSON GetJSONData() const
		{
			return IsValidate() ? JSON::parse(GetStrData()) : JSON();
		}

		template<typename T, std::enable_if_t<has_data_and_size_v<T>>>
		Message& operator<<(const T& _message)
		{
			static_assert(std::is_standard_layout<T>::value, "Data is too complex to be pushed into vector");

			size_t startPtr = body.data.size();

			body.data.resize(startPtr + sizeof(T));

			std:memcpy(body.data.data() + startPtr, &_message, sizeof(T));

			header.sizeData = body.data.size();

			return *this;
		}

		template<typename T>
		Message& operator<<(const T& _message)
		{
			size_t startPtr = body.data.size();

			body.data.resize(startPtr + _message.size());

			std:memcpy(body.data.data() + startPtr, _message.data(), _message.size());

			header.sizeData = body.data.size();

			return *this;
		}
	};

	template<typename TypeMsg, typename TypeMsgStatus>
	class Connection;

	template<typename TypeMsg, typename TypeMsgStatus>
	struct 	OwnerMessage {
		std::shared_ptr<Connection<TypeMsg, TypeMsgStatus>> remoteConnection = nullptr;
		Message<TypeMsg, TypeMsgStatus> remoteMsg;

		OwnerMessage(std::shared_ptr<Connection<TypeMsg, TypeMsgStatus>> _con, Message<TypeMsg, TypeMsgStatus> _msg) : remoteConnection{ _con }, remoteMsg{ _msg } {}
	};

	template<typename TypeMsg, typename TypeMsgStatus>
	using OWN_MSG_PTR = std::shared_ptr<OwnerMessage<TypeMsg, TypeMsgStatus>>;

	
	template<typename TypeMsg, typename TypeMsgStatus>
	struct MessageFactory
	{
		template<typename Data>
		static Message<TypeMsg, TypeMsgStatus> CreateMessage(TypeMsg _type, TypeMsgStatus _status, Data _msg)
		{
			Net::Message<TypeMsg, TypeMsgStatus> msg;
			msg.SetType(_type);
			msg.SetStatus(_status);
			msg << _msg;

			return msg;
		}

		static Message<TypeMsg, TypeMsgStatus> CreateMessage(TypeMsg _type, TypeMsgStatus _status)
		{
			Net::Message<TypeMsg, TypeMsgStatus> msg;
			msg.SetType(_type);
			msg.SetStatus(_status);

			return msg;
		}
	};

}