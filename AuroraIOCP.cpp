#pragma once
#include "../Utility/StringManager.h"
#include "../Utility/LogManager.h"

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
	AuroraStringManager->Clear( buffer, BIGGER_BUFFER_LEN );
}

OverlappedExtra::OverlappedExtra(): 
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
	WSABuffer.len = BIGGER_BUFFER_LEN;
}

AuroraIOCP::AuroraIOCP():
_handle( INVALID_HANDLE_VALUE ),
_created( false )
{	
}

AuroraIOCP::~AuroraIOCP()
{
}

// Create only an completion port without associating it with a file _handle.
bool AuroraIOCP::CreateIOCP( DWORD threadCount )
{
	_handle = CreateIoCompletionPort( INVALID_HANDLE_VALUE, nullptr, 0, threadCount );
	if( nullptr == _handle )
	{
		AuroraLogManager->Critical( L"Error CreateIOCP [%d]", GetLastError() );
		return false;
	}

	_created = true;
	return true;
}

// Associate an existing completion port with a file _handle.
bool AuroraIOCP::Associate( const HANDLE& associateHandle, const Int64& completionKey )
{
	if( INVALID_HANDLE_VALUE == associateHandle )
	{
		AuroraLogManager->Critical( L"Error Associate INVALID_HANDLE_VALUE" );
		return false;
	}

	auto tempHandle = CreateIoCompletionPort( associateHandle, _handle, completionKey, 0 );
	if( NULL == tempHandle )
	{
		AuroraLogManager->Critical( L"[AuroraIOCP::Associate] CreateIoCompletionPort is NULL [%d]", GetLastError() );
		return false;
	}

	// create send/recv data queue.
	_handle = tempHandle;
	return true;
}

bool AuroraIOCP::Associate( const SOCKET& associateSocket, const Int64& completionKey )
{
	return Associate( reinterpret_cast<HANDLE>(associateSocket), completionKey );
}

bool AuroraIOCP::Associate( const SOCKET& associateSocket, const HANDLE& completionKey )
{
	return Associate( reinterpret_cast<HANDLE>(associateSocket), reinterpret_cast<Int64>(completionKey) );
}

AuroraIOCPAccept::AuroraIOCPAccept( UInt16 preAcceptCount ) :
AuroraIOCP(),
_preAcceptCount( preAcceptCount ),
_pAcceptOverlappeds( nullptr )
{
	_pAcceptOverlappeds = new OverlappedExtra[_preAcceptCount];
	ClearOverlapped();
}

AuroraIOCPAccept::~AuroraIOCPAccept()
{
	SafeDeleteArray( _pAcceptOverlappeds );
}

void AuroraIOCPAccept::ClearOverlapped() noexcept
{
	memset( _pAcceptOverlappeds, 0, sizeof( OverlappedExtra ) * _preAcceptCount );
}

OverlappedExtra* AuroraIOCPAccept::GetLastAcceptOverlappedData()
{
	return &_pAcceptOverlappeds[_preAcceptCount - 1];
}

AuroraIOCPSendRecv::AuroraIOCPSendRecv( UInt16 maxClientCount ) :
AuroraIOCP(),
_clientCount( 0 ),
_maxClientCount( maxClientCount ),
_pSendOverlappeds( nullptr ),
_pRecvOverlappeds( nullptr )
{
	_pSendOverlappeds = new OverlappedExtra[_maxClientCount];
	_pRecvOverlappeds = new OverlappedExtra[_maxClientCount];

	ClearOverlapped();
}

AuroraIOCPSendRecv::~AuroraIOCPSendRecv()
{
	SafeDeleteArray( _pRecvOverlappeds );
	SafeDeleteArray( _pSendOverlappeds );
}

void AuroraIOCPSendRecv::ClearOverlapped() noexcept
{
	memset( _pSendOverlappeds, 0, sizeof( OverlappedExtra ) * _maxClientCount );
	memset( _pRecvOverlappeds, 0, sizeof( OverlappedExtra ) * _maxClientCount );
}

void AuroraIOCPSendRecv::IncreaseClientCount( void )
{
	if( true == IsMaxClients() )
	{
		return;
	}

	++_clientCount;
}

void AuroraIOCPSendRecv::DecreaseClientCount( void )
{
	--_clientCount;
	if( 0 > _clientCount )
	{
		_clientCount = 0;
	}
}

OverlappedExtra* AuroraIOCPSendRecv::GetLastRecvOverlappedData()
{ 
	return &_pRecvOverlappeds[_clientCount]; 
}

OverlappedExtra* AuroraIOCPSendRecv::GetLastSendOverlappedData()
{ 
	return &_pSendOverlappeds[(_clientCount - 1)]; 
}

bool AuroraIOCPSendRecv::IsMaxClients() noexcept
{
	return _maxClientCount <= _clientCount;
}