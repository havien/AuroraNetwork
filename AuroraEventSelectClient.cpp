#pragma once

#include "../AuroraUtility/MiscManager.h"
#include "../AuroraUtility/StringManager.h"

#include "NetworkManager.h"
#include "WinsockManager.h"

#include "AuroraEventSelectClient.h"

using namespace Aurora;
using namespace Aurora::Network;

EventSelect::EventSelect( const SOCKET* pSocket, ENetworkRunMode RunMode )
{
	bool result = StartEventSelect( pSocket, RunMode );
	if( false == result )
	{
		// critical error.
		return;
	}
}

EventSelect::~EventSelect( void )
{

}

bool EventSelect::StartEventSelect( const SOCKET *pSocket, ENetworkRunMode RunMode )
{
	if( pSocket )
	{
		_eventObject = WSACreateEvent();
		Int32 SocketResult = -1;

		if( ENetworkRunMode::Server == RunMode )
		{
			SocketResult = WSAEventSelect( (*pSocket), _eventObject, FD_ACCEPT | FD_CLOSE );
		}
		else if( ENetworkRunMode::Client == RunMode )
		{
			SocketResult = WSAEventSelect( (*pSocket), _eventObject, FD_CONNECT | FD_READ | FD_CLOSE );
		}
		else
		{
			return false;
		}

		if( SOCKET_ERROR == SocketResult )
		{
			printf( "[StartEventSelect] WSAEventSelect() failed with error %d\n", WSAGetLastError() );
			return false;
		}

		_eventSocket = (*pSocket);
		return true;
	}

	return false;
}

EventSelectClient::EventSelectClient( const SOCKET* pSocket ) :
_echoMode( false ),
_saveReceivedPacket( true ),
_pEventSelect( nullptr ),
_runningCommunicationThread( true ),
_readWriteWaitHandle( INVALID_HANDLE_VALUE )
{
	if( nullptr == pSocket )
	{
		return;
	}

	_readWriteWaitHandle = CreateEvent( NULL, TRUE, FALSE, NULL );
	if( NULL == _readWriteWaitHandle )
	{
		// failed to create event.
		return;
	}

	_pEventSelect = new EventSelect( pSocket, ENetworkRunMode::Client );
	_eventObject = _pEventSelect->_eventObject;
}

EventSelectClient::~EventSelectClient( void )
{

}

UInt32 __stdcall EventSelectClient::CommunicationWithServer( void* pArgs )
{
	if( nullptr == pArgs )
	{
		return 0;
	}

	EventSelectClient* pThis = reinterpret_cast<EventSelectClient*>(pArgs);
	//AuroraUtility::CMiscManager *pWinUtilManager = AuroraUtility::CMiscManager::Instance();

	/*PRINT_NORMAL_LOG( L"START CommunicationWithServer Thread..!\n" );*/
		
	if( true == pThis->GetEchoMode() )
	{
		Int32 asciiupperstart = 65;
		Int32 asciiupperend = 90;

		Int32 asciilowerstart = 97;
		Int32 asciilowerend = 122;

		char sendBuffer[SUPER_BUFFER_LEN] = {0};
		AuroraStringManager->Clear( sendBuffer, SUPER_BUFFER_LEN );

		srand( (unsigned int)time( NULL ) );
		for ( UInt16 Current = 0; Current < (SUPER_BUFFER_LEN-1); ++Current )
		{
			Int32 ascii = rand() % ( asciilowerend - asciilowerstart ) + asciilowerstart;
			Int32 ascii2 = rand() % ( asciiupperend - asciiupperstart ) + asciiupperstart;

			Int32 randnum = ascii;
			if( ( Current % 2 ) == 0 )
			{
				randnum = ascii2;
			}

			sendBuffer[Current] = static_cast<char>( randnum );
		}

		Int32 sendBytes = 0;
		auto wishSendBytes = static_cast<UInt16>( strlen( sendBuffer ) );
		bool sendResult = AuroraNetworkManager->SendToServer( sendBuffer, wishSendBytes, OUT sendBytes );
		if( true == sendResult && 0 < sendBytes )
		{
			//PRINT_NORMAL_LOG( L"Success to Send %ld Bytes, [%S]\n", sendBytes, sendBuffer );
		}
	}

	WSANETWORKEVENTS networkEvents;
	while( true == pThis->_runningCommunicationThread )
	{
		auto Index = WaitForSingleObject( pThis->_pEventSelect->_eventObject, WSA_INFINITE );
		if( WSA_WAIT_FAILED == Index )
		{
			//err_display("WSAWaitForMultipleEvents()");
			continue;
		}

		/*PRINT_NORMAL_LOG( L"Wake UP [CommunicationWithServer Thread]\n" );*/
		Index -= WSA_WAIT_EVENT_0;

		Int32 SocketResult = WSAEnumNetworkEvents( pThis->_pEventSelect->_eventSocket, 
												   pThis->_pEventSelect->_eventObject, 
												   &networkEvents );
		if( SOCKET_ERROR == SocketResult )
		{
			WSAResetEvent( pThis->_pEventSelect->_eventObject );
			PRINT_NORMAL_LOG( L"[CommunicationWithServer Thread] WSAEnumNetworkEvents() failed with error %d\n", WSAGetLastError() );
			continue;
		}

		// Connect.
		if( FD_CONNECT & networkEvents.lNetworkEvents )
		{
			if( networkEvents.iErrorCode[FD_CONNECT_BIT] != 0 )
			{
				PRINT_NORMAL_LOG( L"FD_CONNECT failed with error %d\n", networkEvents.iErrorCode[FD_CONNECT_BIT] );
				break;
			}

			/*PRINT_NORMAL_LOG( L"FD_CONNECT\n" );*/
		}

		// Recv.
		if( FD_READ & networkEvents.lNetworkEvents )
		{
			if( networkEvents.iErrorCode[FD_READ_BIT] != 0 )
			{
				PRINT_NORMAL_LOG( L"FD_READ failed with error %d\n", networkEvents.iErrorCode[FD_READ_BIT] );
				break;
			}

			char recvBuffer[SUPER_BUFFER_LEN] = {0};
			if( true == pThis->_echoMode )
			{
				Int32 ReceivedBytes = 0;
				bool receiveResult = AuroraNetworkManager->ReceiveFromServer( recvBuffer, 
																			(SUPER_BUFFER_LEN - 1),
																			  ReceivedBytes, 
																			  EDataReceiveMode::Normal );

				if( true == receiveResult )
				{
					PRINT_NORMAL_LOG( L"[Received Data : %S]\n", recvBuffer );

					Int32 asciiupperstart = 65;
					Int32 asciiupperend = 90;

					Int32 asciilowerstart = 97;
					Int32 asciilowerend = 122;
					
					AuroraStringManager->Clear( recvBuffer, SUPER_BUFFER_LEN );

					for ( UInt16 Current = 0; Current< (SUPER_BUFFER_LEN-1); ++Current )
					{
						Int32 ascii = rand( ) % ( asciilowerend - asciilowerstart ) + asciilowerstart;
						Int32 ascii2 = rand( ) % ( asciiupperend - asciiupperstart ) + asciiupperstart;

						auto randnum = ascii;
						if( Current % 2 == 0 )
						{
							randnum = ascii2;
						}

						recvBuffer[Current] = static_cast<char>( randnum );
					}

					Int32 sendBytes = 0;
					AuroraNetworkManager->SendToServer( recvBuffer, static_cast<UInt16>(strlen( recvBuffer )), OUT sendBytes );
				}
			}
			else
			{				
				Int32 ReceivedBytes = 0;
				bool receiveResult = AuroraNetworkManager->ReceiveFromServer( recvBuffer, 
																		   (SUPER_BUFFER_LEN-1), 
																		   ReceivedBytes, 
																		   EDataReceiveMode::Normal );
				if( true == receiveResult )
				{
					if( true == pThis->SaveReceivedPacketMode() )
					{
						// Push to Received Packet List. 
						char* pReceved = new char[ReceivedBytes];
						std::memcpy( pReceved, recvBuffer, ReceivedBytes );
						pThis->_receivedPacketList.push_back( pReceved );
					}

					pThis->AddReceivedPacket( recvBuffer );

					/*char* pTemp = WinsockManager::Instance()->GetFrontReceivedPacket();
					CBasePacket *pPacket = reinterpret_cast<CBasePacket *>( pTemp );*/
				}					
			}
		}

		// Disconnect.
		if( FD_CLOSE & networkEvents.lNetworkEvents )
		{
			PRINT_NORMAL_LOG( L"[FD_CLOSE]\n" );
			AuroraNetworkManager->ForceCloseSocket( pThis->_pEventSelect->_eventSocket );

			if( networkEvents.iErrorCode[FD_CLOSE_BIT] != 0 )
			{
				PRINT_NORMAL_LOG( L"FD_CLOSE failed with error %d\n", networkEvents.iErrorCode[FD_CLOSE_BIT] );
				break;
			}

			return 0;
		}

		Sleep( 1 );
	}

	return 0;
}

char* EventSelectClient::GetFrontReceivedPacket( void )
{
	if( 0 < _receivedPacketList.size() )
	{
		auto pPacket = _receivedPacketList.front();
		_receivedPacketList.erase( _receivedPacketList.begin() );
		return pPacket;
	}
		
	return nullptr;
}

void EventSelectClient::AddReceivedPacket( char* pPacket )
{
	if( pPacket )
	{ 
		_pRecvBufferQueue->Enqueue( pPacket );
	}

	SetEvent( ( *_targetWakeupThread ) );
}