#pragma once

#include "../AuroraUtility/AuroraDefine.h"
#include "../AuroraUtility/Queue.h"

#include "Includes.h"
#include "BaseSocket.h"

namespace Aurora
{
	namespace Network
	{
		struct EventSelectClientData
		{
			EventSelectClientData( void ) { }
			~EventSelectClientData( void ) { }

			BaseSocket ClientSockets[WSA_MAXIMUM_WAIT_EVENTS];
			WSAEVENT ClientEvents[WSA_MAXIMUM_WAIT_EVENTS];
			bool FirstConnect[WSA_MAXIMUM_WAIT_EVENTS];
			Int32 EventCount;
		};

		struct EventSelect
		{
			EventSelect( const SOCKET* pSocket, ENetworkRunMode RunMode );
			~EventSelect( void );

			bool StartEventSelect( const SOCKET *pSocket, ENetworkRunMode RunMode );

			SOCKET _eventSocket;
			WSAEVENT _eventObject;
		};

		class EventSelectClient
		{
		public:
			EventSelectClient( const SOCKET* pSocket );
			~EventSelectClient( void );
		public:
			static UInt32 __stdcall CommunicationWithServer( void* pArgs );
			
			char* GetFrontReceivedPacket( void );
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

			inline HANDLE GetReadWriteHandle( void ) const 
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

			inline const bool GetEchoMode( void ) const
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

			inline const bool SaveReceivedPacketMode( void ) const
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