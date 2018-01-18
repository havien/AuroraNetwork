#pragma once

#include "../Utility/AuroraDefine.h"
#include "../Utility/AuroraSingleton.h"

#include "Includes.h"

#include "AuroraIOCP.h"
#include "IOCPWorker.h"

namespace Aurora
{
	namespace Network
	{
		class IOCPManager : public Singleton<IOCPManager>
		{
			friend class Singleton<IOCPManager>;
		private:
			explicit IOCPManager();
		public:
			virtual ~IOCPManager();

			bool PrepareAccept( const EProtocol protocol, const UInt16 port, const UInt16 workerThreadCount, UInt16 maxClientCount ) noexcept;
			void PreAcceptSockets( const UInt16 preSocketCount ) noexcept;
			bool PrepareSendRecv( const UInt16 threadCount, const UInt16 maxClientCount ) noexcept;

			bool BeginIOCPAcceptWorker( OUT HANDLE& workerHandle, const HANDLE* eventHandle, const UInt16 threadCount = 1 );
			void BeginSendRecvWorkers( CQueue<IOCPData*>* pQueue, HANDLE* pHandle, const UInt16 threadCount, bool isEchoMode );

			bool AssociateClient( const SOCKET& socket ) noexcept;

			bool RequestRecv( const SOCKET& socket, OverlappedExtra* pOverlapped );

			void RequestSendToIOCP( IOCPData* pIOCPData, size_t BufferLength );

			inline const SOCKET& GetListenSocket() const { return _IOCPAcceptWorker.GetIOCPListenSocket(); }
		private:
			IOCPAcceptWorker _IOCPAcceptWorker;
			IOCPSendRecvWorker _IOCPSendRecvWorker;
		};
	}
}

#define AuroraIOCPManager Aurora::Network::IOCPManager::Instance()