#pragma once
#include "../AuroraUtility/AuroraDefine.h"
#include "Includes.h"

namespace Aurora
{
	namespace Network
	{
#pragma pack(push, 1)
		class BaseSocket
		{
		public:
			BaseSocket( void );
			BaseSocket( const UInt16& sendBytes, const UInt16& recvBytes, const SOCKET& socket );

			virtual ~BaseSocket( void );

			void Reset( void );
			void Set( const UInt16& sendBytes, const UInt16& recvBytes, const SOCKET& socket );
			
			void SetSendBytes( const UInt16 GetSendBytes );
			void SetRecvBytes( const UInt16 GetRecvBytes );

			inline const UInt16 GetSendBytes( void ) const { return _sendBytes; }
			inline const UInt16 GetRecvBytes( void ) const { return _recvBytes; }
			inline const SOCKET GetSocket( void ) const { return _socket; }
		private:
			UInt16 _sendBytes;
			UInt16 _recvBytes;
			SOCKET _socket;
		};
	#pragma pack(pop)
	}
}