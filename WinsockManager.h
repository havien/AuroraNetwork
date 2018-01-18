#pragma once

#include "../Utility/AuroraDefine.h"
#include "../Utility/AuroraSingleton.h"
#include "../Utility/Queue.h"

#include "Includes.h"

#include "AuroraIOCP.h"
#include "IOCPWorker.h"

namespace Aurora
{
	namespace Network
	{
		class EventSelectClient;
		class EventSelectServer;

		class WinsockManager : public Singleton<WinsockManager>
		{
			friend class Singleton<WinsockManager>;
		private:
			WinsockManager();
		public:
			virtual ~WinsockManager();

			bool InitWinsock() noexcept;
			void Cleanup() noexcept;

			bool LoadAcceptExFunc( const SOCKET listenSocket ) noexcept;
			bool LoadAcceptExSockaddrFunc( const SOCKET listenSocket ) noexcept;
			bool LoadDisconnectExFunc( const SOCKET listenSocket ) noexcept;

			BOOL GetAcceptEx( SOCKET sListenSocket,
							  SOCKET sAcceptSocket,
							  PVOID lpOutputBuffer,
							  DWORD dwReceiveDataLength,
							  DWORD dwLocalAddressLength,
							  DWORD dwRemoteAddressLength,
							  LPDWORD lpdwBytesReceived,
							  LPOVERLAPPED lpOverlapped ) noexcept;

			void GetAcceptExSockaddr( PVOID lpOutputBuffer,
									  DWORD dwReceiveDataLength,
									  DWORD dwLocalAddressLength,
									  DWORD dwRemoteAddressLength,
									  OUT struct sockaddr **LocalSockaddr,
									  OUT LPINT LocalSockaddrLength,
									  OUT struct sockaddr **RemoteSockaddr,
									  OUT LPINT RemoteSockaddrLength ) noexcept;

			BOOL GetDisconnectEx( SOCKET s,
								  OUT LPOVERLAPPED lpOverlapped,
								  DWORD dwFlags,
								  DWORD dwReserved ) noexcept;

			void SetEchoMode( bool EchoMode );

			// EventSelect.
			bool StartEventSelectEchoClient( SOCKET* pSocket, bool EchoMode );

			bool StartEventSelectClient( SOCKET* pSocket, CQueue<char*>* pRecvBufferQueue, HANDLE* TargetWakeupThread );
			void StopEventSelectClient();

			bool StartEventSelectServer( const SOCKET* pSocket, CQueue<char*>* pRecvBufferQueue, 
										 HANDLE TargetWakeupThread, bool EchoMode );
			
			EventSelectClient* GetEventSelectClient( void )
			{
				if( ENetworkRunMode::Client == _runningMode )
				{
					return _pEventSelectClient;
				}

				return nullptr;
			}

			char* GetFrontReceivedPacket();
		private:
			bool _initiialized;
			WSADATA _WSAData;

			LPFN_ACCEPTEX _lpAcceptEx;
			LPFN_GETACCEPTEXSOCKADDRS _lpAcceptExSockAddrs;
			LPFN_DISCONNECTEX _lpDisconnectEx;

			ENetworkRunMode _runningMode;

			bool _useEventSelect;
			SOCKET _eventSelectSocket;

			EventSelectClient* _pEventSelectClient;
			EventSelectServer* _pEventSelectServer;

			WSAEVENT _listenEventHandle;
		};
	}
}

#define AuroraWinsockManager Aurora::Network::WinsockManager::Instance()