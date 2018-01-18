#pragma once
#include "../Utility/AuroraDefine.h"
#include "../Utility/Queue.h"

#include "Includes.h"

namespace Aurora
{
	namespace Network
	{
		struct EventSelectClientData;
		struct EventSelect;

		class EventSelectServer
		{
		public:
			EventSelectServer( const SOCKET* pSocket );
			~EventSelectServer();
		private:
			bool _echoMode;
			WSAEVENT _eventObject;
		public:
			// accept server thread.
			static UInt32 __stdcall AcceptClientUseEventSelect( void* pArgs );

			// read/receive server thread.
			static UInt32 __stdcall ReadWriteDataUseEventSelect( void* pArgs );

			void SetClientData( const Int32 &EventCount, const SOCKET GetSocket, const WSAEVENT rEvent );
			void SetAcceptData( const Int32 &EventCount, const SOCKET* pSocket, const WSAEVENT rEvent );

			void IncreaseReadWriteClientCount();
			void DecreaseReadWriteClientCount();

			void RemoveClientData( EventSelectClientData* pClientData, const Int32 Position );

			// increase/decrease count.
			void DecreaseAcceptedClientCount();	

			inline EventSelectClientData* GetClientData(){ return &_clientData; }
			inline EventSelectClientData* GetAcceptData(){ return &_acceptData; }

			inline void SetRecvBufferQueue( CQueue<char*>* pRecvBufferQueue )
			{
				if( pRecvBufferQueue )
				{
					pRecvBufferQueue = pRecvBufferQueue;
				}
			}

			inline CQueue<char*>* GetRecvBufferQueue() const
			{
				return _pRecvBufferQueue;
			}

			inline const bool GetEchoMode() const { return _echoMode; }
			inline void SetEchoMode( bool echoMode ) { _echoMode = echoMode; }

			inline const bool IsVeryFirstAccept(){ return _veryFirstAccept; }
			inline void EndFirstAccept(){ _veryFirstAccept = false; }

			// Stop Threads.
			inline void StopAcceptThread(){ _runningAcceptThread = false; }
			inline void StopReadWriteThread(){ _runningReadWriteThread = false; }
			
			HANDLE _targetWakeupThread;
		private:
			EventSelect* _pEventSelect;
			SOCKET _eventSelectSocket;

			// Managed Client Data for EventSelect.
			EventSelectClientData _clientData;
			EventSelectClientData _acceptData;

			// for ServerOperation.
			bool _veryFirstAccept;

			// for Control Server WorkerThread.
			bool _runningAcceptThread;
			bool _runningReadWriteThread;

			// Send Buffer Queue.
			CQueue<char*> _sendBufferQueue;

			// for Parse Packet, Recv Buffer Queue Ptr.
			CQueue<char*>* _pRecvBufferQueue;

			WSAEVENT _listenEventHandle;
			//WSAEVENT m_AcceptEvent;

			std::vector<char*> _waitforSendDatas;
			std::vector<char*> _waitforReceiveDatas;
		};
	}
}