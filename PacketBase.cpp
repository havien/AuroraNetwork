#pragma once
#include "PacketBase.h"

using namespace Aurora;
using namespace Aurora::Network;

BasePacket::BasePacket( const UInt16 pType )
{
	CalculateSize();
	type = pType;
}

BasePacket::~BasePacket( void )
{
}

ClientPacket::ClientPacket( const UInt16 pType ) :
BasePacket( pType )
{

}

ClientPacket::~ClientPacket( void )
{

}

ServerPacket::ServerPacket( const UInt16 pType ) :
BasePacket( pType )
{

}

ServerPacket::~ServerPacket( void )
{

}