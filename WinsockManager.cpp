#pragma once

#include "../AuroraUtility/MiscManager.h"
#include "../AuroraUtility/ThreadManager.h"

#include "AuroraEventSelectClient.h"
#include "AuroraEventSelectServer.h"
#include "AuroraIOCP.h"
#include "WinsockManager.h"

using namespace Aurora;
using namespace Aurora::Network;

// WinsockManager.
WinsockManager::WinsockManager( void ) :
_initiialized( false ),
_useEventSelect( true ),
_runningMode( ENetworkRunMode::Max ),
_eventSelectSocket( INVALID_SOCKET )
{
	InitWinsock();
}

WinsockManager::~WinsockManager( void )
{
	SAFE_DELETE( _pEventSelectServer );
	SAFE_DELETE( _pEventSelectClient );

	Cleanup();
}

bool WinsockManager::InitWinsock()
{
	Int32 Result = WSAStartup( MAKEWORD( 2, 2 ), &_WSAData );
	if( Result != 0 )
	{
		printf( "WSAStartup() failed with error %d\n", WSAGetLastError() );
		return false;
	}

	_initiialized = true;
	return true;
}

void WinsockManager::Cleanup( void )
{
	WSACleanup();
}

void WinsockManager::SetEchoMode( bool EchoMode )
{
	if( ENetworkRunMode::Client == _runningMode )
	{
		_pEventSelectClient->SetEchoMode( EchoMode );
	}
}

bool WinsockManager::StartEventSelectEchoClient( SOCKET *pSocket, bool isEchoMode )
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

bool WinsockManager::StartEventSelectClient( SOCKET *pSocket, CQueue<char*> *pRecvBufferQueue, HANDLE*TargetWakeupThread )
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

bool WinsockManager::StartEventSelectServer( const SOCKET *pSocket, CQueue<char*> *pRecvBufferQueue, HANDLE TargetWakeupThread, bool EchoMode )
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

			HANDLE AcceptThreadHandle = AuroraThreadManager->BeginThread( _pEventSelectServer->AcceptClientUseEventSelect, _pEventSelectServer, nullptr );
			Sleep( 1000 );
			HANDLE ReadWriteThreadHandle = AuroraThreadManager->BeginThread( _pEventSelectServer->ReadWriteDataUseEventSelect, _pEventSelectServer, nullptr );

			WaitForSingleObject( ReadWriteThreadHandle, INFINITE );
			WaitForSingleObject( AcceptThreadHandle, INFINITE );

			CloseHandle( ReadWriteThreadHandle );
			CloseHandle( AcceptThreadHandle );
		}

		return true;
	}

	return false;
}

bool WinsockManager::InitIOCP( const UInt16 workerThreadCount )
{
	return _IOCPWorker.InitIOCPServer( workerThreadCount );
}

void WinsockManager::StartIOCPWorker( CQueue<IOCPData*>* pPacketParserQueue, const HANDLE* eventHandle, 
									  const UInt16 workerThreadCount, const bool isEchoMode )
{
	if( pPacketParserQueue && eventHandle )
	{
		if( true == isEchoMode )
		{
			_IOCPWorker.SetEchoMode();
			PRINT_NORMAL_LOG( L"Set Echo Mode Flag, Now Running Echo Server..!\n" );
		}

		_IOCPWorker.SetParserQueue( pPacketParserQueue );
		_IOCPWorker.SetWaitHandle( eventHandle );

		HANDLE AcceptWorker = AuroraThreadManager->BeginThread( _IOCPWorker.AcceptWorker, &_IOCPWorker, nullptr );
		
		HANDLE* pGQCSHandle = new HANDLE[workerThreadCount];
		for( auto counter = 0; counter < workerThreadCount; ++counter )
		{
			pGQCSHandle[counter] = AuroraThreadManager->BeginThread( _IOCPWorker.GQCSWorker, &_IOCPWorker, nullptr );
		}

		PRINT_FILE_LOG( L"Success to IOCP Worker..!\n" );

		WaitForSingleObject( AcceptWorker, INFINITE );
		WaitForMultipleObjects( workerThreadCount, pGQCSHandle, TRUE, INFINITE );

		CloseHandle( AcceptWorker );

		for( Int32 counter = 0; counter < workerThreadCount; ++counter )
		{
			CloseHandle( pGQCSHandle[counter] );
		}
	}

	PRINT_FILE_LOG( L"Failed to IOCP Worker..!\n" );
}

void WinsockManager::RequestSendToIOCP( IOCPData* pIOCPData, size_t BufferLength )
{
	if( pIOCPData )
	{
		_IOCPWorker.EnqueueSendBufferAndRequestSend( pIOCPData, BufferLength );
	}
}

char* WinsockManager::GetFrontReceivedPacket( void )
{
	char* pBuffer = _pEventSelectClient->GetFrontReceivedPacket();
	return pBuffer;

	//return nullptr;
}