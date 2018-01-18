#pragma once

#include "../Utility/AuroraDefine.h"
#include "../Utility/LogManager.h"
#include "../Utility/StringManager.h"
#include "../Utility/Lock.h"

#include "NetworkManager.h"
#include "WinsockManager.h"
#include "IOCPWorker.h"
#include "IOCPManager.h"
#include "PacketBase.h"

using namespace Aurora;
using namespace Aurora::Network;

IOCPWorker::IOCPWorker() :
_pParserWaitHandle( nullptr ),
_pIOCPObject( nullptr )
{
	InitializeCriticalSection( &_criticalSection );
}

IOCPWorker::~IOCPWorker( void )
{
	SafeDelete( _pIOCPObject );
	DeleteCriticalSection( &_criticalSection );
}

void IOCPWorker::Disconnect( SOCKET& socket, OVERLAPPED* pOverlapped ) noexcept
{
	auto result = AuroraWinsockManager->GetDisconnectEx( socket, pOverlapped, TF_REUSE_SOCKET, 0 );
	if( FALSE == result )
	{
		int error = WSAGetLastError();
		if( error != ERROR_IO_PENDING )
		{
			AuroraLogManager->Error( L"[IOCPWorker::Disconnect] Error! s: %d, error: %d", socket, error );
		}
	}
}


bool IOCPWorker::RequestShutdown( void )
{
	OverlappedExtra ShutdownOverlapped;
	ShutdownOverlapped.Reset();
	ShutdownOverlapped.Operation = EOverlappedOperation::Shutdown;

	LPOVERLAPPED lpOverlapped = reinterpret_cast<LPOVERLAPPED>(&ShutdownOverlapped);
	return TRUE == PostQueuedCompletionStatus( _pIOCPObject, 0, 0, lpOverlapped ) ? true : false;
}

IOCPAcceptWorker::IOCPAcceptWorker() :
IOCPWorker(),
_runningAcceptThread( true ),
_IOCPListenSocket( INVALID_SOCKET )
{

}

IOCPAcceptWorker::~IOCPAcceptWorker()
{
	Shutdown();
	closesocket( _IOCPListenSocket );

	SafeDelete( _pIOCPObject );
}


UInt32 IOCPAcceptWorker::Worker( void* pArgs )
{
	if( nullptr == pArgs )
	{
		AuroraLogManager->Error( L"[IOCPAcceptWorker::Worker] pArgs is nullptr" );
		return 0;
	}

	auto pThis = reinterpret_cast<IOCPAcceptWorker*>(pArgs);
	/*if( false == pThis->_pIOCPObject->IsCreatedIOCP() )
	{
		return 0;
	}*/

	char* pRemainBuffer = nullptr;
	int remainBufferSize = 0;
	while( true == pThis->IsRunningAccept() )
	{
		DWORD transferedBytes = 0;
		//UInt64 CompletionKey = 0;
		OVERLAPPED* pOverlapped = nullptr;
		SOCKET clientSocket = INVALID_SOCKET;

		BOOL GQCSResult = GetQueuedCompletionStatus( pThis->_pIOCPObject->GetIOCPHandle(),
													 &transferedBytes,
													 reinterpret_cast<ULONG_PTR*>(&clientSocket),
													 reinterpret_cast<LPOVERLAPPED*>(&pOverlapped),
													 INFINITE );

		if( FALSE == GQCSResult )
		{
			auto errorCode = WSAGetLastError();
			if( nullptr == pOverlapped )
			{
				AuroraLogManager->Error( L"[IOCPAcceptWorker::Worker Error] %d", errorCode );
			}
			else
			{
				AuroraLogManager->Error( L"[IOCPAcceptWorker::Worker Error] I/O Failed! [%d]", errorCode );

				if( WSAENOTSOCK == errorCode )
				{
					// 이미 닫은 소켓의 OVERLAPPED 데이터가 살아있음.
					AuroraLogManager->Error( L"[IOCPWorker::Worker Error] WSAENOTSOCK, (%d)", clientSocket );
				}

				if( ERROR_NETNAME_DELETED == errorCode )
				{
					// 이미 통신이 종료된 패킷에 통신을 시도. 클라가 먼저 커넥션을 끊었을 가능성이 농후함.
					// Force Close Client Connection.
					AuroraLogManager->Error( L"[IOCPWorker::Worker Error] ERROR_NETNAME_DELETED, (%d)", clientSocket );

					pThis->Disconnect( clientSocket, pOverlapped );

					//pThis->ForceCloseClient( clientSocket );
				}

				// non-paged pool limit. 
				if( WSAENOBUFS == errorCode )
				{
					// 운이 좋아야 여기에 걸린다.
					// Force Close Client Connection.
					AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker Error] Non-Paged Pool LIMIT!! WSAENOBUFS (%d)",
											 clientSocket );

					// 더 이상 연결을 받을 수가 없다. 기존 연결을 끊어서 리소스를 줄여야 한다.
					pThis->Disconnect( clientSocket, pOverlapped );
					//pThis->ForceCloseClient( clientSocket );
				}
			}

			Sleep( 1 );
			continue;
		}

		auto pOverlappedEX = reinterpret_cast<OverlappedExtra*>(pOverlapped);
		switch( pOverlappedEX->Operation )
		{
			case EOverlappedOperation::Accept:
			{
				SOCKADDR* lpLocalSockAddr = NULL;
				SOCKADDR* lpRemoteSockAddr = NULL;

				int localSockAddrLen = 0;
				int remoteSockAddrLen = 0;

				AuroraWinsockManager->GetAcceptExSockaddr( pOverlappedEX->IOCPdata.buffer, 0, sizeof( SOCKADDR_IN ) + 16,
														   sizeof( SOCKADDR_IN ) + 16, &lpLocalSockAddr, &localSockAddrLen,
														   &lpRemoteSockAddr, &remoteSockAddrLen );

				if( 0 != remoteSockAddrLen )
				{
					auto IP = inet_ntoa( ((SOCKADDR_IN *) lpRemoteSockAddr)->sin_addr );
					//auto port = (SOCKADDR_IN*)lpRemoteSockAddr.

					//pThis->in
					AuroraLogManager->Trace( L"[IOCPWorker::GQCSWorker] Client Connect! [IP:%S, port : ]", IP );

					bool associate = AuroraIOCPManager->AssociateClient( pOverlappedEX->IOCPdata.socket );
					if( false == associate )
					{
						AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker] Work Accept! error occur during IOCP Associate" );
						pThis->Disconnect( pOverlappedEX->IOCPdata.socket, pOverlapped );
						break;
					}

					AuroraIOCPManager->RequestRecv( pOverlappedEX->IOCPdata.socket, pOverlappedEX );
				}
				else
				{
					AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker] Failed GetAcceptExSockaddrs %u",
											 GetLastError() );

					//pThis->ForceCloseClient( pOverlappedEX->IOCPdata.socket );
					pThis->Disconnect( pOverlappedEX->IOCPdata.socket, pOverlapped );
				}

				break;
			}
			default:
			{
				AuroraLogManager->Error( L"[IOCPAcceptWorker::Worker] operation isn't Accept!!",
										 GetLastError() );

				pThis->Disconnect( pOverlappedEX->IOCPdata.socket, pOverlapped );
				break;
			}
		}
	}

	return 0;
}

bool IOCPAcceptWorker::Bind( const UInt16 port ) noexcept
{
	if( INVALID_SOCKET == _IOCPListenSocket )
	{
		return false;
	}

	int reuse = 1;
	if( SOCKET_ERROR == setsockopt( _IOCPListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof( reuse ) ) )
	{
		AuroraLogManager->Error( L"[IOCPWorker::Bind] SO_REUSEADDR option failed with error %d",
								 WSAGetLastError() );
		WSACleanup();
		return false;
	}

	_listenAddr.sin_family = AF_INET;
	_listenAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	_listenAddr.sin_port = htons( port );

	auto socketResult = bind( _IOCPListenSocket, (SOCKADDR*) &_listenAddr, sizeof( _listenAddr ) );
	if( SOCKET_ERROR == socketResult )
	{
		AuroraLogManager->Error( L"[IOCPWorker::Bind] failed with error %d", WSAGetLastError() );
		WSACleanup();
		return false;
	}

	return true;
}


bool IOCPAcceptWorker::PrepareAccept( const EProtocol protocol, const UInt16 port, const UInt16 threadCount, const UInt16 preAcceptCount )
{
	AuroraLogManager->Trace( L"[IOCPWorker::PrepareAccept] Begin! (port:%d, threadCount:%d, preAcceptCount:%d)",
							 port, threadCount, preAcceptCount );

	_pIOCPObject = new AuroraIOCPAccept( preAcceptCount );
	if( false == _pIOCPObject->CreateIOCP( threadCount ) )
	{
		AuroraLogManager->Error( L"[IOCPWorker::InitIOCPServer] _pIOCPObject->CreateIOCP" );
		return false;
	}

	_IOCPListenSocket = WSASocket( AF_INET, SOCK_STREAM, static_cast<Int32>(protocol), NULL, 0, WSA_FLAG_OVERLAPPED );
	if( INVALID_SOCKET == _IOCPListenSocket )
	{
		AuroraLogManager->Error( L"[IOCPWorker::InitIOCPServer] Error WSASocket (%d)", WSAGetLastError() );

		WSACleanup();
		return false;
	}

	if( NULL == CreateIoCompletionPort( reinterpret_cast<HANDLE>(_IOCPListenSocket), _pIOCPObject->GetIOCPHandle(), 0, 0 ) )
	{
		AuroraLogManager->Error( L"[IOCPWorker::InitIOCPServer] Error CreateIoCompletionPort (%d)", GetLastError() );
		WSACleanup();

		return false;
	}

	if( false == Bind( port ) )
	{
		return false;
	}

	auto socketResult = SOCKET_ERROR;
	bool isUseEx = true;
	if( true == isUseEx )
	{
		AuroraWinsockManager->LoadAcceptExFunc( _IOCPListenSocket );
		AuroraWinsockManager->LoadAcceptExSockaddrFunc( _IOCPListenSocket );
		AuroraWinsockManager->LoadDisconnectExFunc( _IOCPListenSocket );

		BOOL on = TRUE;
		/*if( setsockopt( _IOCPListenSocket, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char *) &on, sizeof( on ) ) )
		{
		AuroraLogManager->Error( L"[IOCPWorker::InitIOCPServer] SO_CONDITIONAL_ACCEPT option failed with error %d",
		WSAGetLastError() );
		return false;
		}*/

		socketResult = listen( _IOCPListenSocket, 0 );
	}
	else
	{
		socketResult = listen( _IOCPListenSocket, SOMAXCONN );
	}

	if( SOCKET_ERROR == socketResult )
	{
		WSACleanup();
		AuroraLogManager->Error( L"[IOCPWorker::InitIOCPServer] Error listening on socket." );
		return false;
	}

	return true;
}

void IOCPAcceptWorker::PreAcceptSockets( const UInt16 socketCount ) noexcept
{
	for( auto i = 0; i < socketCount; ++i )
	{
		OverlappedExtra* pOverlappedExtra = dynamic_cast<AuroraIOCPAccept*>(_pIOCPObject)->GetLastAcceptOverlappedData();
		if( pOverlappedExtra )
		{
			pOverlappedExtra->IOCPdata.socket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
			if( INVALID_SOCKET == pOverlappedExtra->IOCPdata.socket )
			{
				AuroraLogManager->Error( L"[IOCPWorker::PreAcceptSockets] error WSASocket (%d)",
										 WSAGetLastError() );
			}

			pOverlappedExtra->Operation = EOverlappedOperation::Accept;

			DWORD transferred = 0;
			auto result = AuroraWinsockManager->GetAcceptEx( _IOCPListenSocket,
															 pOverlappedExtra->IOCPdata.socket,
															 pOverlappedExtra->IOCPdata.buffer,
															 0, //(4096 - ((sizeof( SOCKADDR_IN ) + 16) * 2)),
															 sizeof( SOCKADDR_IN ) + 16,
															 sizeof( SOCKADDR_IN ) + 16,
															 &transferred,
															 reinterpret_cast<LPOVERLAPPED>(pOverlappedExtra) );

			int error = WSAGetLastError();
			if( FALSE == result && error != ERROR_IO_PENDING )
			{
				AuroraLogManager->Error( L"[IOCPWorker::PreAcceptSockets] Error!!! count:%d, listen:%d, socket: %d, error: %d",
										 i, _IOCPListenSocket, pOverlappedExtra->IOCPdata.socket, error );
			}
		}
	}
}

void IOCPAcceptWorker::ForceCloseClient( SOCKET socket )
{
	AuroraNetworkManager->ForceCloseSocket( socket );

	//_pIOCPObject->DecreaseClientCount();
}

void IOCPAcceptWorker::Shutdown()
{
	StopAcceptThread();
}

IOCPSendRecvWorker::IOCPSendRecvWorker() :
IOCPWorker(),
_echoMode( false ),
_runningWorkers( true )
{

}

IOCPSendRecvWorker::~IOCPSendRecvWorker()
{
	SafeDelete( _pIOCPObject );
}

UInt32 IOCPSendRecvWorker::Worker( void* pArgs )
{
	if( nullptr == pArgs )
	{
		AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker] pArgs is nullptr" );
		return 0;
	}

	auto pThis = reinterpret_cast<IOCPSendRecvWorker*>(pArgs);
	if( false == pThis->_pIOCPObject->IsCreatedIOCP() )
	{
		return 0;
	}

	char* pRemainBuffer = nullptr;
	int remainBufferSize = 0;
	while( true == pThis->IsRunningWorkers() )
	{
		DWORD transferedBytes = 0;
		UInt64 CompletionKey = 0;
		OVERLAPPED* pOverlapped = nullptr;
		SOCKET clientSocket = INVALID_SOCKET;

		BOOL GQCSResult = GetQueuedCompletionStatus( pThis->_pIOCPObject->GetIOCPHandle(),
													 &transferedBytes,
													 reinterpret_cast<ULONG_PTR*>(&clientSocket),
													 reinterpret_cast<LPOVERLAPPED*>(&pOverlapped),
													 INFINITE );

		if( FALSE == GQCSResult )
		{
			auto errorCode = WSAGetLastError();
			if( nullptr == pOverlapped )
			{
				AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker Error] %d", errorCode );
			}
			else
			{
				AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker Error] I/O Failed! [%d]", errorCode );

				if( WSAENOTSOCK == errorCode )
				{
					// 이미 닫은 소켓의 OVERLAPPED 데이터가 살아있음.
					AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker Error] WSAENOTSOCK, (%d)", clientSocket );
				}

				if( ERROR_NETNAME_DELETED == errorCode )
				{
					// 이미 통신이 종료된 패킷에 통신을 시도. 클라가 먼저 커넥션을 끊었을 가능성이 농후함.
					// Force Close Client Connection.
					AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker Error] ERROR_NETNAME_DELETED, (%d)", clientSocket );

					pThis->Disconnect( clientSocket, pOverlapped );

					//pThis->ForceCloseClient( clientSocket );
				}

				// non-paged pool limit. 
				if( WSAENOBUFS == errorCode )
				{
					// 운이 좋아야 여기에 걸린다.
					// Force Close Client Connection.
					AuroraLogManager->Error( L"[IOCPWorker::GQCSWorker Error] Non-Paged Pool LIMIT!! WSAENOBUFS (%d)",
											 clientSocket );

					// 더 이상 연결을 받을 수가 없다. 기존 연결을 끊어서 리소스를 줄여야 한다.
					pThis->Disconnect( clientSocket, pOverlapped );
					//pThis->ForceCloseClient( clientSocket );
				}
			}

			Sleep( 1 );
			continue;
		}

		auto pOverlappedEX = reinterpret_cast<OverlappedExtra*>(pOverlapped);
		switch( pOverlappedEX->Operation )
		{
			case EOverlappedOperation::Send:
			{
				AuroraLogManager->Trace( L"[RequestSend] Send %d bytes!", transferedBytes );

				if( false == pThis->GetEchoMode() )
				{
					/*if( 0 == pThis->_waitSendQueue.ItemCount() )
					{
						continue;
					}

					CAutoLockW AutoLocker( pThis->GetCriticalSection() );
					{
						auto pWaitSendData = pThis->_waitSendQueue.Dequeue();
						if( pWaitSendData )
						{
							pOverlappedEX->Reset();
							memcpy( &pOverlappedEX->WSABuffer.buf, &pWaitSendData->buffer, sizeof( char ) * BIGGER_BUFFER_LEN );
							pOverlappedEX->WSABuffer.len = BIGGER_BUFFER_LEN;
						}
					}*/
				}

				break;
			}
			case EOverlappedOperation::Recv:
			{
				//AuroraLogManager->Trace( L"[IOCPWorker::GQCSWorker] Received %d bytes", transferedBytes );
				AuroraLogManager->Trace( L"[IOCPSendRecvWorker Receive bytes: %d, String :%S]", transferedBytes, pOverlappedEX->WSABuffer.buf );

				if( 0 == transferedBytes )
				{
					// Disconnect from Client.
					AuroraLogManager->Trace( L"[IOCPWorker::GQCSWorker] Close Connection, (%d)", CompletionKey );
					//pThis->ForceCloseClient( clientSocket );
				}

				//int remainBytes = 0;
				//int receivedBytes = 0;

				/*if( nullptr != pRemainBuffer )
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

					SafeDeleteArray( pRemainBuffer );
				}*/
				
				//remainBytes = transferedBytes;

				//while( 0 < remainBytes )
				//{
				//	
				//	auto pHeader = reinterpret_cast<PacketHeader*>(pOverlappedEX->WSABuffer.buf);
				//	if( false == AuroraNetworkManager->ValidatePacket( pHeader ) )
				//	{
				//		break;
				//	}

				//	auto pClientPacket = reinterpret_cast<ClientPacket*>(pOverlappedEX->WSABuffer.buf);
				//	if( pClientPacket )
				//	{
				//		// echo mode.
				//		if( true == pThis->GetEchoMode() )
				//		{
				//			auto pSendOverlappedExtra = dynamic_cast<AuroraIOCPSendRecv*>(pThis->_pIOCPObject)->GetLastSendOverlappedData();

				//			pSendOverlappedExtra->Reset();

				//			memcpy( &pSendOverlappedExtra->WSABuffer.buf,
				//					&pOverlappedEX->WSABuffer.buf,
				//					sizeof( char )* transferedBytes );

				//			pSendOverlappedExtra->WSABuffer.len = transferedBytes;
				//			pThis->RequestSend( clientSocket, pSendOverlappedExtra );
				//		}
				//		else
				//		{
				//			pThis->EnqueuePacket( pClientPacket );
				//		}
				//	}

				//	remainBytes -= pClientPacket->GetSize();
				//	pClientPacket += remainBytes;
				//}

				//// 패킷을 다 처리했다고 판단했는데 아직 처리못한 버퍼가 남아있다.
				//if( 0 < remainBytes )
				//{
				//	remainBufferSize = remainBytes;
				//	pRemainBuffer = new char[remainBufferSize];
				//	AuroraStringManager->ClearAndCopy( pRemainBuffer, pOverlappedEX->WSABuffer.buf, remainBytes );
				//}

				pThis->RequestRecv( clientSocket, pOverlappedEX );
				break;
			}
			case EOverlappedOperation::Shutdown:
			{
				AuroraLogManager->Trace( L"[RequestRecv] Shutdown IOCP" );
				pThis->Shutdown();
				break;
			}
		}
	}

	return 0;
}

bool IOCPSendRecvWorker::PrepareSendRecv( const UInt16 threadCount, const UInt16 maxClientCount ) noexcept
{
	AuroraLogManager->Trace( L"[IOCPWorker::PrepareSendRecv] Begin! (threadCount:%d, maxClientCount:%d)",
							threadCount, maxClientCount );

	_pIOCPObject = new AuroraIOCPSendRecv( maxClientCount );
	if( false == _pIOCPObject->CreateIOCP( threadCount ) )
	{
		AuroraLogManager->Error( L"[IOCPWorker::PrepareSendRecv] _pIOCPObject->CreateIOCP" );
		return false;
	}

	return true;
}

bool IOCPSendRecvWorker::AssociateClient( const SOCKET& socket ) noexcept
{
	return _pIOCPObject->Associate( socket, _pIOCPObject->GetIOCPHandle() );
}

bool IOCPSendRecvWorker::RequestRecv( const SOCKET& socket, OverlappedExtra* pOverlapped )
{
	if( INVALID_SOCKET != socket && pOverlapped )
	{
		DWORD numOfRecvBytes = 0;
		DWORD recvFlag = 0;

		pOverlapped->Reset();
		pOverlapped->IOCPdata.socket = socket;
		pOverlapped->Operation = EOverlappedOperation::Recv;

		auto recvResult = WSARecv( socket,
								   &(pOverlapped->WSABuffer),
								   1,
								   &numOfRecvBytes,
								   &recvFlag,
								   &(pOverlapped->Overlapped),
								   NULL );

		if( SOCKET_ERROR == recvResult )
		{
			auto error = WSAGetLastError();
			if( WSA_IO_PENDING != error )
			{
				AuroraLogManager->Error( L"[IOCPSendRecvWorker::RequestRecv Error] %d", error );
				return false;
			}
		}

		return true;
	}

	AuroraLogManager->Error( L"[IOCPSendRecvWorker::RequestRecv Error] parameter" );
	return false;
}

bool IOCPSendRecvWorker::RequestSend( const SOCKET& socket, OverlappedExtra* pOverlappedExtra )
{
	if( INVALID_SOCKET != socket && pOverlappedExtra )
	{
		unsigned long NumOfSendBytes = 0;
		unsigned long RecvFlag = 0;

		pOverlappedExtra->IOCPdata.socket = socket;
		pOverlappedExtra->Operation = EOverlappedOperation::Send;

		Int32 RecvResult = WSASend( socket,
									&(pOverlappedExtra->WSABuffer),
									1,
									&NumOfSendBytes,
									RecvFlag,
									&(pOverlappedExtra->Overlapped),
									NULL );

		if( SOCKET_ERROR == RecvResult )
		{
			auto error = WSAGetLastError();
			if( WSA_IO_PENDING != error )
			{
				AuroraLogManager->Error( L"[RequestSend Error] [Socket : %d] %d", socket, error );
				return false;
			}
		}

		return true;
	}

	AuroraLogManager->Error( L"[IOCPSendRecvWorker::RequestSend Error] parameter" );
	return false;
}

void IOCPSendRecvWorker::EnqueueRecvBufferAndWakeupParser( const IOCPData* pIOCPData, size_t length )
{
	if( true == EnqueueRecvBuffer( pIOCPData, length ) )
	{
		SetEvent( (*_pParserWaitHandle) );
	}
}

bool IOCPSendRecvWorker::EnqueueSendBuffer( const IOCPData* pIOCPData, size_t length )
{
	if( pIOCPData )
	{
		auto pSendOverlappedExtra = dynamic_cast<AuroraIOCPSendRecv*>(_pIOCPObject)->GetLastSendOverlappedData();
		if( pSendOverlappedExtra )
		{
			pSendOverlappedExtra->Reset();
			memcpy( &pSendOverlappedExtra->WSABuffer.buf, &pIOCPData->buffer, length );

			auto WSABufferLen = static_cast<unsigned long>(length);
			pSendOverlappedExtra->WSABuffer.len = WSABufferLen;

			/*CAutoLockW AutoLocker( GetCriticalSection() );
			{
				_waitSendQueue.Enqueue( &pSendOverlappedExtra->IOCPdata );
				RequestSend( pIOCPData->socket, pSendOverlappedExtra );
			}*/

			return true;
		}
	}

	return false;
}


bool IOCPSendRecvWorker::EnqueueRecvBuffer( const IOCPData* pIOCPData, size_t length )
{
	/*if( nullptr != _pParserQueue && pIOCPData->buffer && (0 < length) )
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
	}*/

	return false;
}


void IOCPSendRecvWorker::EnqueueSendBufferAndRequestSend( const IOCPData* pIOCPData, size_t length )
{
	EnqueueSendBuffer( pIOCPData, length );
}

void IOCPSendRecvWorker::EnqueuePacket( ClientPacket* const pPacket )
{
	if( nullptr == pPacket )
	{
		return;
	}

	CAutoLockW AutoLocker( GetCriticalSection() );
	{
		auto pNewPacket = new ClientPacket( pPacket->GetType() );
		if( nullptr == pNewPacket )
		{
			AuroraLogManager->Error( L"[EnqueuePacket] new packet error! pNewPacket is nullptr!" );
		}

		memcpy( (void*) pNewPacket, (void*) pPacket, pPacket->GetSize() );
		_clientPackets.emplace( pNewPacket );
	}
}

void IOCPSendRecvWorker::Shutdown()
{
	StopWorkers();
}

void IOCPSendRecvWorker::ForceCloseClient( SOCKET socket )
{
	AuroraNetworkManager->ForceCloseSocket( socket );

	//_pIOCPObject->DecreaseClientCount();
}