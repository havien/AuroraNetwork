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

			bool RequestRecv( SOCKET socket, OverlappedExtra *pReadOverlappedExtra );
			bool RequestSend( SOCKET socket, OverlappedExtra *pOverlappedExtra );

			bool RequestShutdown( void );
			void ForceCloseClient( SOCKET socket );

			inline bool EnqueueRecvBuffer( IOCPData *pIOCPData, size_t BufferLength );
			inline bool EnqueueSendBuffer( IOCPData *pIOCPData, size_t BufferLength );

			void EnqueueRecvBufferAndWakeupParser( IOCPData *pIOCPData, size_t BufferLength );
			void EnqueueSendBufferAndRequestSend( IOCPData *pIOCPData, size_t BufferLength );


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

			HANDLE* _pParserWaitHandle;
		};
	}
}