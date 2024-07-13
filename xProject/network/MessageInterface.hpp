#pragma once

#include <vector>
#include <memory>

namespace Net {

	template<typename TypeMsg, typename StatusMsg>
	struct IHeader {
	protected:
		TypeMsg type;
		StatusMsg status;

		uint32_t sizeData = 0;
	public:
		const uint32_t Size() const { return sizeData; }
		void SetSize(uint32_t _size) { sizeData = _size; }

		TypeMsg Type() const { return type; }
		StatusMsg Status() const { return status; }

		void SetType(TypeMsg _type) { type = _type; }
		void SetStatus(StatusMsg _status) { status = _status; }
	};

	struct IBody {
	protected:
		std::vector<std::uint8_t> data;

	public:
		const uint32_t Size() const { return data.size(); }
		const bool Empty() const { return data.empty(); }
		std::vector<std::uint8_t>& Data() { return data; }
		void Clear() { data.clear(); }
	};

	template<typename TypeHeader,
			 typename TypeBody>
	struct IMessage {
	protected:
		TypeHeader header{};
		TypeBody body{};
	public:
		static constexpr size_t headerSize = sizeof(TypeHeader);

		TypeHeader& Header() { return header; }
		TypeBody& Body() { return body; }

		const uint32_t HSize() const { return header.Size(); }
		const uint32_t BSize() const { return body.Size(); }
	};

	template<typename MessageIMPL>
	class Connection;

	template<typename MessageIMPL>
	struct 	OwnerMessage {
		std::shared_ptr<Connection<MessageIMPL>> remoteConnection = nullptr;
		MessageIMPL remoteMsg;

		OwnerMessage(std::shared_ptr<Connection<MessageIMPL>> _con, MessageIMPL _msg) : remoteConnection{ _con }, remoteMsg{ _msg } {}
	};

	template<typename MessageIMPL>
	using OWN_MSG_PTR = std::shared_ptr<OwnerMessage<MessageIMPL>>;

}