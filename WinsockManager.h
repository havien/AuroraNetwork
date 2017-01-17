#pragma once

#include "../AuroraUtility/AuroraDefine.h"
#include "../AuroraUtility/AuroraSingleton.h"
#include "../AuroraUtility/Queue.h"

#include "Includes.h"

#include "AuroraIOCP.h"
#include "AuroraIOCPWorker.h"

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
			WinsockManager( void );
			NON_COPYABLE( WinsockManager );
		public:
			virtual ~WinsockManager( void );
			void Cleanup( void );

			void SetEchoMode( bool EchoMode );

			// EventSelect.
			bool StartEventSelectEchoClient( SOCKET* pSocket, bool EchoMode );

			bool StartEventSelectClient( SOCKET *pSocket, CQueue<char*>* pRecvBufferQueue, HANDLE* TargetWakeupThread );
			void StopEventSelectClient( void );

			bool StartEventSelectServer( const SOCKET* pSocket, CQueue<char*>* pRecvBufferQueue, 
										 HANDLE TargetWakeupThread, bool EchoMode );
			
			// IOCP.
			bool InitIOCP( const UInt16 workerThreadCount );
			void StartIOCPWorker( CQueue<IOCPData*>* pQueue, const HANDLE* eventHandle, const UInt16 hreadCount, bool isEchoMode );
			void RequestSendToIOCP( IOCPData* pIOCPData, size_t BufferLength );

			// for C++/CLI.
			EventSelectClient* GetEventSelectClient( void )
			{
				if( ENetworkRunMode::Client == _runningMode )
				{
					return _pEventSelectClient;
				}

				return nullptr;
			}
			char* GetFrontReceivedPacket( void );

			inline const ENetworkRunMode GetNetworkMode( void ) const
			{
				return _runningMode;
			}

			inline void SetNetworkMode( ENetworkRunMode NetworkMode )
			{
				_runningMode = NetworkMode;
			}
		private:
			bool InitWinsock( void );

			bool _initiialized;
			WSADATA _WSAData;

			ENetworkRunMode _runningMode;

			/////////////////// IOCP Model.
			IOCPWorker _IOCPWorker;

			/////////////////// EventSelect Model.
			bool _useEventSelect;
			SOCKET _eventSelectSocket;

			EventSelectClient* _pEventSelectClient;
			EventSelectServer* _pEventSelectServer;

			WSAEVENT _listenEventHandle;
		};
	}

#define AuroraWinsockManager Network::WinsockManager::Instance()
}