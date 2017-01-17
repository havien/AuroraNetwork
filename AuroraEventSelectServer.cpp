#pragma once

#include "../AuroraUtility/MiscManager.h"
#include "../AuroraUtility/StringManager.h"

#include "NetworkManager.h"
#include "WinsockManager.h"

#include "AuroraEventSelectClient.h"
#include "AuroraEventSelectServer.h"

using namespace Aurora;
using namespace Aurora::Network;
	

EventSelectServer::EventSelectServer( const SOCKET* pSocket ) :
_echoMode( false ),
_sendBufferQueue( SEND_BUFFER_QUEUE_SIZE ),
_pRecvBufferQueue( nullptr ),
_runningAcceptThread( true ),
_runningReadWriteThread( true ),
_targetWakeupThread( INVALID_HANDLE_VALUE )
{
	if( nullptr == pSocket )
	{
		return;
	}

	memset( &_clientData, 0, sizeof( EventSelectClientData ) );
	memset( &_acceptData, 0, sizeof( EventSelectClientData ) );

	_pEventSelect = new EventSelect( pSocket, ENetworkRunMode::Server );
	_eventObject = _pEventSelect->_eventObject;
	_eventSelectSocket = _pEventSelect->_eventSocket;

	Int32 ClientEventCount = 1;
	SetAcceptData( ClientEventCount, &_pEventSelect->_eventSocket, _pEventSelect->_eventObject );
	_veryFirstAccept = true;
}

EventSelectServer::~EventSelectServer( void )
{
	SAFE_DELETE( _pEventSelect );
}

UInt32 __stdcall EventSelectServer::AcceptClientUseEventSelect( void* pArgs )
{
	if( nullptr == pArgs )
	{
		return 0;
	}

	auto pThis = (EventSelectServer*)pArgs;
	WSANETWORKEVENTS networkEvents;
	auto pAcceptData = pThis->GetAcceptData();

	PRINT_NORMAL_LOG( L"START Accept Thread..!\n" );
	while( true == pThis->_runningAcceptThread )
	{
		// Wait for network Accept events on all sockets.
		auto Index = WSAWaitForMultipleEvents( pAcceptData->EventCount, pAcceptData->ClientEvents, FALSE, WSA_INFINITE, FALSE );
		if( WSA_WAIT_FAILED == Index )
		{
			//err_display("WSAWaitForMultipleEvents()");
			continue;
		}

		PRINT_NORMAL_LOG( L"Wake UP [Accept Thread]\n" );
		Index -= WSA_WAIT_EVENT_0;

		// Iterate through all events and enumerate
		// if the wait does not fail.
		for( Int32 counter = Index; counter < pAcceptData->EventCount; ++counter )
		{
			auto SocketResult = WSAEnumNetworkEvents( pAcceptData->ClientSockets[counter].GetSocket(),
													  pAcceptData->ClientEvents[counter],
													  &networkEvents );
			if( SOCKET_ERROR == SocketResult )
			{
				WSAResetEvent( pAcceptData->ClientEvents[counter] );
				PRINT_NORMAL_LOG( L"[Accept Thread] WSAEnumNetworkEvents() failed with error %d\n", WSAGetLastError() );
				return 1;
			}

			if( networkEvents.lNetworkEvents & FD_ACCEPT )
			{
				if( networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0 )
				{
					PRINT_NORMAL_LOG( L"FD_ACCEPT failed with error %d\n", networkEvents.iErrorCode[FD_ACCEPT_BIT] );
					break;
				}

				sockaddr_in clientAddr;
				Int32 addrLen = sizeof( clientAddr );
				SOCKET ClientSocket = accept( pAcceptData->ClientSockets[counter].GetSocket(),
											(SOCKADDR *)&clientAddr, 
											&addrLen );

				if( INVALID_SOCKET == ClientSocket )
				{
					PRINT_NORMAL_LOG( L"accept() failed with error %d\n", WSAGetLastError() );
					continue;
				}

				char ipAddress[MAX_IPV4_IP_LEN] = { 0 };
				AuroraNetworkManager->DetermineIPAddress( clientAddr, OUT ipAddress );

				PRINT_NORMAL_LOG( L"[Accept Thread] Connect to Client: IP=%S, Port=%d\n",
								  ipAddress, AuroraNetworkManager->GetPort( clientAddr ) );

				if( WSA_MAXIMUM_WAIT_EVENTS <= pAcceptData->EventCount )
				{
					PRINT_NORMAL_LOG( L"[Accept Thread] Maximum Wait Event Counts!!\n" );
					continue;
				}

				++pAcceptData->EventCount;

				Int32 ReadWriteEventCount = 0;
				if( true == pThis->IsVeryFirstAccept() )
				{
					pThis->EndFirstAccept();
					ReadWriteEventCount = pAcceptData->EventCount - 1;
				}
				else
				{
					ReadWriteEventCount = pAcceptData->EventCount;
				}

				// Completed Accept List.
				SOCKET curSocket = pAcceptData->ClientSockets[counter].GetSocket();
				pThis->SetAcceptData( ReadWriteEventCount, &curSocket, pAcceptData->ClientEvents[counter] );

				WSAEVENT m_AcceptEvent = WSACreateEvent();
				Int32 SelectResult = WSAEventSelect( ClientSocket, m_AcceptEvent, FD_READ | FD_WRITE | FD_CLOSE );
				if( SOCKET_ERROR == SelectResult )
				{
					PRINT_NORMAL_LOG( L"WSAEventSelect() failed with error %d\n", WSAGetLastError() );
					return 1;
				}

				// EventClientList for Read/Write.
				pThis->SetClientData( ReadWriteEventCount, ClientSocket, m_AcceptEvent );
			}

			if( networkEvents.lNetworkEvents & FD_CLOSE )
			{
				if( ( 0 != networkEvents.iErrorCode[FD_CLOSE_BIT] ) )
				{
					PRINT_NORMAL_LOG( L"[Accept Thread] FD_CLOSE failed with error %d\n",
									  networkEvents.iErrorCode[FD_CLOSE_BIT] );
				}

				PRINT_NORMAL_LOG( L"FD_CLOSE is OK!\n" );
			}
		}

		Sleep( 1 );
	}

	return 0;
}

UInt32 __stdcall EventSelectServer::ReadWriteDataUseEventSelect( void* pArgs )
{
	if( nullptr == pArgs )
	{
		return 0;
	}

	EventSelectServer* pThis = (EventSelectServer*)pArgs;

	// must be exists Shared Recv Buffer.
	if( nullptr == pThis->GetRecvBufferQueue() )
	{
		return 0;
	}

	auto pClientData = pThis->GetClientData();

	WSANETWORKEVENTS NetworkEvents;
	char tempBuffer[SUPER_BUFFER_LEN] = { 0 };

	PRINT_NORMAL_LOG( L"START Read/Write Thread..!\n" );
	while( true == pThis->_runningReadWriteThread )
	{
		// Wait for network Accept events on all sockets.
		DWORD Index = WSAWaitForMultipleEvents( pClientData->EventCount, pClientData->ClientEvents, FALSE, 1500, FALSE );
		if( WSA_WAIT_FAILED == Index )
		{
			//PRINT_NORMAL_LOG( L"WSAWaitForMultipleEvents() - WSA_WAIT_FAILED\n" );
			continue;
		}

		Index = Index - WSA_WAIT_EVENT_0;

		// Iterate through all events and enumerate
		// if the wait does not fail.
		for( Int32 counter = Index; counter < pClientData->EventCount; ++counter )
		{
			Index = WSAWaitForMultipleEvents( 1, &pClientData->ClientEvents[counter], TRUE, 1000, FALSE );
			if( (WSA_WAIT_FAILED != Index) && (WSA_WAIT_TIMEOUT != Index) )
			{
				Int32 eventResult = WSAEnumNetworkEvents( pClientData->ClientSockets[counter].GetSocket(),
														   pClientData->ClientEvents[counter],
														   &NetworkEvents );
				if( SOCKET_ERROR == eventResult )
				{
					WSAResetEvent( pClientData->ClientEvents[counter] );
					PRINT_NORMAL_LOG( L"[Read/Write Thread] WSAEnumNetworkEvents() failed with error %d\n", WSAGetLastError() );
					continue;
				}

				// recv(:4100), recvfrom(:4100), WSARecv(:4100), WSARecvFrom(:4100)
				if( FD_READ & NetworkEvents.lNetworkEvents || FD_WRITE & NetworkEvents.lNetworkEvents )
				{
					// 전송이 완료 되었을 때, Connect로 연결하고 소켓이 처음 연결되었을 때
					// Accept 호출 뒤에 소켓의 연결이 수락 되었을 때.
					// send 관련 함수를 호출하여 WOULDBLOCK이 리턴되었다가 전송 버퍼가 비워졌을 때.
					if( FD_READ & NetworkEvents.lNetworkEvents && (0 != NetworkEvents.iErrorCode[FD_READ_BIT]) )
					{
						PRINT_NORMAL_LOG( L"NetworkEvents.iErrorCode[FD_READ_BIT]\n" );
						continue;
					}

					// send(:4100), sendto(:4100), WSASend(:4100), WSASendTo(:4100)
					if( FD_WRITE & NetworkEvents.lNetworkEvents && (0 != NetworkEvents.iErrorCode[FD_WRITE_BIT]) )
					{
						PRINT_NORMAL_LOG( L"NetworkEvents.iErrorCode[FD_WRITE_BIT]\n" );
						continue;
					}

					if( false == pThis->GetEchoMode() && true == pClientData->FirstConnect[counter] )
					{
						PRINT_NORMAL_LOG( L"[First Connect!! ClientNumber :%d]\n", counter );
						pClientData->FirstConnect[counter] = false;
						continue;
					}

					AuroraStringManager->Clear( tempBuffer, SUPER_BUFFER_LEN );

					// Receive data.
					if( 0 == pClientData->ClientSockets[counter].GetRecvBytes() )
					{
						auto curSocket = pClientData->ClientSockets[counter].GetSocket();

						UInt16 ReceivedBytes = 0;
						bool recvResult = AuroraNetworkManager->RecvFromClient( curSocket, 
																				tempBuffer, 
																				(SUPER_BUFFER_LEN-1),
																				ReceivedBytes );
						if( false == recvResult )
						{
							if( WSAEWOULDBLOCK == WSAGetLastError() )
							{
								char* pWaitBuffer = new char[SUPER_BUFFER_LEN];
								AuroraStringManager->Clear( pWaitBuffer, SUPER_BUFFER_LEN );
								strcpy_s( pWaitBuffer, SUPER_BUFFER_LEN, tempBuffer );

								pThis->_waitforReceiveDatas.push_back( pWaitBuffer );
								PRINT_NORMAL_LOG( L"Recv WOULDBLOCK! just Continue..ClientNumber : %d\n", counter );
								continue;
							}
							else
							{
								WSAResetEvent( pClientData->ClientEvents[counter] );

								if( WSAECONNRESET == WSAGetLastError() )
								{
									pThis->RemoveClientData( pClientData, counter );
									pThis->DecreaseAcceptedClientCount();
									PRINT_NORMAL_LOG( L"Recv Error! Disconnect from Client! ClientNumber : %d\n", counter );
									continue;
								}

								PRINT_NORMAL_LOG( L"Recv Error! ClientNumber : %d, Message : %d\n", counter, WSAGetLastError() );
								break;
							}
							break;
						}

						// Client Disconnect by Gracefully closed.
						if( 0 == ReceivedBytes )
						{
							pThis->RemoveClientData( pClientData, counter );
							pThis->DecreaseAcceptedClientCount();
							PRINT_NORMAL_LOG( L"Recv End. Client Disconnect by Gracefully closed. ClientNumber : %d\n", counter );
							break;
						}

						pClientData->ClientSockets[counter].SetRecvBytes( 0 );

						UInt16 sendBytes = 0;
						sockaddr_in clientaddr;
						Int32 addrlen = sizeof( clientaddr );

						getpeername( pClientData->ClientSockets[counter].GetSocket(), (SOCKADDR *)&clientaddr, &addrlen );
						
						char ipAddress[MAX_IPV4_IP_LEN] = { 0 };
						AuroraNetworkManager->DetermineIPAddress( clientaddr, OUT ipAddress );

						PRINT_NORMAL_LOG( L"[TCP/%S:%d][Position:%d]\n", 
										  ipAddress, AuroraNetworkManager->GetPort( clientaddr ), counter );

						if( true == pThis->GetEchoMode() )
						{
							AuroraNetworkManager->SendToClient( curSocket, tempBuffer, ReceivedBytes, sendBytes );
						}

						// 큐에 데이터를 넣음.
						//pRecvBufferQueue->Push( tempBuffer );
						//
						//// 깨워야 할 스레드가 있다면 깨운다.
						//if( nullptr != pThis->_targetWakeupThread )
						//{
						//	SetEvent( pThis->_targetWakeupThread );
						//}
					}
				}

				if( FD_CLOSE & NetworkEvents.lNetworkEvents )
				{
					PRINT_NORMAL_LOG( L"[Send/Recv Thread] FD_CLOSE with %d\n", NetworkEvents.iErrorCode[FD_CLOSE_BIT] );

					WSAResetEvent( pClientData->ClientEvents[counter] );
					pThis->RemoveClientData( pClientData, counter );
					pThis->DecreaseAcceptedClientCount();
				}
			}
		}

		Sleep( 1 );
	}
	return 0;
}

void EventSelectServer::IncreaseReadWriteClientCount( void )
{
	if( _clientData.EventCount >= WSA_MAXIMUM_WAIT_EVENTS )
	{
		_clientData.EventCount = WSA_MAXIMUM_WAIT_EVENTS;
	}
	else
	{
		++_clientData.EventCount;
	}
}

void EventSelectServer::DecreaseReadWriteClientCount( void )
{
	if( 0 >= _clientData.EventCount )
	{
		_clientData.EventCount = 0;
	}
	else
	{
		++_clientData.EventCount;
	}
}

void EventSelectServer::SetClientData( const Int32 &EventCount, const SOCKET pSocket, const WSAEVENT rEvent )
{
	Int32 Position = (EventCount - 1);
	_clientData.FirstConnect[Position] = true;

	_clientData.EventCount = EventCount;
	_clientData.ClientEvents[Position] = rEvent;

	UInt16 SendRecvBytes = 0;
	_clientData.ClientSockets[Position].Set( SendRecvBytes, SendRecvBytes, pSocket );
}

void EventSelectServer::SetAcceptData( const Int32 &EventCount, const SOCKET *pSocket, const WSAEVENT rEvent )
{
	Int32 Position = (EventCount - 1);

	_acceptData.EventCount = EventCount;
	_acceptData.ClientEvents[Position] = rEvent;

	UInt16 SendRecvBytes = 0;
	_acceptData.ClientSockets[Position].Set( SendRecvBytes, SendRecvBytes, (*pSocket) );
}

void EventSelectServer::RemoveClientData( EventSelectClientData* pClientData, const Int32 position )
{
	PRINT_NORMAL_LOG( L"[Enter RemoveClientData] CurPosition = %d\n", position );

	if( pClientData && (0 <= position && pClientData->EventCount >= position) )
	{
		WSACloseEvent( pClientData->ClientEvents[position] );
		closesocket( pClientData->ClientSockets[position].GetSocket() );

		pClientData->EventCount = pClientData->EventCount - 1;
		pClientData->ClientEvents[position] = WSA_INVALID_EVENT;
		pClientData->ClientSockets[position].Reset();

		for( auto Current = position; Current <= pClientData->EventCount; ++Current )
		{
			auto NextCur = (Current + 1);
			auto pNextEvent = pClientData->ClientEvents[NextCur];
			auto pNextSockes = pClientData->ClientSockets[NextCur];

			PRINT_NORMAL_LOG( L"[RemoveClientData] Move Array, Current : %d, Current+1 : %d\n", Current, NextCur );

			pClientData->ClientEvents[Current] = pNextEvent;
			pClientData->ClientSockets[Current].Set( pNextSockes.GetSendBytes(),
													 pNextSockes.GetRecvBytes(),
													 pNextSockes.GetSocket() );
		}
	}
}

void EventSelectServer::DecreaseAcceptedClientCount( void )
{
	// Server-side에서 EventSelect 모델을 돌리기 위해서는 최초 실행 시에 접속하는 소켓에 대한 Accept를 한 번 해두기 때문에,
	// WSAEventCount가 무조건 1부터 시작한다. 그러므로 WaitCount를 0으로 만드는 것이 아니라 최초 Accept 상태로 설정 해둔다.
	if( 1 == _acceptData.EventCount )
	{
		_veryFirstAccept = true;
		return;
	}

	--_acceptData.EventCount;
}