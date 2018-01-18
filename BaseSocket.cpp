#pragma once
#include "BaseSocket.h"

namespace Aurora
{
	namespace Network
	{
		BaseSocket::BaseSocket():
		_sendBytes( 0 ),
		_recvBytes( 0 ),
		_socket( INVALID_SOCKET )
		{
		}

		BaseSocket::BaseSocket( const UInt16 &sendBytes, const UInt16 &recvBytes, const SOCKET &socket ) :
		_sendBytes( sendBytes ),
		_recvBytes( recvBytes ),
		_socket( socket )
		{

		}

		BaseSocket::~BaseSocket( void )
		{
			Reset();
		}

		void BaseSocket::Reset( void )
		{
			_recvBytes = 0;
			_socket = INVALID_SOCKET;
		}

		void BaseSocket::Set( const UInt16& sendBytes, const UInt16& recvBytes, const SOCKET& socket )
		{
			_sendBytes = sendBytes;
			_recvBytes = recvBytes;
			_socket = socket;
		};

		void BaseSocket::SetSendBytes( const UInt16 GetSendBytes )
		{
			if( 0 > GetSendBytes )
			{
				return;
			}

			_sendBytes = GetSendBytes;
		}

		void BaseSocket::SetRecvBytes( const UInt16 GetRecvBytes )
		{
			if( 0 > GetRecvBytes )
			{
				return;
			}
			_recvBytes = GetRecvBytes;
		}
	}
}