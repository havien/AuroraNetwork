#pragma once

#include "../Utility/LogManager.h"
#include "../Utility/ThreadManager.h"

#include "AuroraEventSelectClient.h"
#include "AuroraEventSelectServer.h"
#include "WinsockManager.h"

using namespace Aurora;
using namespace Aurora::Network;

// WinsockManager.
WinsockManager::WinsockManager() :
	_initiialized( false ),
	_useEventSelect( true ),
	_lpAcceptEx( NULL ),
	_lpAcceptExSockAddrs( NULL ),
	_lpDisconnectEx( NULL ),
	_runningMode( ENetworkRunMode::Max ),
	_eventSelectSocket( INVALID_SOCKET )
{
}

WinsockManager::~WinsockManager( void )
{
	SafeDelete( _pEventSelectServer );
	SafeDelete( _pEventSelectClient );

	Cleanup();
}

bool WinsockManager::InitWinsock() noexcept
{
	auto Result = WSAStartup( MAKEWORD( 2, 2 ), &_WSAData );
	if( 0 != Result )
	{
		AuroraLogManager->Critical( L"WSAStartup() failed with error %d", WSAGetLastError() );
		return false;
	}

	_initiialized = true;
	return true;
}

void WinsockManager::Cleanup() noexcept
{
	WSACleanup();
}

bool WinsockManager::LoadAcceptExFunc( const SOCKET listenSocket ) noexcept
{
	if( INVALID_SOCKET != listenSocket )
	{
		DWORD dwBytes = 0;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;

		auto socketResult = WSAIoctl( listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
									  &GuidAcceptEx, sizeof( GuidAcceptEx ),
									  &_lpAcceptEx, sizeof( _lpAcceptEx ),
									  &dwBytes, NULL, NULL );

		if( SOCKET_ERROR == socketResult )
		{
			AuroraLogManager->Error( L"[IOCPWorker::InitIOCPServer] WSAIoctl GuidAcceptEx failed with error: %d",
									 WSAGetLastError() );

			closesocket( listenSocket );
			Cleanup();
			return false;
		}

		return true;
	}

	return false;
}

bool WinsockManager::LoadAcceptExSockaddrFunc( const SOCKET listenSocket ) noexcept
{
	if( INVALID_SOCKET != listenSocket )
	{
		DWORD dwBytes = 0;
		GUID GuidAcceptExSockaddr = WSAID_GETACCEPTEXSOCKADDRS;

		auto socketResult = WSAIoctl( listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
									  &GuidAcceptExSockaddr, sizeof( GuidAcceptExSockaddr ),
									  &_lpAcceptExSockAddrs, sizeof( _lpAcceptExSockAddrs ),
									  &dwBytes, NULL, NULL );

		if( SOCKET_ERROR == socketResult )
		{
			AuroraLogManager->Error( L"[IOCPWorker::InitIOCPServer] WSAIoctl GetAcceptExSockaddr failed with error: %d",
									 WSAGetLastError() );

			closesocket( listenSocket );
			Cleanup();
			return false;
		}

		return true;
	}

	return false;
}

bool WinsockManager::LoadDisconnectExFunc( const SOCKET listenSocket ) noexcept
{
	if( INVALID_SOCKET != listenSocket )
	{
		GUID guidDisconnectEx = WSAID_DISCONNECTEX;
		DWORD dwBytes = 0;
		auto socketResult = WSAIoctl( listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
									  &guidDisconnectEx, sizeof( guidDisconnectEx ),
									  &_lpDisconnectEx, sizeof( _lpDisconnectEx ),
									  &dwBytes, NULL, NULL );


		if( SOCKET_ERROR == socketResult )
		{
			AuroraLogManager->Error( L"[IOCPWorker::LoadMswsockDisconnectExFunc] WSAIoctl GuidDisconnectEx failed with error: %d",
									 WSAGetLastError() );

			closesocket( listenSocket );
			Cleanup();
			return false;
		}

		return true;
	}

	return false;
}

BOOL WinsockManager::GetAcceptEx( SOCKET sListenSocket,
								  SOCKET sAcceptSocket,
								  PVOID lpOutputBuffer,
								  DWORD dwReceiveDataLength,
								  DWORD dwLocalAddressLength,
								  DWORD dwRemoteAddressLength,
								  LPDWORD lpdwBytesReceived,
								  LPOVERLAPPED lpOverlapped ) noexcept
{
	return _lpAcceptEx( sListenSocket, sAcceptSocket, lpOutputBuffer,
						dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength,
						lpdwBytesReceived, lpOverlapped );
}

void WinsockManager::GetAcceptExSockaddr( PVOID lpOutputBuffer,
										  DWORD dwReceiveDataLength,
										  DWORD dwLocalAddressLength,
										  DWORD dwRemoteAddressLength,
										  OUT struct sockaddr **LocalSockaddr,
										  OUT LPINT LocalSockaddrLength,
										  OUT struct sockaddr **RemoteSockaddr,
										  OUT LPINT RemoteSockaddrLength ) noexcept
{
	_lpAcceptExSockAddrs( lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength,
						  LocalSockaddr, LocalSockaddrLength, RemoteSockaddr, RemoteSockaddrLength );
}

BOOL WinsockManager::GetDisconnectEx( SOCKET s,
									  OUT LPOVERLAPPED lpOverlapped,
									  DWORD dwFlags,
									  DWORD dwReserved ) noexcept
{
	return _lpDisconnectEx( s, lpOverlapped, dwFlags, dwReserved );
}

void WinsockManager::SetEchoMode( bool EchoMode )
{
	if( ENetworkRunMode::Client == _runningMode )
	{
		_pEventSelectClient->SetEchoMode( EchoMode );
	}
}

bool WinsockManager::StartEventSelectEchoClient( SOCKET* pSocket, bool isEchoMode )
{
	if( pSocket )
	{
		if( true == _useEventSelect )
		{
			_eventSelectSocket = (*pSocket);
			_pEventSelectClient = new EventSelectClient( &_eventSelectSocket );
			_pEventSelectClient->SetEchoMode( isEchoMode );

			HANDLE CommunicationHandle = AuroraThreadManager->BeginThread( _pEventSelectClient->CommunicationWithServer,
																		   _pEventSelectClient,
																		   nullptr );

			WaitForSingleObject( CommunicationHandle, INFINITE );
		}

		return true;
	}

	return false;
}

bool WinsockManager::StartEventSelectClient( SOCKET* pSocket, CQueue<char*>* pRecvBufferQueue, HANDLE*TargetWakeupThread )
{
	if( pSocket )
	{
		if( true == _useEventSelect )
		{
			_eventSelectSocket = (*pSocket);
			_pEventSelectClient = new EventSelectClient( &_eventSelectSocket );
			if( INVALID_HANDLE_VALUE != TargetWakeupThread )
			{
				_pEventSelectClient->_targetWakeupThread = TargetWakeupThread;
			}

			_pEventSelectClient->SetRecvBufferQueue( pRecvBufferQueue );

			HANDLE CommunicationHandle = AuroraThreadManager->BeginThread( _pEventSelectClient->CommunicationWithServer,
																		   _pEventSelectClient,
																		   nullptr );

			WaitForSingleObject( CommunicationHandle, INFINITE );
			CloseHandle( CommunicationHandle );
		}

		return true;
	}

	return false;
}

void WinsockManager::StopEventSelectClient( void )
{
	if( ENetworkRunMode::Client == _runningMode )
	{
		_pEventSelectClient->StopCommunicationThread();
	}
}

bool WinsockManager::StartEventSelectServer( const SOCKET* pSocket, CQueue<char*>* pRecvBufferQueue, HANDLE TargetWakeupThread, bool EchoMode )
{
	if( pSocket && pRecvBufferQueue )
	{
		if( true == _useEventSelect )
		{
			_eventSelectSocket = (*pSocket);
			_pEventSelectServer = new EventSelectServer( &_eventSelectSocket );
			_pEventSelectServer->SetEchoMode( EchoMode );

			if( INVALID_HANDLE_VALUE != TargetWakeupThread )
			{
				_pEventSelectServer->_targetWakeupThread = TargetWakeupThread;
			}

			_pEventSelectServer->SetRecvBufferQueue( pRecvBufferQueue );

			HANDLE AcceptThreadHandle = AuroraThreadManager->BeginThread( _pEventSelectServer->AcceptClientUseEventSelect, 
																		  _pEventSelectServer, 
																		  nullptr );
			Sleep( 1000 );
			HANDLE ReadWriteThreadHandle = AuroraThreadManager->BeginThread( _pEventSelectServer->ReadWriteDataUseEventSelect,
																			 _pEventSelectServer, 
																			 nullptr );

			WaitForSingleObject( ReadWriteThreadHandle, INFINITE );
			WaitForSingleObject( AcceptThreadHandle, INFINITE );

			CloseHandle( ReadWriteThreadHandle );
			CloseHandle( AcceptThreadHandle );
		}

		return true;
	}

	return false;
}

char* WinsockManager::GetFrontReceivedPacket( void )
{
	return _pEventSelectClient->GetFrontReceivedPacket();
}