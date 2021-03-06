/************************************************************************/
/*Add by:zhoulunhao													*/
/*Email	:zhoulunhao@hotmail.com											*/
/************************************************************************/

#include "../object/Player.h"
#include "memory_buffer/NetPack.h"
#include "utility/MsgTool.h"
#include "SeigeForceLog.h"
#include "Fuben.pb.h"
#include "Opcode.pb.h"

using namespace pb;

OpHandler(CMSG_SEIGE_FORCE_ALL_INFO)
{
	pb::GS2C_Seige_Force_All_Info msg;
	m_SeigeForceLog->SaveTo(this,msg);
	Send(pb::SMSG_SEIGE_FORCE_ALL_INFO,msg);
}
OpHandler(CMSG_SEIGE_FORCE_BUY_ARMY)
{
	pb::C2GS_Seige_Force_Buy_Army msg;pack >> msg;
	pb::GS2C_Seige_Force_Buy_Army_Rsp sendmsg;
	m_SeigeForceLog->SaveTo(this,sendmsg,msg.army_id());
	Send(pb::SMSG_SEIGE_FORCE_BUY_ARMY_RSP,sendmsg);
}
OpHandler(CMSG_SEIGE_FORCE_LEVEL_UP)
{
	pb::C2GS_Seige_Force_Level_Up msg;pack >> msg;
	pb::GS2C_Seige_Force_Level_Up_Rsp sendmsg;
	m_SeigeForceLog->SaveTo(this,sendmsg,msg.army_id());
	Send(pb::SMSG_SEIGE_FORCE_LEVEL_UP_RSP,sendmsg);
}
OpHandler(CMSG_SEIGE_FORCE_USE)
{
	pb::C2GS_Seige_Force_Use_Army msg;pack >> msg;
	pb::GS2C_Seige_Force_Use_Army_Rsp sendmsg;
	m_SeigeForceLog->SaveTo(this,sendmsg,msg.army_id(),msg.group_unique_id(),msg.cityid());
	Send(pb::SMSG_SEIGE_FORCE_USE_RSP,sendmsg);
}