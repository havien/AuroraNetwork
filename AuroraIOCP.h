#pragma once

#include "../AuroraUtility/Includes.h"
#include "../AuroraUtility/AuroraDefine.h"
#include "Includes.h"

namespace Aurora
{
	namespace Network
	{
		const UInt16 MAX_CLIENT_COUNT = 15000;

		enum class EOverlappedOperation : Byte
		{
			None = 0,
			Accept,
			Recv,
			Send,
			RecvFrom,
			SendTo,
			ShutdownIOCP,
			Max,
		};

		struct IOCPData
		{
			SOCKET socket;
			char buffer[SUPER_BUFFER_LEN];

			IOCPData( const SOCKET pSocket );
			~IOCPData( void );

			void Clear( void );
		};

		struct OverlappedExtra
		{
			OverlappedExtra( void );
			~OverlappedExtra( void );

			void Reset( void );
			
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
			AuroraIOCP( void );
			~AuroraIOCP( void );
		public:
			void IncreaseClientCount( void );
			void DecreaseClientCount( void );
			void ClearOverlapped( void );

			// Create only an completion port without associating it with a file _handle.
			bool CreateIOCP( DWORD ThreadCount );

			// Associate an existing completion port with a file _handle.
			bool Associate( HANDLE associateHandle, UInt64 completionKey );
			bool Associate( SOCKET associateSocket, UInt64 completionKey );

			OverlappedExtra* GetLastRecvOverlappedData( void );
			OverlappedExtra* GetLastSendOverlappedData( void );

			inline bool IsCreatedIOCP( void ) const { return _created; }
			inline const HANDLE GetIOCPHandle( void ) const { return _handle; }
			inline const UInt16 GetClientCount( void ) const { return _clientCount; }
		private:
			HANDLE _handle;
			bool _created;
			UInt16 _clientCount;

			OverlappedExtra m_RecvOverlappeds[MAX_CLIENT_COUNT];
			OverlappedExtra m_SendOverlappeds[MAX_CLIENT_COUNT];

			/*CQueue<OverlappedExtra *> m_RecvOverlappeds;
			CQueue<OverlappedExtra *> m_SendOverlapppedQueue;*/
		};
	}
}