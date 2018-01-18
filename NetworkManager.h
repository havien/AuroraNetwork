#pragma once

#include "../Utility/Includes.h"
#include "../Utility/AuroraDefine.h"
#include "../Utility/AuroraSingleton.h"

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
			virtual ~NetworkManager();

			void Init( ENetworkRunMode NetworkMode );

			void DetermineIPAddress( const sockaddr_in& addr, OUT char* pIP, bool isIPV6 = false );
			const UInt16 GetPort( const sockaddr_in& addr ) noexcept;

			// Client.
			bool InitClient( const char* pServerIP, const UInt16 ServerPort );
			bool ConnectToServer();
			bool SendToServer( const char* pSendBuffer, const UInt16 &WishSendingBytes, OUT Int32 &realSendBytes );
			bool ReceiveFromServer( char* pReceiveBuffer, const UInt16 &WishReceiveBytes, 
									OUT Int32 &ReceivedBytes, EDataReceiveMode ReceiveMode = EDataReceiveMode::WaitCompleteReceive );

			bool BroadcastToClient( const UInt16 ClientCount, BaseSocket* pClientSocketArray, 
									const char* pSendBuffer, const UInt16 &WishSendingBytes, 
									OUT UInt16 &SendedBytes );

			Int32 recvn( SOCKET &GetSocket, char*buf, Int32 RecvLength, Int32 flags );

			inline SOCKET* GetClientSocket(){ return &_clientSocket; }

			// Server.
			bool InitServer( const UInt16 ServerPort );
			bool StartServer();

			//bool BeginAccept();
			bool SendToClient( const SOCKET &ClientSocket, const char* pSendBuffer, 
							   const UInt16 &WishSendingBytes, UInt16 &SendedBytes );

			bool RecvFromClient( const SOCKET &ClientSocket, char* pReceiveBuffer, 
								 const UInt16 &WishReceiveBytes, UInt16 &ReceivedBytes );

			inline void StopServer(){ _isRunningServer = false; }

			const SOCKET *GetServerListenSocket();

			bool CloseSocket( SOCKET* pSocket );
			void ForceCloseSocket( SOCKET GetSocket );

			bool ValidatePacket( PacketHeader const* pHeader );

			inline const ENetworkRunMode GetNetworkMode() const { return _runningMode; }
			inline void SetNetworkMode( ENetworkRunMode NetworkMode ) { _runningMode = NetworkMode; }

			inline const Int32 GetNetworkMode(){ return (int)_runningMode; }
			inline void SetNetworkMode( Int32 NetworkMode ) { _runningMode = static_cast<ENetworkRunMode>(NetworkMode); }
		private:
			NetworkManager();

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