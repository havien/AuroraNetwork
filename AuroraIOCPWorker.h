#pragma once

#include "../AuroraUtility/Includes.h"
#include "../AuroraUtility/AuroraDefine.h"
#include "../AuroraUtility/Queue.h"

#include "Includes.h"
#include "AuroraIOCP.h"

namespace Aurora
{
	namespace Network
	{
		struct PacketHeader;
		class BasePacket;
		class ClientPacket;

		class IOCPWorker
		{
		public:
			IOCPWorker( void );
			~IOCPWorker( void );

			// accept thread.
			static UInt32 __stdcall AcceptWorker( void* pArgs );

			// recv thread.
			static UInt32 __stdcall GQCSWorker( void* pArgs );

			// IOCP.
			bool InitIOCPServer( UInt16 workerThreadCount );
			void Shutdown( void );

			bool RequestRecv( SOCKET socket, OverlappedExtra* pReadOverlappedExtra );
			bool RequestSend( SOCKET socket, OverlappedExtra* pOverlappedExtra );

			bool RequestShutdown( void );
			void ForceCloseClient( SOCKET socket );

			bool EnqueueRecvBuffer( IOCPData* pIOCPData, size_t BufferLength );
			bool EnqueueSendBuffer( IOCPData* pIOCPData, size_t BufferLength );

			void EnqueueRecvBufferAndWakeupParser( IOCPData* pIOCPData, size_t BufferLength );
			void EnqueueSendBufferAndRequestSend( IOCPData* pIOCPData, size_t BufferLength );

			void EnqueuePacket( ClientPacket* const pPacket );

			inline CRITICAL_SECTION* GetCriticalSection( void ) { return &_criticalSection; }

			inline bool GetEchoMode( void ) const { return _echoMode; }
			inline void SetEchoMode( void ) { _echoMode = true; }
			
			inline const SOCKET& GetIOCPListenSocket( void ) const { return _IOCPListenSocket; }

			inline void SetParserQueue( CQueue<IOCPData*>* pQueue )
			{
				_pParserQueue = pQueue;
			}

			inline void SetWaitHandle( const HANDLE* pHandle )
			{ 
				_pParserWaitHandle = const_cast<HANDLE*>(pHandle); 
			}

			inline bool IsRunningGQCS( void ) 
			{
				return _runningGQCSThread;  
			}

			inline void StopAcceptThread( void ) { _runningAcceptThread = false; }
			inline void StopQGCSThread( void ) { _runningGQCSThread = false; }
		private:
			bool _echoMode;
			bool _runningAcceptThread;
			bool _runningGQCSThread;
			
			CRITICAL_SECTION _criticalSection;

			SOCKET _IOCPListenSocket;
			sockaddr_in _listenAddr;
			AuroraIOCP* _pIOCPObject;

			CQueue<IOCPData*>* _pParserQueue;
			CQueue<IOCPData*> _waitSendQueue;

			std::queue<ClientPacket*> _clientPackets;

			HANDLE* _pParserWaitHandle;
		};
	}
}