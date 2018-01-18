#pragma once

#include "../Utility/Includes.h"
#include "../Utility/AuroraDefine.h"
#include "../Utility/Queue.h"

#include "Includes.h"
#include "AuroraIOCP.h"

namespace Aurora
{
	namespace Network
	{
		class IOCPWorker
		{
		public:
			IOCPWorker();
			virtual ~IOCPWorker();
			virtual void Shutdown() = 0;

			void Disconnect( SOCKET& socket, OVERLAPPED* pOverlapped ) noexcept;

			bool RequestShutdown();
			virtual void ForceCloseClient( SOCKET socket ) = 0;

			inline CRITICAL_SECTION* GetCriticalSection(){ return &_criticalSection; }			
			inline void SetWaitHandle( const HANDLE* pHandle )
			{ 
				_pParserWaitHandle = const_cast<HANDLE*>(pHandle); 
			}
		protected:
			CRITICAL_SECTION _criticalSection;
			
			
			HANDLE* _pParserWaitHandle;
		public:
			AuroraIOCP* _pIOCPObject;
		};

		class IOCPAcceptWorker : public IOCPWorker
		{
		public:
			IOCPAcceptWorker();
			virtual ~IOCPAcceptWorker();

			bool Bind( const UInt16 port ) noexcept;
			bool PrepareAccept( const EProtocol protocol, const UInt16 port, const UInt16 workerThreadCount, const UInt16 preAcceptCount );
			void PreAcceptSockets( const UInt16 socketCount ) noexcept;

			static UInt32 __stdcall Worker( void* pArgs );
			virtual void Shutdown();

			/*void PreAcceptSockets( const UInt16 socketCount ) noexcept;
			void Disconnect( SOCKET& socket, OVERLAPPED* pOverlapped ) noexcept;*/

			//bool RequestShutdown();
			virtual void ForceCloseClient( SOCKET socket );
			inline CRITICAL_SECTION* GetCriticalSection() { return &_criticalSection; }

			inline const SOCKET& GetIOCPListenSocket() const { return _IOCPListenSocket; }
			inline const bool IsRunningAccept() noexcept { return _runningAcceptThread; }
			inline void StopAcceptThread() noexcept { _runningAcceptThread = false; }
		private:
			bool _runningAcceptThread;
			
			SOCKET _IOCPListenSocket;
			sockaddr_in _listenAddr;

			//AuroraIOCP* _pIOCPObject;
		};

		struct PacketHeader;
		class BasePacket;
		class ClientPacket;

		class IOCPSendRecvWorker : public IOCPWorker
		{
		public:
			IOCPSendRecvWorker();
			virtual ~IOCPSendRecvWorker();

			static UInt32 __stdcall Worker( void* pArgs );

			bool PrepareSendRecv( const UInt16 threadCount, const UInt16 maxClientCount ) noexcept;
			virtual void Shutdown();

			bool AssociateClient( const SOCKET& socket ) noexcept;

			bool RequestRecv( const SOCKET& socket, OverlappedExtra* pReadOverlappedExtra );
			bool RequestSend( const SOCKET& socket, OverlappedExtra* pOverlappedExtra );

			//bool RequestShutdown();
			virtual void ForceCloseClient( SOCKET socket );

			bool EnqueueRecvBuffer( const IOCPData* pIOCPData, size_t BufferLength );
			bool EnqueueSendBuffer( const IOCPData* pIOCPData, size_t BufferLength );

			void EnqueueRecvBufferAndWakeupParser( const IOCPData* pIOCPData, size_t length );
			void EnqueueSendBufferAndRequestSend( const IOCPData* pIOCPData, size_t length );

			void EnqueuePacket( ClientPacket* const pPacket );

			inline bool GetEchoMode() const { return _echoMode; }
			inline void SetEchoMode() { _echoMode = true; }

			inline void SetParserQueue( CQueue<IOCPData*>* pQueue )
			{
				pQueue;
				//_pParserQueue = pQueue;
			}

			inline const bool IsRunningWorkers() noexcept { return _runningWorkers; }
			inline void StopWorkers() noexcept { _runningWorkers = false; }
		private:
			bool _echoMode;
			bool _runningWorkers;

			//AuroraIOCPSendRecv* _pIOCPObject;

			/*CQueue<IOCPData*>* _pParserQueue;
			CQueue<IOCPData*> _waitSendQueue;*/

			std::queue<ClientPacket*> _clientPackets;
		};
	}
}