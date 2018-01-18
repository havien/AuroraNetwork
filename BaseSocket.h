#pragma once
#include "../Utility/AuroraDefine.h"
#include "Includes.h"

namespace Aurora
{
	namespace Network
	{
#pragma pack(push, 1)
		class BaseSocket
		{
		public:
			BaseSocket();
			BaseSocket( const UInt16& sendBytes, const UInt16& recvBytes, const SOCKET& socket );

			virtual ~BaseSocket();

			void Reset();
			void Set( const UInt16& sendBytes, const UInt16& recvBytes, const SOCKET& socket );
			
			void SetSendBytes( const UInt16 GetSendBytes );
			void SetRecvBytes( const UInt16 GetRecvBytes );

			inline const UInt16 GetSendBytes() const { return _sendBytes; }
			inline const UInt16 GetRecvBytes() const { return _recvBytes; }
			inline const SOCKET GetSocket() const { return _socket; }
		private:
			UInt16 _sendBytes;
			UInt16 _recvBytes;
			SOCKET _socket;
		};
#pragma pack(pop)
	}
}