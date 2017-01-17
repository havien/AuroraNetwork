#pragma once
#include "NetworkManager.h"
#include "../AuroraUtility/MiscManager.h"
#include "../AuroraUtility/StringManager.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace Aurora;
using namespace Aurora::Network;

NetworkManager::NetworkManager( void ) : 
_initialized( false ),
_runningMode( ENetworkRunMode::Max ),
_clientSocket( INVALID_SOCKET ),
_listenSocket( INVALID_SOCKET ),
_isRunningServer( false ),
_serverPort( 0 )
{
 	AuroraStringManager->Clear( &_serverIP, MAX_IPV4_IP_LEN );
}

NetworkManager::~NetworkManager( void )
{
	if( true == _initialized )
	{
		if( ENetworkRunMode::Client == _runningMode )
		{
			CloseSocket( &_clientSocket );
		}
		else if( ENetworkRunMode::Server == _runningMode )
		{
			CloseSocket( &_listenSocket );
		}
	}

	_initialized = false;
}

void NetworkManager::DetermineIPAddress( const sockaddr_in& addr, OUT char* pIP, bool isIPV6 )
{
	if( nullptr == pIP )
	{
		return;
	}

	if( true == isIPV6 )
	{
		inet_ntop( AF_INET6, (PVOID)(&addr.sin_addr), OUT pIP, sizeof( char ) * MAX_IPV4_IP_LEN );
	}
	else
	{
		inet_ntop( AF_INET, (PVOID)(&addr.sin_addr), OUT pIP, sizeof( char ) * MAX_IPV4_IP_LEN );
	}
}

const UInt16 NetworkManager::GetPort( const sockaddr_in& addr )
{
	return ntohs( addr.sin_port );
}

bool NetworkManager::InitClientNetwork( const char* pServerIP, const UInt16 ServerPort )
{
	if( pServerIP )
	{
		if( ENetworkRunMode::Server == _runningMode )
		{
			return false;
		}

		AuroraStringManager->Copy( pServerIP, _serverIP, MAX_IPV4_IP_LEN );
		_serverPort = ServerPort;

		_clientSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( INVALID_SOCKET == _clientSocket )
		{
			//PRINT_NORMAL_LOG( eLogPrintType_Normal, L"socket failed with error : %ld\n", WSAGetLastError() );
			PRINT_NORMAL_LOG( L"socket failed with error : %ld\n", WSAGetLastError() );
			CloseSocket( &_clientSocket );
			WSACleanup();
			return false;
		}

		_initialized = true;
		return true;
	}

	return false;
}

bool NetworkManager::CloseSocket( SOCKET* pSocket )
{
	if( nullptr == pSocket )
	{
		return false;
	}

	shutdown( (*pSocket), SD_BOTH );
	closesocket((*pSocket));

	(*pSocket) = INVALID_SOCKET;
	return true;
}

bool NetworkManager::ConnectToServer( void )
{
	if( true == _initialized && INVALID_SOCKET != _clientSocket )
	{
		_clientAddr.sin_family = AF_INET;
		inet_pton( AF_INET, _serverIP, (PVOID *)(&_clientAddr.sin_addr.s_addr) );
		_clientAddr.sin_port = htons( _serverPort );

		auto connectResult = connect( _clientSocket, reinterpret_cast<SOCKADDR*>(&_clientAddr), sizeof( _clientAddr ) );
		if( SOCKET_ERROR == connectResult )
		{
			PRINT_NORMAL_LOG( L"[ConnectToServer] Failed to Connect Server..![Reason : %d]\n",
							  WSAGetLastError() );

			CloseSocket( &_clientSocket );
			return false;
		}

		return true;
	}

	return false;
}

bool NetworkManager::SendToServer( const char* pSendBuffer, const UInt16 &wishSendBytes, OUT Int32 &sendBytes )
{
	if( nullptr == pSendBuffer )
	{
		return false;
	}

	
	auto sendResult = send( _clientSocket, pSendBuffer, wishSendBytes, 0 );
	if( SOCKET_ERROR == sendResult )
	{
		CloseSocket( &_clientSocket );
		sendBytes = -1;

		PRINT_NORMAL_LOG( L"[SendToServer] Send failed with error: %d\n", WSAGetLastError() );
		return false;
	}

	sendBytes = sendResult;
	PRINT_NORMAL_LOG( L"[SendToServer] Success to Send %d Bytes\n", sendBytes );
	return true;
}

bool NetworkManager::ReceiveFromServer( char* pReceiveBuffer, const UInt16 &wishReceiveBytes, 
										OUT Int32 &receivedBytes, EDataReceiveMode receiveMode )
{
	if( nullptr == pReceiveBuffer )
	{
		return false;
	}

	if( pReceiveBuffer )
	{
		static Int32 resultVal = -1;

		if( EDataReceiveMode::WaitCompleteReceive == receiveMode )
		{
			resultVal = recvn( _clientSocket, pReceiveBuffer, wishReceiveBytes, 0 );
		}
		else if( EDataReceiveMode::Normal == receiveMode )
		{
			resultVal = recv( _clientSocket, pReceiveBuffer, wishReceiveBytes, 0 );
		}

		if( 0 == resultVal )
		{
			// Connection Closed.
			receivedBytes = -1;
			PRINT_NORMAL_LOG( L"[ReceiveFromServer] Recv failed with Closed Connection : %d\n", WSAGetLastError() );
			return false;
		}
		else if( 0 < resultVal )
		{
			// Received Data.
			PRINT_NORMAL_LOG( L"[ReceiveFromServer] Success! Received %d Bytes\n", resultVal );
			receivedBytes = resultVal;
			return true;
		}
		else
		{
			// occurred Another Error.
			receivedBytes = -1;
			PRINT_NORMAL_LOG( L"[ReceiveFromServer] Occurred Error! [%d]\n", WSAGetLastError() );
			return false;
		}
	}

	return false;
}

Int32 NetworkManager::recvn( SOCKET &socket, char* pBuffer, Int32 recvLength, Int32 flags )
{
	char* pRecvData = pBuffer;
	Int32 curReceivedResult = 0;
	Int32 leftRecvBytes = recvLength;

	while( 0 < leftRecvBytes )
	{
		curReceivedResult = recv( socket, pRecvData, leftRecvBytes, flags );

		if( SOCKET_ERROR == curReceivedResult )
		{
			PRINT_NORMAL_LOG( L"[recvn] LAST ERROR : %d", WSAGetLastError() );
			return SOCKET_ERROR;
		}
		else if( 0 == curReceivedResult )
		{
			break;
		}

		leftRecvBytes -= curReceivedResult;
		pRecvData += curReceivedResult;
	}

	return (recvLength - leftRecvBytes);
}

/////////////////////
// Server.
/////////////////////
bool NetworkManager::InitServerNetwork( const UInt16 ServerPort )
{
	if( ENetworkRunMode::Server != _runningMode )
	{
		return false;
	}

	// Initialize listening socket
	_listenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( INVALID_SOCKET == _listenSocket )
	{
		//printf("socket() failed with error %d\n", WSAGetLastError());
		return false;
	}

	_isRunningServer = false;

	_initialized = true;
	_serverPort = ServerPort;

	return true;
}

bool NetworkManager::StartServerNetwork( void )
{
	if( true == _initialized && ENetworkRunMode::Server == _runningMode )
	{
		_listenAddr.sin_family = AF_INET;
		_listenAddr.sin_addr.s_addr = htonl( INADDR_ANY );
		_listenAddr.sin_port = htons( _serverPort );

		Int32 SocketResult = bind( _listenSocket, (SOCKADDR *)&_listenAddr, sizeof(_listenAddr) );
		if( SOCKET_ERROR == SocketResult )
		{
			//AuroraUtility::CMiscManager::Instance( )->PrintDebugTextToOutputWindow( eLogPrintType_Normal, L"bind() failed with error %d\n", WSAGetLastError( ) );
			PRINT_NORMAL_LOG( L"bind() failed with error %d\n", WSAGetLastError() );
			return false;
		}

		//----------------------
		// Listen for incoming connection requests 
		// on the created socket
		SocketResult = listen( _listenSocket, SOMAXCONN );
		if( SOCKET_ERROR == SocketResult )
		{
			//AuroraUtility::CMiscManager::Instance( )->PrintDebugTextToOutputWindow( eLogPrintType_Normal, L"Error listening on socket.\n" );
			PRINT_NORMAL_LOG( L"Error listening on socket.\n" );
			return false;
		}

		_isRunningServer = true;
		return true;
	}

	return false;
}

bool NetworkManager::AcceptClient( void )
{
	if( ENetworkRunMode::Server != _runningMode )
	{
		return false;
	}

	while( true == _isRunningServer )
	{
		if( true != _initialized )
		{
			return false;
		}

		Int32 ClientAddrLen = sizeof(_clientAddr);
		SOCKET ClientSocket = accept( _listenSocket, (SOCKADDR *)&_clientAddr, &ClientAddrLen );
		if( INVALID_SOCKET == ClientSocket )
		{
			//AuroraUtility::CMiscManager::Instance( )->PrintDebugTextToOutputWindow( eLogPrintType_Normal, L"accept() failed with error %d\n", WSAGetLastError( ) );
			PRINT_NORMAL_LOG( L"accept() failed with error %d\n", WSAGetLastError() );
			continue;
		}
	}

	return true;
}

bool NetworkManager::SendToClient( const SOCKET &ClientSocket, const char* pSendBuffer, const UInt16 &WishSendingBytes, UInt16 &SendedBytes )
{
	if( ENetworkRunMode::Client  != _runningMode )
	{
		return false;
	}

	if( 0 >= WishSendingBytes )
	{
		//PRINT_NORMAL_LOG( eLogPrintType_Normal, L"[SendToClient] lower to 0(zero) SendingBytes : %d\n", wishSendBytes );
		PRINT_NORMAL_LOG( L"[SendToClient] lower to 0(zero) SendingBytes : %d\n", WishSendingBytes );
		return false;
	}

	if( pSendBuffer && (0 < strlen( pSendBuffer )) )
	{
		Int32 iResult = send( ClientSocket, pSendBuffer, WishSendingBytes, 0 );
		if( SOCKET_ERROR == iResult )
		{
			//PRINT_NORMAL_LOG( eLogPrintType_Normal, L"[SendToClient] Send failed with error: %d\n", WSAGetLastError() );
			PRINT_NORMAL_LOG( L"[SendToClient] Send failed with error: %d\n", WSAGetLastError() );
			//CloseSocket( ClientSocket );
			SendedBytes = 0;
			return false;
		}

		SendedBytes = static_cast<UInt16>(iResult);
		return true;
	}

	return false;
}

bool NetworkManager::RecvFromClient( const SOCKET &ClientSocket, char* pReceiveBuffer, const UInt16 &WishReceiveBytes, UInt16 &ReceivedBytes )
{
	if( ENetworkRunMode::Server != _runningMode )
	{
		return false;
	}

	if( 0 >= WishReceiveBytes )
	{
		//PRINT_NORMAL_LOG( eLogPrintType_Normal, L"[RecvFromClient] lower to 0(zero) SendingBytes : %d\n", wishReceiveBytes );
		PRINT_NORMAL_LOG( L"[RecvFromClient] lower to 0(zero) SendingBytes : %d\n", WishReceiveBytes );
		return false;
	}

	if( pReceiveBuffer )
	{
		Int32 iResult = recv( ClientSocket, pReceiveBuffer, WishReceiveBytes, 0 );
		if( SOCKET_ERROR == iResult )
		{
			//PRINT_NORMAL_LOG( eLogPrintType_Normal, L"[RecvFromClient] Recv failed with error: %d\n", WSAGetLastError() );
			PRINT_NORMAL_LOG( L"[RecvFromClient] Recv failed with error: %d\n", WSAGetLastError() );
			return false;
		}

		ReceivedBytes = static_cast<UInt16>(iResult);
		return true;
	}

	return false;
}

bool NetworkManager::BroadcastToClient( const UInt16 ClientCount, BaseSocket *pClientSocketArray, const char* pSendBuffer,
											const UInt16 &WishSendingBytes, UInt16 &SendedBytes )
{
	if( ENetworkRunMode::Server != _runningMode )
	{
		return false;
	}

	if( 0 >= WishSendingBytes )
	{
		//PRINT_NORMAL_LOG( eLogPrintType_Normal, L"[BroadcastToClient] lower to 0(zero) SendingBytes : %d\n", wishSendBytes );
		PRINT_NORMAL_LOG( L"[BroadcastToClient] lower to 0(zero) SendingBytes : %d\n", WishSendingBytes );
		return false;
	}

	if( pClientSocketArray && pSendBuffer&& ( 0 < strlen( pSendBuffer ) ) )
	{
		bool TrySend = false;
		for ( auto Current = 0; Current < ClientCount; ++Current )
		{
			SOCKET CurSocket = pClientSocketArray[Current].GetSocket();
			Int32 sendResult = send( CurSocket, pSendBuffer, WishSendingBytes, 0 );
			if( SOCKET_ERROR == sendResult )
			{
				//PRINT_NORMAL_LOG( eLogPrintType_Normal, L"[BroadcastToClient] Send failed with error: %d\n", WSAGetLastError() );
				PRINT_NORMAL_LOG( L"[BroadcastToClient] Send failed with error: %d\n", WSAGetLastError() );
				SendedBytes = 0;
				return false;
			}

			SendedBytes = static_cast<UInt16>(sendResult);
			TrySend = true;
		}

		return TrySend;
	}

	return false;
}

const SOCKET* NetworkManager::GetServerListenSocket( void )
{
	if( ENetworkRunMode::Server != _runningMode )
	{
		return nullptr;
	}

	if( _initialized && (INVALID_SOCKET != _listenSocket) )
	{
		return &_listenSocket;
	}

	return nullptr;
}

void NetworkManager::ForceCloseSocket( SOCKET GetSocket )
{
	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if( SOCKET_ERROR == setsockopt( GetSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof( LINGER ) ) )
	{
		PRINT_NORMAL_LOG( L"[ForceCloseSocket] setsockopt linger option error: %d\n", GetLastError() );
		return;
	}

	closesocket( GetSocket );
}