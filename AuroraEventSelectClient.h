#pragma once

#include "../Utility/AuroraDefine.h"
#include "../Utility/Queue.h"

#include "Includes.h"
#include "BaseSocket.h"

namespace Aurora
{
	namespace Network
	{
		struct EventSelectClientData
		{
			explicit EventSelectClientData(){ }
			~EventSelectClientData(){ }

			BaseSocket ClientSockets[WSA_MAXIMUM_WAIT_EVENTS];
			WSAEVENT ClientEvents[WSA_MAXIMUM_WAIT_EVENTS];
			bool FirstConnect[WSA_MAXIMUM_WAIT_EVENTS];
			Int32 EventCount;
		};

		struct EventSelect
		{
			explicit EventSelect( const SOCKET* pSocket, ENetworkRunMode RunMode );
			~EventSelect();

			bool StartEventSelect( const SOCKET* pSocket, ENetworkRunMode RunMode );

			SOCKET _eventSocket;
			WSAEVENT _eventObject;
		};

		class EventSelectClient
		{
		public:
			explicit EventSelectClient( const SOCKET* pSocket );
			~EventSelectClient();
		public:
			static UInt32 __stdcall CommunicationWithServer( void* pArgs );
			
			char* GetFrontReceivedPacket();
			void AddReceivedPacket( char* pPacket );

			inline void SetRecvBufferQueue( CQueue<char*>* pRecvBufferQueue )
			{
				if( pRecvBufferQueue )
				{
					_pRecvBufferQueue = pRecvBufferQueue;
				}
			}

			inline void StopCommunicationThread( void )
			{
				_runningCommunicationThread = false; 
			}

			inline HANDLE GetReadWriteHandle() const 
			{
				return _readWriteWaitHandle; 
			}

			inline void ResetReadWriteEvent( void )
			{ 
				ResetEvent( _readWriteWaitHandle ); 
			}

			std::deque<char*>* GetReceivedPacketList( void )
			{
				return &_receivedPacketList; 
			}

			inline const bool GetEchoMode() const
			{ 
				return _echoMode; 
			}

			inline void SetEchoMode( bool EchoMode )
			{
				_echoMode = EchoMode; 
			}

			inline void SaveReceivedPacket( bool value )
			{ 
				_saveReceivedPacket = value;
			}

			inline const bool SaveReceivedPacketMode() const
			{
				return _saveReceivedPacket; 
			}

			CQueue<char*>* _pRecvBufferQueue;
			HANDLE* _targetWakeupThread;
		private:
			bool _echoMode;
			bool _saveReceivedPacket;
			WSAEVENT _eventObject;
			EventSelect* _pEventSelect;
			bool _runningCommunicationThread;
			HANDLE _readWriteWaitHandle;

			// for C++/CLI Chat Client. for Temporary Saved Receive packet.
			std::deque<char*> _receivedPacketList;
		};
	}
}