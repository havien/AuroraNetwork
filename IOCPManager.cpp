#pragma once

#include "../Utility/LogManager.h"
#include "../Utility/ThreadManager.h"

#include "AuroraIOCP.h"
#include "IOCPManager.h"

using namespace Aurora;
using namespace Aurora::Network;

IOCPManager::IOCPManager()
{

}

IOCPManager::~IOCPManager( void )
{
}

bool IOCPManager::PrepareAccept( const EProtocol protocol, const UInt16 port, const UInt16 workerThreadCount, UInt16 maxClientCount ) noexcept
{
	return _IOCPAcceptWorker.PrepareAccept( protocol, port, workerThreadCount, maxClientCount );
}

void IOCPManager::PreAcceptSockets( const UInt16 preSocketCount ) noexcept
{
	_IOCPAcceptWorker.PreAcceptSockets( preSocketCount );
}

bool IOCPManager::PrepareSendRecv( const UInt16 threadCount, const UInt16 maxClientCount ) noexcept
{
	return _IOCPSendRecvWorker.PrepareSendRecv( threadCount, maxClientCount );
}

bool IOCPManager::BeginIOCPAcceptWorker( OUT HANDLE& workerHandle, const HANDLE* eventHandle, const UInt16 threadCount )
{
	threadCount;

	_IOCPAcceptWorker.SetWaitHandle( eventHandle );

	AuroraLogManager->Trace( L"Begin Accept Worker Thread.." );
	
	workerHandle = AuroraThreadManager->BeginThread( _IOCPAcceptWorker.Worker, 
													 &_IOCPAcceptWorker, 
													 nullptr );

	if( INVALID_HANDLE_VALUE == workerHandle )
	{
		AuroraLogManager->Critical( L"Failed to Begin IOCP Accept Worker" );
		return false;
	}

	AuroraLogManager->Trace( L"Success Begin to IOCP Worker..!" );
	return true;
}

void IOCPManager::BeginSendRecvWorkers( CQueue<IOCPData*>* pPacketParserQueue, 
										HANDLE* pHandle,
										const UInt16 threadCount, 
										const bool isEchoMode )
{
	if( !pHandle )
	{
		AuroraLogManager->Critical( L"[IOCPManager::BeginSendRecvWorkers] pHandle is nullptr" );
		return;
	}

	if( pPacketParserQueue )
	{
		if( true == isEchoMode )
		{
			_IOCPSendRecvWorker.SetEchoMode();
			AuroraLogManager->Trace( L"[IOCPManager::BeginSendRecvWorkers] Echo Mode! Now Running Echo Server..!" );
		}

		_IOCPSendRecvWorker.SetParserQueue( pPacketParserQueue );
		
		AuroraLogManager->Trace( L"[IOCPManager::BeginSendRecvWorkers] Begin SendRecv Worker Thread..[count: %d]", 
								 threadCount );
		
		for( auto counter = 0; counter < threadCount; ++counter )
		{
			pHandle[counter] = AuroraThreadManager->BeginThread( _IOCPSendRecvWorker.Worker,
																 &_IOCPSendRecvWorker, 
																 nullptr );
		}

		AuroraLogManager->Trace( L"[IOCPManager::BeginSendRecvWorkers] Success Begin to SendRecv Worker..!" );		
	}
	else
	{

		AuroraLogManager->Critical( L"[IOCPManager::BeginSendRecvWorkers] Failed to Begin SendRecv Workers..!" );
	}
}

bool IOCPManager::AssociateClient( const SOCKET& socket ) noexcept
{
	return _IOCPSendRecvWorker.AssociateClient( socket );
}

bool IOCPManager::RequestRecv( const SOCKET& socket, OverlappedExtra* pOverlapped )
{
	return _IOCPSendRecvWorker.RequestRecv( socket, pOverlapped );
}

void IOCPManager::RequestSendToIOCP( IOCPData* pIOCPData, size_t BufferLength )
{
	if( pIOCPData )
	{
		_IOCPSendRecvWorker.EnqueueSendBufferAndRequestSend( pIOCPData, BufferLength );
	}
}