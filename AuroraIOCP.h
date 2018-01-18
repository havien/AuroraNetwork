#pragma once

#include "../Utility/Includes.h"
#include "../Utility/AuroraDefine.h"
#include "Includes.h"

namespace Aurora
{
	namespace Network
	{
		enum class EOverlappedOperation : Byte
		{
			None = 0,
			Accept,
			Recv,
			Send,
			RecvFrom,
			SendTo,
			Shutdown,
			Max,
		};

		struct IOCPData
		{
			SOCKET socket;
			char buffer[BIGGER_BUFFER_LEN];

			explicit IOCPData( const SOCKET pSocket );
			~IOCPData();

			void Clear();
		};

		struct OverlappedExtra
		{
			explicit OverlappedExtra();
			~OverlappedExtra();

			void Reset();
			
			OVERLAPPED Overlapped;
			WSABUF WSABuffer;
			EOverlappedOperation Operation;
			IOCPData IOCPdata;
		};

		struct OverlappedExtraRecv : public OverlappedExtra
		{
			Int16 remainPacketLength;
			UInt32 lastCheckTime;
			UInt32 recvPacketCount;
			UInt32 overTrafficCount;
			UInt32 internalBufferSize;
		};

		class AuroraIOCP
		{
		public:
			explicit AuroraIOCP();
			virtual ~AuroraIOCP();
		public:
			virtual void ClearOverlapped() noexcept = 0;

			// Create only an completion port without associating it with a file _handle.
			bool CreateIOCP( DWORD ThreadCount );

			// Associate an existing completion port with a file _handle.
			bool Associate( const HANDLE& associateHandle, const Int64& completionKey );
			bool Associate( const SOCKET& associateSocket, const Int64& completionKey );
			bool Associate( const SOCKET& associateSocket, const HANDLE& completionKey );

			bool IsMaxClients();

			inline bool IsCreatedIOCP() const noexcept { return _created; }
			inline const HANDLE GetIOCPHandle() const noexcept { return _handle; }
		private:
			HANDLE _handle;
			bool _created;
		};

		class AuroraIOCPAccept : public AuroraIOCP
		{
		public:
			explicit AuroraIOCPAccept( UInt16 preAcceptCount );
			~AuroraIOCPAccept();
		public:
			virtual void ClearOverlapped() noexcept;
			OverlappedExtra* GetLastAcceptOverlappedData();
		private:
			UInt16 _preAcceptCount;
			OverlappedExtra* _pAcceptOverlappeds;
		};

		class AuroraIOCPSendRecv : public AuroraIOCP
		{
		public:
			explicit AuroraIOCPSendRecv( UInt16 maxClientCount );
			~AuroraIOCPSendRecv();
		public:
			virtual void ClearOverlapped() noexcept;

			void IncreaseClientCount();
			void DecreaseClientCount();

			OverlappedExtra* GetLastRecvOverlappedData();
			OverlappedExtra* GetLastSendOverlappedData();

			bool IsMaxClients() noexcept;

			inline const UInt16 GetClientCount() const { return _clientCount; }
		private:
			Int16 _clientCount;
			Int16 _maxClientCount;

			OverlappedExtra* _pSendOverlappeds;
			OverlappedExtra* _pRecvOverlappeds;
		};
	}
}