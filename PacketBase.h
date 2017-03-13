#pragma once
#include "../AuroraUtility/Includes.h"
#include "../AuroraUtility/AuroraDefine.h"

#include "Includes.h"

namespace Aurora
{
	namespace Network
	{
#pragma pack(push, 1)
		struct PacketHeader
		{
			UInt16 size;
			UInt16 type;
		};
		class BasePacket
		{
		protected:
			PacketHeader header;
		public:
			BasePacket( const UInt16 pType );
			virtual ~BasePacket( void );

			inline const UInt16 GetSize( void ) const { return header.size; }
			inline const UInt16 GetType( void ) const { return header.type; }
			inline const UInt16 GetMaxSize( void ) { return sizeof( this ); }

			inline bool CheckSize( void ) { return header.size == sizeof( this ); }
			inline void CalculateSize( void ) { header.size = sizeof( this ); }
		};

		class ClientPacket : public BasePacket
		{
		public:
			ClientPacket( const UInt16 pType );
			virtual ~ClientPacket( void );
		};

		class ServerPacket : public BasePacket
		{
		public:
			ServerPacket( const UInt16 pType );
			virtual ~ServerPacket( void );
		public:
			inline const ECommonResult& GetResult( void ) { return result; }
			inline void SetResult( ECommonResult pResult ) { result = pResult; }
		protected:
			ECommonResult result;
		};
	#pragma pack(pop)
	}
}