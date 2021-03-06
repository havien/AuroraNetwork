#pragma once
#include "../Utility/AuroraDefine.h"

namespace Aurora
{
	namespace Network
	{
		enum class EProtocol : Int32
		{
			TCP = 6, // IPPROTO_TCP
			UDP = 17, // IPPROTO_UDP
			Max,
		};

		enum class ENetworkRunMode : Byte
		{
			Server = 0,
			Client,
			Max,
		};

		enum class EDataReceiveMode : Byte
		{
			Normal = 0,
			WaitCompleteReceive,
			Max,
		};

		enum class ECommonResult : Byte
		{
			OK = 0,
			Success,
			Error,
			NotFound,
			AlreadyExists,
			NotEnough,
			Max,
		};

		enum class EPacketOperation : UInt16
		{
			None = 0,
			RegisterReq = 175,
			RegisterAck,
			LoginReq = 226,
			LoginAck,
			LogoutReq,
			LogoutAck,
			ChatReq,
			ChatAck,
			WhisperChatReq,
			WhisperChatAck,
			NotifyEvent,
			Max,
		};

		const UInt16 PACKET_HEADER_SIZE = 8;
		const UInt16 MAX_CLIENT_EVENT_COUNT = 1;

		constexpr UInt16 MAX_IPV4_IP_LEN = 14 + 1;
		constexpr UInt16 MAX_IPV6_IP_LEN = 26 + 1;
		const UInt16 NORMAL_BUFFER_LEN = 512;
		const UInt16 BIG_BUFFER_LEN = (NORMAL_BUFFER_LEN << 1);
		const UInt16 BIGGER_BUFFER_LEN = (BIG_BUFFER_LEN << 2);

		const UInt16 MAX_BUFFER_LEN = BIGGER_BUFFER_LEN;

		const UInt16 MAX_PACKET_SIZE = 1024;
		const UInt16 MAX_RECEIVE_BUFFER_SIZE = 8192;

		// buffer size.
		const UInt16 SEND_BUFFER_QUEUE_SIZE = 256;
		const UInt16 CONCURRENT_PROCESS_PACKET_COUNT = 1024;
	}
}