#pragma once
#include "../AuroraUtility/Includes.h"
#include "../AuroraUtility/AuroraDefine.h"

#include "Includes.h"

namespace Aurora
{
	namespace Network
	{
#pragma pack(push, 1)
		class BasePacket
		{
		protected:
			UInt16 size;
			UInt16 type;
		public:
			BasePacket( const UInt16 pType );
			virtual ~BasePacket( void );

			inline const UInt16 GetSize( void ) const { return size; }
			inline const UInt16 GetType( void ) const { return type; }
			inline const UInt16 GetMaxSize( void ) { return sizeof( this ); }

			inline bool CheckSize( void ) { return size == sizeof( this ); }
			inline void CalculateSize( void ) { size = sizeof( this ); }
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
			inline void SetResult( ECommonResult pResult ) { result = pResult; }
		protected:
			ECommonResult result;
		};
	#pragma pack(pop)
	}
}