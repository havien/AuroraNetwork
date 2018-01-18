#pragma once
#include "../Utility/Includes.h"
#include "../Utility/AuroraDefine.h"

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
			virtual ~BasePacket();

			inline const UInt16 GetSize() const { return header.size; }
			inline const UInt16 GetType() const { return header.type; }
			inline const UInt16 GetMaxSize(){ return sizeof( this ); }

			inline bool CheckSize(){ return header.size == sizeof( this ); }
			inline void CalculateSize(){ header.size = sizeof( this ); }
		};

		class ClientPacket : public BasePacket
		{
		public:
			ClientPacket( const UInt16 pType );
			virtual ~ClientPacket();
		};

		class ServerPacket : public BasePacket
		{
		public:
			ServerPacket( const UInt16 pType );
			virtual ~ServerPacket();
		public:
			inline const ECommonResult& GetResult(){ return result; }
			inline void SetResult( ECommonResult pResult ) { result = pResult; }
		protected:
			ECommonResult result;
		};
	#pragma pack(pop)
	}
}