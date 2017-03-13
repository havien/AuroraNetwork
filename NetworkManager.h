#pragma once

#include "../AuroraUtility/Includes.h"
#include "../AuroraUtility/AuroraDefine.h"
#include "../AuroraUtility/AuroraSingleton.h"

#include "Includes.h"
#include "PacketBase.h"
#include "BaseSocket.h"

namespace Aurora
{
	namespace Network
	{
		class NetworkManager : public Singleton<NetworkManager>
		{
			friend class Singleton<NetworkManager>;
		public:
			virtual ~NetworkManager( void );

			void Init( ENetworkRunMode NetworkMode );

			void DetermineIPAddress( const sockaddr_in& addr, OUT char* pIP, bool isIPV6 = false );
			const UInt16 GetPort( const sockaddr_in& addr );

			// Client.
			bool InitClientNetwork( const char* pServerIP, const UInt16 ServerPort );
			bool ConnectToServer( void );
			bool SendToServer( const char* pSendBuffer, const UInt16 &WishSendingBytes, OUT Int32 &realSendBytes );
			bool ReceiveFromServer( char* pReceiveBuffer, const UInt16 &WishReceiveBytes, 
									OUT Int32 &ReceivedBytes, EDataReceiveMode ReceiveMode = EDataReceiveMode::WaitCompleteReceive );

			bool BroadcastToClient( const UInt16 ClientCount, BaseSocket *pClientSocketArray, const char* pSendBuffer,
									const UInt16 &WishSendingBytes, UInt16 &SendedBytes );

			Int32 recvn( SOCKET &GetSocket, char*buf, Int32 RecvLength, Int32 flags );

			inline SOCKET* GetClientSocket( void ) { return &_clientSocket; }

			// Server.
			bool InitServerNetwork( const UInt16 ServerPort );
			bool StartServerNetwork( void );

			bool AcceptClient( void );
			bool SendToClient( const SOCKET &ClientSocket, const char* pSendBuffer, 
							   const UInt16 &WishSendingBytes, UInt16 &SendedBytes );

			bool RecvFromClient( const SOCKET &ClientSocket, char* pReceiveBuffer, 
								 const UInt16 &WishReceiveBytes, UInt16 &ReceivedBytes );

			inline void StopServer( void ) { _isRunningServer = false; }

			const SOCKET *GetServerListenSocket( void );

			bool CloseSocket( SOCKET* pSocket );
			void ForceCloseSocket( SOCKET GetSocket );

			bool ValidatePacket( PacketHeader const* pHeader );

			inline const ENetworkRunMode GetNetworkMode( void ) const { return _runningMode; }
			inline void SetNetworkMode( ENetworkRunMode NetworkMode ) { _runningMode = NetworkMode; }

			inline const Int32 GetNetworkMode( void ) { return (int)_runningMode; }
			inline void SetNetworkMode( Int32 NetworkMode ) { _runningMode = static_cast<ENetworkRunMode>(NetworkMode); }
		private:
			NetworkManager( void );
			NON_COPYABLE( NetworkManager )

			bool _initialized;
			ENetworkRunMode _runningMode;

			// for Client.
			SOCKET _clientSocket;
			sockaddr_in _clientAddr;
			char _serverIP[MAX_IPV4_IP_LEN];
			UInt16 _serverPort;

			// for Server.
			bool _isRunningServer;
			SOCKET _listenSocket;
			sockaddr_in _listenAddr;
		};
	}

	#define AuroraNetworkManager NetworkManager::Instance()
}