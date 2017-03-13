#pragma once

#include "../AuroraUtility/AuroraDefine.h"
#include "../AuroraUtility/MiscManager.h"
#include "../AuroraUtility/StringManager.h"
#include "../AuroraUtility/Lock.h"

#include "NetworkManager.h"
#include "AuroraIOCPWorker.h"
#include "PacketBase.h"

using namespace Aurora;
using namespace Aurora::Network;

IOCPWorker::IOCPWorker( void ) :
_runningAcceptThread( true ),
_runningGQCSThread( true ),
_IOCPListenSocket( INVALID_SOCKET ),
_waitSendQueue( Aurora::NORMAL_QUEUE_SIZE ),
_pParserWaitHandle( nullptr ),
_pParserQueue( nullptr )
{
	InitializeCriticalSection( &_criticalSection );
}

IOCPWorker::~IOCPWorker( void )
{
	SAFE_DELETE( _pIOCPObject );
	DeleteCriticalSection( &_criticalSection );
}

bool IOCPWorker::InitIOCPServer( UInt16 WorkerThreadCount )
{
	_IOCPListenSocket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
	if( INVALID_SOCKET == _IOCPListenSocket )
	{
		return false;
	}

	_listenAddr.sin_family = AF_INET;
	_listenAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	_listenAddr.sin_port = htons( DEFAULT_SERVER_PORT );

	auto socketResult = bind( _IOCPListenSocket, (SOCKADDR*)&_listenAddr, sizeof( _listenAddr ) );
	if( SOCKET_ERROR == socketResult )
	{
		PRINT_NORMAL_LOG( L"bind() failed with error %d\n", WSAGetLastError() );
		return false;
	}

	//----------------------
	// Listen for incoming connection requests 
	// on the created socket
	socketResult = listen( _IOCPListenSocket, SOMAXCONN );
	if( SOCKET_ERROR == socketResult )
	{
		PRINT_NORMAL_LOG( L"Error listening on socket.\n" );
		return false;
	}

	_pIOCPObject = new AuroraIOCP();
	if( false == _pIOCPObject->CreateIOCP( WorkerThreadCount ) )
	{
		return false;
	}

	return true;
}

UInt32 IOCPWorker::AcceptWorker( void* pArgs )
{
	if( nullptr == pArgs )
	{
		return 0;
	}

	auto pThis = (IOCPWorker *)pArgs;
	if( false == pThis->_pIOCPObject->IsCreatedIOCP() )
	{
		return 0;
	}

	while( pThis->_runningAcceptThread )
	{
		sockaddr_in ClientAddr;
		Int32 ClientAddrLen = sizeof( ClientAddr );
		SOCKET ClientSocket = accept( pThis->GetIOCPListenSocket(), (SOCKADDR *)&ClientAddr, &ClientAddrLen );
		if( MAX_CLIENT_COUNT == pThis->_pIOCPObject->GetClientCount() )
		{
			// Client Count is Already Maximum.
			PRINT_NORMAL_LOG( L"[IOCPAcceptWorker] Client Count is Already Maximum...Force Close!%d\n", ClientSocket );
			pThis->ForceCloseClient( ClientSocket );
			continue;
		}

		PRINT_FILE_LOG( L"[IOCPAcceptWorker] Client Connect..%d, Current Client Count : %d\n", ClientSocket, 
						pThis->_pIOCPObject->GetClientCount() );

		// Associate IOCP <-> clientSocket.
		pThis->_pIOCPObject->Associate( ClientSocket, static_cast<UInt64>( ClientSocket ) );

		auto pOverlappedExtra = pThis->_pIOCPObject->GetLastRecvOverlappedData();
		if( pOverlappedExtra )
		{
			pThis->RequestRecv( ClientSocket, pOverlappedExtra );
			pThis->_pIOCPObject->IncreaseClientCount();
		}

		Sleep( 1 );
	}

	return 0;
}

UInt32 IOCPWorker::GQCSWorker( void* pArgs )
{
	if( nullptr == pArgs )
	{
		return 0;
	}

	auto pThis = reinterpret_cast<IOCPWorker*>(pArgs);
	if( false == pThis->_pIOCPObject->IsCreatedIOCP() )
	{
		return 0;
	}

	char* pRemainBuffer = nullptr;
	int remainBufferSize = 0;
	while( true == pThis->_runningGQCSThread )
	{
		DWORD transferedBytes = 0;
		UInt64 CompletionKey = 0;
		OVERLAPPED* pOverlapped = nullptr;
		SOCKET clientSocket = INVALID_SOCKET;

		BOOL GQCSResult = GetQueuedCompletionStatus( pThis->_pIOCPObject->GetIOCPHandle(),
													 &transferedBytes,
													 reinterpret_cast<ULONG_PTR*>( &clientSocket ),
													 reinterpret_cast<LPOVERLAPPED*>( &pOverlapped ),
													 INFINITE );

		if( FALSE == GQCSResult )
		{
			auto errorCode = WSAGetLastError();
			if( nullptr == pOverlapped )
			{
				PRINT_FILE_LOG( L"[GQCS Error] %d\n", errorCode );
			}
			else
			{
				PRINT_NORMAL_LOG( L"[GQCS Error] I/O Failed! [%d]\n", errorCode );

				if( WSAENOTSOCK == errorCode )
				{
					// 이미 닫은 소켓의 OVERLAPPED 데이터가 살아있음.
					PRINT_FILE_LOG( L"[GQCS Error] WSAENOTSOCK, (%d)\n", clientSocket );
				}

				if( ERROR_NETNAME_DELETED == errorCode )
				{
					// 이미 통신이 종료된 패킷에 통신을 시도. 클라가 먼저 커넥션을 끊었을 가능성이 농후함.
					// Force Close Client Connection.
					PRINT_FILE_LOG( L"[GQCS Error] ERROR_NETNAME_DELETED, (%d)\n", clientSocket );
					pThis->ForceCloseClient( clientSocket );
				}

				// non-paged pool limit. 
				if( WSAENOBUFS == errorCode )
				{
					// 운이 좋아야 여기에 걸린다.
					// Force Close Client Connection.
					PRINT_FILE_LOG( L"[GQCS Error] Non-Paged Pool LIMIT!! WSAENOBUFS, (%d)\n", 
									clientSocket );

					pThis->ForceCloseClient( clientSocket );
				}
			}

			Sleep( 1 );
			continue;
		}

		if( 0 == transferedBytes )
		{
			// Disconnect from Client.
			PRINT_FILE_LOG( L"[RequestRecv] Close Connection, (%d)\n", CompletionKey );
			pThis->ForceCloseClient( clientSocket );
		}


		auto pOverlappedEX = reinterpret_cast<OverlappedExtra*>(pOverlapped);
		switch ( pOverlappedEX->Operation )
		{
			case EOverlappedOperation::Send:
			{
				PRINT_NORMAL_LOG( L"[RequestSend] Send %d bytes!\n", transferedBytes );

				if( false == pThis->GetEchoMode() )
				{
					if( 0 == pThis->_waitSendQueue.ItemCount() )
					{
						continue;
					}

					CAutoLockWindows AutoLocker( pThis->GetCriticalSection() );
					{
						auto pWaitSendData = pThis->_waitSendQueue.Dequeue();
						if( pWaitSendData )
						{
							pOverlappedEX->Reset();
							memcpy( &pOverlappedEX->WSABuffer.buf, &pWaitSendData->buffer, sizeof(char) * SUPER_BUFFER_LEN );
							pOverlappedEX->WSABuffer.len = SUPER_BUFFER_LEN;
						}
					}
				}
					
				break;
			}
			case EOverlappedOperation::Recv:
			{
				PRINT_NORMAL_LOG( L"[RequestRecv] Received %d bytes\n", transferedBytes );

				int remainBytes = 0;
				//int receivedBytes = 0;

				if( nullptr != pRemainBuffer )
				{
					remainBytes = remainBufferSize;
					while( 0 < remainBytes )
					{
						auto pHeader = reinterpret_cast<PacketHeader*>(pRemainBuffer);
						if( false == AuroraNetworkManager->ValidatePacket( pHeader ) )
						{
							break;
						}

						auto pClientPacket = reinterpret_cast<ClientPacket*>(pRemainBuffer);
						if( pClientPacket )
						{
							pThis->EnqueuePacket( pClientPacket );

							remainBytes -= pClientPacket->GetSize();
							pClientPacket += remainBytes;
						}
					}

					SAFE_DELETE_ARRAY_POINTER( pRemainBuffer );
				}

				remainBytes = transferedBytes;

				while( 0 < remainBytes )
				{
					auto pHeader = reinterpret_cast<PacketHeader*>(pOverlappedEX->WSABuffer.buf);
					if( false == AuroraNetworkManager->ValidatePacket( pHeader ) )
					{
						break;
					}

					auto pClientPacket = reinterpret_cast<ClientPacket*>(pOverlappedEX->WSABuffer.buf);
					if( pClientPacket )
					{
						// echo mode.
						if( true == pThis->GetEchoMode() )
						{
							auto pSendOverlappedExtra = pThis->_pIOCPObject->GetLastSendOverlappedData();

							pSendOverlappedExtra->Reset();
							memcpy( &pSendOverlappedExtra->WSABuffer.buf,
									&pOverlappedEX->WSABuffer.buf,
									sizeof( char )* transferedBytes );

							pSendOverlappedExtra->WSABuffer.len = transferedBytes;
							pThis->RequestSend( clientSocket, pSendOverlappedExtra );
						}
						else
						{
							pThis->EnqueuePacket( pClientPacket );
						}
					}

					remainBytes -= pClientPacket->GetSize();
					pClientPacket += remainBytes;
				}

				// 패킷을 다 처리했다고 판단했는데 아직 처리못한 버퍼가 남아있다.
				if( 0 < remainBytes )
				{
					remainBufferSize = remainBytes;
					pRemainBuffer = new char[remainBufferSize];
					AuroraStringManager->ClearAndCopy( pRemainBuffer, pOverlappedEX->WSABuffer.buf, remainBytes );
				}

				pThis->RequestRecv( clientSocket, pOverlappedEX );
				break;
			}
			case EOverlappedOperation::ShutdownIOCP:
			{
				PRINT_NORMAL_LOG( L"[RequestRecv] Shutdown IOCP" );
				pThis->Shutdown();
				break;
			}
		}
	}

	return 0;
}

void IOCPWorker::Shutdown( void )
{
	// 사실 이런저런 데이터와 포인터와 등등 정리가 완료된 뒤에 스레드를 종료해야함.
	StopQGCSThread();
	StopAcceptThread();
}

bool IOCPWorker::RequestRecv( SOCKET socket, OverlappedExtra* pReadOverlappedExtra )
{
	if( INVALID_SOCKET != socket && pReadOverlappedExtra )
	{
		DWORD numOfRecvBytes = 0;
		DWORD recvFlag = 0;

		pReadOverlappedExtra->Reset();
		pReadOverlappedExtra->IOCPdata.socket = socket;
		pReadOverlappedExtra->Operation = EOverlappedOperation::Recv;

		Int32 RecvResult = WSARecv( socket,
									&( pReadOverlappedExtra->WSABuffer ),
									1,
									&numOfRecvBytes,
									&recvFlag,
									&( pReadOverlappedExtra->Overlapped ),
									NULL );

		if( SOCKET_ERROR == RecvResult )
		{
			auto error = WSAGetLastError();
			if( WSA_IO_PENDING != error )
			{
				PRINT_NORMAL_LOG( L"[RequestRecv Error] %d\n", error );
				return false;
			}
		}

		return true;
	}

	return false;
}

bool IOCPWorker::RequestSend( SOCKET socket, OverlappedExtra* pOverlappedExtra )
{
	if( INVALID_SOCKET != socket && pOverlappedExtra )
	{
		unsigned long NumOfSendBytes;
		unsigned long RecvFlag = 0;

		pOverlappedExtra->IOCPdata.socket = socket;
		pOverlappedExtra->Operation = EOverlappedOperation::Send;

		Int32 RecvResult = WSASend( socket,
									&( pOverlappedExtra->WSABuffer ),
									1,
									&NumOfSendBytes,
									RecvFlag,
									&( pOverlappedExtra->Overlapped ),
									NULL );
		if( SOCKET_ERROR == RecvResult )
		{
			auto error = WSAGetLastError();
			if( WSA_IO_PENDING != error )
			{
				PRINT_NORMAL_LOG( L"[RequestSend Error] [Socket : %d] %d\n", socket, error );
				return false;
			}
		}

		return true;
	}

	return false;
}

bool IOCPWorker::RequestShutdown( void )
{
	OverlappedExtra ShutdownOverlapped;
	ShutdownOverlapped.Reset();
	ShutdownOverlapped.Operation = EOverlappedOperation::ShutdownIOCP;

	LPOVERLAPPED lpOverlapped = reinterpret_cast<LPOVERLAPPED>( &ShutdownOverlapped );
	return TRUE == PostQueuedCompletionStatus( _pIOCPObject, 0, 0, lpOverlapped ) ? true : false;
}

void IOCPWorker::ForceCloseClient( SOCKET GetSocket )
{
	AuroraNetworkManager->ForceCloseSocket( GetSocket );
	_pIOCPObject->DecreaseClientCount();
	_pIOCPObject->ClearOverlapped();
}

bool IOCPWorker::EnqueueRecvBuffer( IOCPData* pIOCPData, size_t length )
{
	if( nullptr != _pParserQueue && pIOCPData->buffer && ( 0 < length ) )
	{
		CAutoLockWindows AutoLocker( GetCriticalSection() );
		{
			auto pData = new IOCPData( pIOCPData->socket );
			memcpy( pData->buffer, pIOCPData->buffer, sizeof( char ) * length );
			if( pData )
			{
				return _pParserQueue->Enqueue( pData );
			}
		}
	}

	return false;
}

void IOCPWorker::EnqueueRecvBufferAndWakeupParser( IOCPData* pIOCPData, size_t length )
{
	if( true == EnqueueRecvBuffer( pIOCPData, length ) )
	{
		SetEvent( (*_pParserWaitHandle) );
	}
}

bool IOCPWorker::EnqueueSendBuffer( IOCPData* pIOCPData, size_t length )
{
	if( pIOCPData )
	{
		auto pSendOverlappedExtra = _pIOCPObject->GetLastSendOverlappedData();
		if( pSendOverlappedExtra )
		{
			pSendOverlappedExtra->Reset();
			memcpy( &pSendOverlappedExtra->WSABuffer.buf, &pIOCPData->buffer, length );
				
			auto WSABufferLen = static_cast<unsigned long>( length );
			pSendOverlappedExtra->WSABuffer.len = WSABufferLen;
				
			CAutoLockWindows AutoLocker( GetCriticalSection() );
			{
				_waitSendQueue.Enqueue( &pSendOverlappedExtra->IOCPdata );
				RequestSend( pIOCPData->socket, pSendOverlappedExtra );
			}

			return true;
		}
	}

	return false;
}

void IOCPWorker::EnqueueSendBufferAndRequestSend( IOCPData* pIOCPData, size_t length )
{
	EnqueueSendBuffer( pIOCPData, length );
}

void IOCPWorker::EnqueuePacket( ClientPacket* const pPacket )
{
	if( nullptr == pPacket )
	{
		return;
	}

	CAutoLockWindows AutoLocker( GetCriticalSection() );
	{
		auto pNewPacket = new ClientPacket( pPacket->GetType() );
		if( nullptr == pNewPacket )
		{
			PRINT_NORMAL_LOG( L"[EnqueuePacket] new packet error! pNewPacket is nullptr!\n" );
		}

		memcpy( (void*)pNewPacket, (void*)pPacket, pPacket->GetSize() );
		_clientPackets.emplace( pNewPacket );
	}
}