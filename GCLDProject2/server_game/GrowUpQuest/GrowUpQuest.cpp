/************************************************************************/
/*Add by:zhoulunhao													*/
/*Email	:zhoulunhao@hotmail.com											*/
/************************************************************************/
#include "GrowUpQuest.h"

#include "Opcode.pb.h"
#include "../object/Player.h"

GrowUpQuestLog::GrowUpQuestLog()
{
	Init();
}



void GrowUpQuestLog::SaveTo( pb::GxDB_Grow_Up_Quest& msg )
{
	for(GrowUpQuestStructMap::iterator Iter = grow_up_quest_struct_map_.begin();Iter != grow_up_quest_struct_map_.end();++Iter)
	{
		pb::GxDB_Grow_Up_Quest_Info* quest_info = msg.add_quest_info();
		quest_info->set_quest_id(Iter->first);
		quest_info->set_cur_value(Iter->second.cur_value);
		quest_info->set_is_finished(Iter->second.is_finished);
		quest_info->set_group_id(Iter->second.group_id);
		quest_info->set_quest_type(Iter->second.quest_type);
		quest_info->set_is_take_reward(Iter->second.is_take_reward);
	}
}

void GrowUpQuestLog::SaveTo(Player& player,pb::GS2C_Grow_Up_Quest_All_Info& msg )
{
	for(GrowUpQuestStructMap::iterator Iter = grow_up_quest_struct_map_.begin();Iter != grow_up_quest_struct_map_.end();++Iter)
	{
		const GrowUpQuestProto* grow_up_quest_proto = sGrowUpQuestTableMgr.GetGrowUpQuestProto(Iter->first);
		if(grow_up_quest_proto)
		{
			pb::Grow_Up_Quest_Info* quest_info = msg.add_grow_up_info();
			quest_info->set_cur_value(Iter->second.cur_value);
			quest_info->set_quest_id(Iter->first);
			quest_info->set_quest_type(grow_up_quest_proto->QuestType());
			quest_info->set_group_id(grow_up_quest_proto->QuestGroup());
			quest_info->set_need_value(grow_up_quest_proto->Value1());
			quest_info->set_is_take_reward(Iter->second.is_take_reward);
			quest_info->set_is_finished(Iter->second.is_finished);
		}	
	}
	
	for(GrowUpQuestGroupStructMap::iterator iter = grow_up_quest_group_struct_map_.begin();iter != grow_up_quest_group_struct_map_.end();++iter)
	{
		const GrowUpQuestGroup* grow_up_quest_group = sGrowUpQuestTableMgr.GetGrowUpQuestGroup(iter->first);
		if(grow_up_quest_group)
		{
			if(player.GetDaysFromRegist() >= grow_up_quest_group->OpenTime())
			{
				pb::Group_Up_Group_Info* group_info = msg.add_group_info();
				group_info->set_group_id(iter->first);
				group_info->set_is_take_group_reward(iter->second.is_take_group_reward);
				group_info->set_is_finished(iter->second.is_finished);
			}
		}
	}
}

void GrowUpQuestLog::SaveTo( pb::GxDB_Grow_Up_Quest_Group& msg )
{
	for(GrowUpQuestGroupStructMap::iterator Iter = grow_up_quest_group_struct_map_.begin();Iter != grow_up_quest_group_struct_map_.end();++Iter)
	{
		pb::GxDB_Grow_Up_Quest_Group_Info* group_info = msg.add_group_info();
		group_info->set_group_id(Iter->first);
		group_info->set_group_is_finished(Iter->second.is_finished);
		group_info->set_is_take_group_reward(Iter->second.is_take_group_reward);
	}
}
void GrowUpQuestLog::LoadFrom( const pb::GxDB_Grow_Up_Quest& msg )
{
	for(int i = 0;i < msg.quest_info_size();++i)
	{
		const pb::GxDB_Grow_Up_Quest_Info& Info = msg.quest_info(i);
		GrowUpQuestStruct& ref = grow_up_quest_struct_map_[Info.quest_id()];
		ref.cur_value = Info.cur_value();
		ref.is_finished = Info.is_finished();
		ref.group_id = Info.group_id();
		ref.quest_type = Info.quest_type();
		ref.is_take_reward = Info.is_take_reward();
	}
}

void GrowUpQuestLog::LoadFrom( const pb::GxDB_Grow_Up_Quest_Group& msg )
{
	for(int i = 0;i < msg.group_info_size();++i)
	{
		const pb::GxDB_Grow_Up_Quest_Group_Info& group_info = msg.group_info(i);
		GrowUpQuestGroupStruct& ref = grow_up_quest_group_struct_map_[group_info.group_id()];
		ref.is_finished = group_info.group_is_finished();
		ref.is_take_group_reward = group_info.is_take_group_reward();
	}
}
void GrowUpQuestLog::Init()
{
	grow_up_quest_struct_map_.clear();
	grow_up_quest_group_struct_map_.clear();
	const GrowUpQuestProtoMap&	grow_up_quest_proto = sGrowUpQuestTableMgr.GetGrowUpQuestProtoMap();
	for(GrowUpQuestProtoMap::const_iterator iter = grow_up_quest_proto.begin();iter != grow_up_quest_proto.end();++iter)
	{
		GrowUpQuestStruct& ref = grow_up_quest_struct_map_[iter->first];
		ref.group_id = iter->second->QuestGroup();
		ref.quest_type = iter->second->QuestType();
		if(UNLIKELY(ref.quest_type == pb::LEVLE_UP))
		{
			ref.cur_value = 1;
		}
		else
		{
			ref.cur_value = 0;
		}
		ref.is_finished = 0;
		ref.is_take_reward = 0;
	}
	
	const GrowUpQuestGroupMap&	grow_up_quest_group = sGrowUpQuestTableMgr.GetGrowUpQuestGroupMap();
	for(GrowUpQuestGroupMap::const_iterator iter = grow_up_quest_group.begin();iter != grow_up_quest_group.end();++iter)
	{
		GrowUpQuestGroupStruct& ref = grow_up_quest_group_struct_map_[iter->first];
		//ref.group_id = iter->first;
		ref.is_finished = 0;
		ref.is_take_group_reward = 0;
	}

}

GrowUpQuestLog::~GrowUpQuestLog()
{

}

void GrowUpQuestLog::UpdateGrowUpData(Player* player,pb::GrowUpQuestType type,const uint32 value)
{
	for(GrowUpQuestProtoMap::const_iterator iter = sGrowUpQuestTableMgr.GetGrowUpQuestProtoMap().begin();iter != sGrowUpQuestTableMgr.GetGrowUpQuestProtoMap().end();++iter)
	{
		if(iter->second->QuestType() == static_cast<uint32>(type)/* && iter->first <= quest_id*/)
		{
			GrowUpQuestStruct& ref = grow_up_quest_struct_map_[iter->first];
			if(value > ref.cur_value)
				ref.cur_value = value;
			if(UNLIKELY(ref.cur_value >= iter->second->Value1()))
			{
				if(ref.is_finished==0)
				{
					ref.is_finished = 1;
				}
			}
			else
				ref.is_finished = 0;
			PlayerGrowUpQuestDb::SendUpdateInfoToDb(*player,iter->first,ref,pb::DATA_UPDATE);
		}
	}
}

bool GrowUpQuestLog::CheckQuestFinished( const uint32 quest_id ) const
{
	GrowUpQuestStructMap::const_iterator iter = grow_up_quest_struct_map_.find(quest_id);
	if(iter != grow_up_quest_struct_map_.end())
	{
		if(iter->second.is_finished)
			return true;
	}
	return false;
}

void GrowUpQuestLog::SendUpdateDataToClient( Player& player,const uint32 quest_id)
{
	pb::GS2C_Grow_Up_Quest_Update msg;
	GrowUpQuestStruct& ref = grow_up_quest_struct_map_[quest_id];
	const GrowUpQuestProto* grow_up_quest_proto = sGrowUpQuestTableMgr.GetGrowUpQuestProto(quest_id);
	pb::Grow_Up_Quest_Info* quest_info = msg.mutable_grow_up_info();
	if(grow_up_quest_proto)
	{
		quest_info->set_quest_type(ref.quest_type);
		quest_info->set_quest_id(quest_id);
		quest_info->set_need_value(grow_up_quest_proto->Value1());
		quest_info->set_cur_value(ref.cur_value);
		quest_info->set_group_id(ref.group_id);
		quest_info->set_is_finished(ref.is_finished);
		quest_info->set_is_take_reward(ref.is_take_reward);
	}
	player.Send(pb::SMSG_GROW_UP_QUEST_UPDATE,msg);

}

GrowUpQuestStructMap& GrowUpQuestLog::GetGrowUpQuestStructMap()
{
	return grow_up_quest_struct_map_;
}

GrowUpQuestGroupStructMap& GrowUpQuestLog::GetGrowUpQuestGroupStructMap()
{
	return grow_up_quest_group_struct_map_;
}

void GrowUpQuestLog::UpdateGroupData(Player* player,const uint32 GroupId )
{
	std::vector<int> vec;
	for(GrowUpQuestStructMap::iterator iter = grow_up_quest_struct_map_.begin();iter != grow_up_quest_struct_map_.end();++iter)
	{
		if(iter->second.group_id == GroupId)
		{
			if(iter->second.is_finished)
				vec.push_back(1);
			else
				vec.push_back(0);
		}
	}
	int i = 1;
	for(std::vector<int>::iterator It = vec.begin();It != vec.end();++It)
	{
		i &= *It;
	}
	GrowUpQuestGroupStruct& ref = grow_up_quest_group_struct_map_[GroupId];
	if(UNLIKELY(i == 1))
	{
		ref.is_finished = 1;
	}
	else
	{
		ref.is_finished = 0;
	}
	MarkTheLastQuestCompleted(player);
	PlayerGrowUpQuestDb::SendUpdateGroupInfoToDb(*player,GroupId,ref,pb::DATA_UPDATE);
}

bool GrowUpQuestLog::CheckGroupFinished( const uint32 group_id ) const
{
	GrowUpQuestGroupStructMap::const_iterator iter = grow_up_quest_group_struct_map_.find(group_id);
	if(iter != grow_up_quest_group_struct_map_.end())
	{
		if(iter->second.is_finished)
			return true;
	}
	return false;
}

void GrowUpQuestLog::SendUpdateGroupDataToClient( Player& player,const uint32 GroupId )
{
	pb::GS2C_Grow_Up_Quest_Group_Update msg;
	GrowUpQuestGroupStruct& ref = grow_up_quest_group_struct_map_[GroupId];
	pb::Group_Up_Group_Info * group_info = msg.mutable_group_info();
	group_info->set_group_id(GroupId);
	group_info->set_is_finished(ref.is_finished);
	group_info->set_is_take_group_reward(ref.is_take_group_reward);
	player.Send(pb::SMSG_GROW_UP_QUEST_GROUP_UPDATE,msg);
}

void GrowUpQuestLog::GM_Finish( Player& player,const uint32 quest_id )
{
	if(quest_id == 0) return;
	if(!CheckQuestFinished(quest_id))
	{
		const GrowUpQuestProto* grow_up_quest = sGrowUpQuestTableMgr.GetGrowUpQuestProto(quest_id);
		GrowUpQuestStruct &ref = grow_up_quest_struct_map_[quest_id];
		if(grow_up_quest)
		{
			ref.cur_value = sGrowUpQuestTableMgr.getMaxValueFromGroupId(grow_up_quest->QuestGroup());
			ref.is_finished = 1;
		}
		SendUpdateDataToClient(player,quest_id);
		PlayerGrowUpQuestDb::SendUpdateInfoToDb(player,quest_id,ref,pb::DATA_UPDATE);
	}
}

// 
// GrowUpQuestStruct GrowUpQuestLog::GetGrowUpQuestStruct( const uint32 quest_id )
// {
// 	return grow_up_quest_struct_map_[quest_id];
// }

// 
// GrowUpQuestGroupStruct GrowUpQuestLog::GetGrowUpQuestGroupStruct( const uint32 group_id )
// {
// 	return grow_up_quest_group_struct_map_[group_id];
// }

void GrowUpQuestLog::GM_FinishGroup( Player& player,const uint32 group_id )
{
	if(group_id == 0) return;
	if(!CheckGroupFinished(group_id))
	{
		GrowUpQuestGroupStruct& ref = grow_up_quest_group_struct_map_[group_id];
		ref.is_finished = 1;
		SendUpdateGroupDataToClient(player,group_id);
		PlayerGrowUpQuestDb::SendUpdateGroupInfoToDb(player,group_id,ref,pb::DATA_UPDATE);
	}
}



bool GrowUpQuestLog::CanTakeReward(pb::GrowUpRewardType type,const uint32 quest_id)
{
	if(LIKELY(type == pb::REWARD_NORMAL))
	{
		if(!CheckQuestFinished(quest_id)){
			return false;
		}
		if(grow_up_quest_struct_map_.empty())
		{
			return false;
		}
		GrowUpQuestStruct &ref = grow_up_quest_struct_map_[quest_id];
		if(ref.is_take_reward == 1)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		if(!CheckGroupFinished(quest_id))
		{
			return false;
		}
		if(grow_up_quest_group_struct_map_.empty())
		{
			return false;
		}
		GrowUpQuestGroupStruct& ref = grow_up_quest_group_struct_map_[quest_id];
		if(ref.is_take_group_reward == 1)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
}

void GrowUpQuestLog::Clear(Player& player)
{
	bool isNotNull = false;
	if(!grow_up_quest_group_struct_map_.empty())
	{
		isNotNull = true;
		grow_up_quest_group_struct_map_.clear();
	}
	if(!grow_up_quest_struct_map_.empty())
	{
		isNotNull = true;
		grow_up_quest_struct_map_.clear();
	}
	if(isNotNull)
	{
		PlayerGrowUpQuestDb::SendDeleteAllInfo(player,pb::DATA_DEL);
	}
}

bool GrowUpQuestLog::CanSendRewardEmail() const
{
	return !grow_up_quest_group_struct_map_.empty() || !grow_up_quest_struct_map_.empty();
}

void GrowUpQuestLog::MarkTheLastQuestCompleted(Player* player)
{
	std::vector<bool> vec_finish;
	for(GrowUpQuestGroupStructMap::const_iterator iter = grow_up_quest_group_struct_map_.begin();iter != grow_up_quest_group_struct_map_.end();++iter)
	{
		if(iter->first == pb::ALL_QUEST && iter->second.is_finished == 1)
		{
			return;
		}
		if(iter->second.is_finished)
		{
			vec_finish.push_back(true);
		}
	}
	bool flag = true;
	for(std::vector<bool>::const_iterator iter = vec_finish.begin();iter != vec_finish.end();++iter)
	{
		flag = flag && *iter;
	}
	if(flag)
	{
		GrowUpQuestGroupStruct& ref = grow_up_quest_group_struct_map_[pb::ALL_QUEST];
		ref.is_finished = 1;
		PlayerGrowUpQuestDb::SendUpdateGroupInfoToDb(*player,pb::ALL_QUEST,ref,pb::DATA_UPDATE);
	}
}
