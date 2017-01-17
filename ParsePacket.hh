#pragma once

#include "../AprilUtility/TempDefine.h"
#include "../AprilUtility/WindowsUtilityManager.h"
#include "../AprilUtility/Queue.h"
#include "../AprilUtility/CstringManager.h"

#include "Packet.h"

using namespace AprilUtility;
//namespace AprilNetwork
//{
#ifdef _WIN32 /* || _WIN64*/
	class CParsePacket : public CAprilSingleton<CParsePacket>
	{
		friend class CAprilSingleton<CParsePacket>;
	private:
		CParsePacket( void ) : 
		m_BufferQueue( NormalQueueSize )
		{
			m_ParsePacketEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
			if( m_ParsePacketEvent == NULL )
			{
				//printf( "CreateEvent failed (%d)\n", GetLastError() );
			}

			m_RunningParsePacketThread = true;
		}
		CParsePacket( const CParsePacket& CopyArg );
	public:
		virtual ~CParsePacket( void )
		{
			for ( Counter Current = 0; Current < m_BufferQueue.ItemCount(); ++Current )
			{
				char *pCurrentBuffer = m_BufferQueue.Pop();
				if ( pCurrentBuffer )
				{
					SAFE_DELETE_POINTER( pCurrentBuffer );
				}
			}

			CloseHandle( m_ParsePacketEvent );
		}

		void WakeupParsePacketThread( void )
		{
			SetEvent( m_ParsePacketEvent );
		}

		void ResetWaitEvent( void )
		{
			ResetEvent( m_ParsePacketEvent );
		}

		void StopParsePacketThread( void )
		{
			m_RunningParsePacketThread = false;
		}

		CQueue<char *> * GetBufferQueue( void )
		{
			return &m_BufferQueue;
		}

		void PushBuffer( char *pBuffer )
		{
			if ( pBuffer && (0 < strlen(pBuffer)) )
			{
				size_t BufferLength = sizeof(pBuffer);
				char *pNewBuffer = new char[BufferLength];
				memset( pNewBuffer, 0, BufferLength );
				memcpy( pNewBuffer, pBuffer, BufferLength );
				//std::copy( &pBuffer, &pBuffer + BufferLength, &pNewBuffer );
				//AprilUtility::CCstringManager::GetInstance()->MemsetAndCopyStringA( pNewBuffer, (BufferLength + 1), pBuffer );

				m_BufferQueue.Push( pNewBuffer );

				WakeupParsePacketThread();
			}
		}

		static unsigned int __stdcall ParsePacket( void *pArgs )
		{
			if ( pArgs )
			{
				CParsePacket *pThis = (CParsePacket *)pArgs;
				CWindowsUtilityManager *pWinUtilManager = CWindowsUtilityManager::GetInstance();

				CQueue<char *> *pBufferQueue = pThis->GetBufferQueue();

				// 패킷 사이즈 검사, 패킷 사이즈보다 크면 자름.
				// 패킷 사이즈 보다 작으면 기다려서 사이즈만큼 다 받음.
				// 패킷 타입에 따라 분기.
				while ( true == pThis->m_RunningParsePacketThread )
				{
					DWORD WaitResult = WaitForSingleObject( pThis->m_ParsePacketEvent, INFINITE );
					if ( WAIT_OBJECT_0 == WaitResult )
					{
						//pWinUtilManager->PrintDebugTextToOutputWindow( L"[Wake UP ParsePacket Thread]\n" );
						if ( 0 < pBufferQueue->ItemCount() )
						{
							char *pCurrentPacket = pBufferQueue->Pop();
							if ( pCurrentPacket )
							{
								CBasePacket *pPacket = reinterpret_cast<CBasePacket *>(pCurrentPacket);
								ENormalPacketOperation Operation = static_cast<ENormalPacketOperation>(pPacket->GetOperation());
								switch ( Operation )
								{
									case RegisterReq:
									{

										break;
									}
									case LoginReq:
									{
										break;
									}
									case LogoutReq:
									{
										break;
									}
									case ChatReq:
									{
										break;
									}
								}
								//pWinUtilManager->PrintDebugTextToOutputWindow( L"%S\n", pCurrentPacket );
							}
						}

						pThis->ResetWaitEvent();
					}
					else
					{
						switch ( WaitResult )
						{
							case WAIT_ABANDONED_0:
							case WAIT_FAILED:
							{
								break;
							}
						}
					}
				}
			}

			return 0;
		}

	private:
		bool m_RunningParsePacketThread;
		HANDLE m_ParsePacketEvent;
		CQueue<char *> m_BufferQueue;
	};
//}
#endif