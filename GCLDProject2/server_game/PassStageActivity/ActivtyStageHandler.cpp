/************************************************************************/
/*Add by:zhoulunhao													*/
/*Email	:zhoulunhao@hotmail.com											*/
/************************************************************************/
#include "object/Player.h"
#include "PassStageActivity/ActivtyStage.h"
#include "PassStageActivity/Table/ActivtyStageRewardTableMgr.h"
#include "Quest.pb.h"
#include "memory_buffer/NetPack.h"
#include "reward/reward.h"
#include "Opcode.pb.h"


using namespace pb; 
OpHandler(CMSG_PASS_STAGE_ALL_INFO)
{
	GS2C_Pass_Stage_Reward_All_Info msg;
	m_PassStageReward->SaveTo(this,msg);
	Send(pb::SMSG_PASS_STAGE_ALL_INFO_RSP,msg);
}


OpHandler(CMSG_PASS_STAGE_TAKE_REWARD)
{
	C2GS_Pass_Stage_Reward msg;
	pack >> msg;
	GS2C_Pass_Stage_Reward_Res sendmsg;
	uint32 Id = msg.id();
	if(IsPassStageGiveGoldActive())
	{
		if(LIKELY(m_PassStageReward->CheckStageIsPassed(Id)))
		{
			if(LIKELY(!m_PassStageReward->CheckStageIsTakeReward(Id)))
			{
				m_PassStageReward->UpdateStageIsTakeReward(Id);
				const ActivityStageReward* stage_reward = sActivityStageRewardTableMgr.GetActivityStageReward(Id);
				if(LIKELY(stage_reward))
				{
					sendmsg.set_ret(pb::PS_TAKE_REWARD_SUCCESS);
					sendmsg.set_id(Id);
					pb::StReward *reward = sendmsg.mutable_rewards();
					reward->set_type(stage_reward->RewardItemId());
					reward->set_value(stage_reward->RewardItemNum());
					bool result = sReward.Change(*this,stage_reward->RewardItemId(),stage_reward->RewardItemNum());
					NLOG("type == %u,num == %u,result = %d",reward->type(),reward->value(),result);
					PassStageRewardStructMap& struct_map = m_PassStageReward->GetPassStageRewardStructMap();
					PlayerPassStageRewardDB::SendInfoToDB(*this,Id,struct_map[Id],pb::DATA_UPDATE);
				}
				else
				{
					sendmsg.set_ret(pb::PS_OTHER_REASON);
					sendmsg.set_id(Id);
				}
			}
			else
			{
				sendmsg.set_ret(pb::PS_STAGE_HAS_TAKE_REWARD);
			}
		}
		else
		{
			sendmsg.set_ret(pb::PS_STAGE_NOT_PASSED);
		}
		Send(pb::SMSG_PASS_STAGE_TAKE_REWARD_RSP,sendmsg);
		return;
	}
}