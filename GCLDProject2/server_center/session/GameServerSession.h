
#ifndef GAME_SERVER_SESSION_H__
#define GAME_SERVER_SESSION_H__


#include "net_framework/Session.h"
#include "memory_buffer/MemoryPool.h"


class GameServerSession : public Session
{
	DECLARE_OBJ_POOL(GameServerSession)

public:
	typedef void (GameServerSession::*NetPackHandler)(NetPack&);
	typedef std::vector<NetPackHandler> HandlerArray;

public:
	static HandlerArray RegisteServerHandlers( void );

private:
	static HandlerArray s_serverHandlers ;

public:
	GameServerSession( void ) ;
	virtual ~GameServerSession( void ) ;

	virtual void Update( uint64 nowTimeMS, int64 diffMS ) ;

public:
	typedef std::vector< std::string > ServerNameTable ;
	typedef ServerNameTable::iterator ServerNameTableItr  ;

	ServerNameTable& GetServerNames( void ) {   return m_strNames ;   }

private:
	ServerNameTable m_strNames ;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Handler
public:
	virtual bool HandleNetPack( NetPack &pack );

	virtual void HandlePing( NetPack &packet ) ;

	void HandleRegister( NetPack &packet ) ;
	void HandleGetLoginServers( NetPack &packet ) ;
	void HandleGetCrossServers( NetPack &packet );
	void HandleBroadcastToLoginServer( NetPack &packet ) ;

	void HandleC2GWebGetPayerListRet( NetPack &packet ) ;

	void HandleGetEventLogServers( NetPack &packet ) ;
	void HandleWebPayRet( NetPack &packet ) ;

	void HandleSendPlayerMailRet( NetPack &packet ) ;
	void HandlePlayerForbidenRet( NetPack &packet );
	void HandleSelectPlayerVipRet( NetPack &packet );
	void HandleReCreatePayOrderRet( NetPack &packet );
	void HandleExchangeGiftCode( NetPack &packet );
	void HandleFeedBack( NetPack &packet );
	void HandleGetOnlinePlayerCountRet( NetPack &packet );
	void HandleGetServerInfoRet( NetPack &packet );
	
	

	
} ;



#endif


