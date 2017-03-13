#pragma once
#include "../AuroraUtility/StringManager.h"
#include "AuroraIOCP.h"

using namespace Aurora;
using namespace Aurora::Network;

IOCPData::IOCPData( const SOCKET pSocket ) :
socket( pSocket )
{
	Clear();
}

IOCPData::~IOCPData( void )
{

}

void IOCPData::Clear( void )
{
	AuroraStringManager->Clear( buffer, SUPER_BUFFER_LEN );
}

OverlappedExtra::OverlappedExtra( void ) : 
IOCPdata( INVALID_SOCKET )
{
	Reset();
}

OverlappedExtra::~OverlappedExtra( void )
{

}

void OverlappedExtra::Reset( void )
{
	memset( &Overlapped, 0, sizeof( WSAOVERLAPPED ) );
	memset( &WSABuffer, 0, sizeof( WSABUF ) );
	IOCPdata.Clear();

	Operation = EOverlappedOperation::None;
	WSABuffer.buf = IOCPdata.buffer;
	WSABuffer.len = SUPER_BUFFER_LEN;
}

AuroraIOCP::AuroraIOCP( void ) :
_handle( INVALID_HANDLE_VALUE ),
_created( false ),
_clientCount( 0 )
{
	memset( m_RecvOverlappeds, 0, sizeof(OverlappedExtra) * MAX_CLIENT_COUNT );
	memset( m_SendOverlappeds, 0, sizeof(OverlappedExtra) * MAX_CLIENT_COUNT );
}

AuroraIOCP::~AuroraIOCP()
{
}

void AuroraIOCP::IncreaseClientCount( void )
{
	if( MAX_CLIENT_COUNT <= _clientCount )
	{
		return;
	}

	++_clientCount;
}

void AuroraIOCP::DecreaseClientCount( void )
{
	--_clientCount;
	if( 0 > _clientCount )
	{
		_clientCount = 0;
	}
}

void AuroraIOCP::ClearOverlapped( void )
{
	/*memset( &m_SendOverlappeds[_clientCount], 0, sizeof( OverlappedExtra ) );
	memset( &m_RecvOverlappeds[_clientCount], 0, sizeof( OverlappedExtra ) );*/
}

// Create only an completion port without associating it with a file _handle.
bool AuroraIOCP::CreateIOCP( DWORD threadCount )
{
	_handle = CreateIoCompletionPort( INVALID_HANDLE_VALUE, nullptr, 0, threadCount );
	if( nullptr == _handle )
	{
		DWORD IOCPError = GetLastError(); IOCPError;
		return false;
	}

	_created = true;
	return true;
}

// Associate an existing completion port with a file _handle.
bool AuroraIOCP::Associate( HANDLE associateHandle, UInt64 completionKey )
{
	if( INVALID_HANDLE_VALUE != associateHandle )
	{
		HANDLE TempHandle = CreateIoCompletionPort( associateHandle, _handle, completionKey, 0 );
		if( NULL == TempHandle )
		{
			auto IOCPError = GetLastError(); IOCPError;
			return false;
		}

		// create send/recv data queue.
		_handle = TempHandle;
		return true;
	}

	return false;
}

bool AuroraIOCP::Associate( SOCKET associateSocket, UInt64 completionKey )
{
	if( MAX_CLIENT_COUNT <= _clientCount )
	{
		/*PRINT_NORMAL_LOG( L"Already Max Client Count!!\n" );*/
		return false;
	}

	HANDLE AssocHandle = reinterpret_cast<HANDLE>(associateSocket);
	return Associate( AssocHandle, completionKey );
}

OverlappedExtra* AuroraIOCP::GetLastRecvOverlappedData( void ) 
{ 
	return &m_RecvOverlappeds[_clientCount]; 
}

OverlappedExtra* AuroraIOCP::GetLastSendOverlappedData( void ) 
{ 
	return &m_SendOverlappeds[(_clientCount - 1)]; 
}