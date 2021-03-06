/************************************************************************/
/*Add by:zhoulunhao													*/
/*Email	:zhoulunhao@hotmail.com											*/
/************************************************************************/


#include "utility/Utility.h"
#include "WorldFightAchievementMgr.h"
#include "DbTool.h"

WorldFightAchievement::WorldFightAchievement(const DB_WorldFightAchievement& event)
:ment_(event)
{
	Utility::SplitStr(ment_.attack_reward,attack_reward_,'|');
	Utility::SplitStr(ment_.occupy_reward,occupy_reward_,'|');
}

uint32 WorldFightAchievement::Id() const
{
	return ment_.id;
}

uint32 WorldFightAchievement::CityId() const
{
	return ment_.city_id;
}

const IntPair& WorldFightAchievement::getAttackReward() const
{
	return attack_reward_;
}

const IntPair& WorldFightAchievement::getOccupyReward() const
{
	return occupy_reward_;
}

WorldFightAchievementBox::WorldFightAchievementBox(const DB_WorldFightAchievementBox& box)
:box_(box)
{
	Utility::SplitStr(box_.attack_city_reward,attack_city_reward_,'|');
	Utility::SplitStr(box_.occupy_city_reward,occupy_city_reward_,'|');
}

uint32 WorldFightAchievementBox::Id() const
{
	return box_.id;
}

uint32 WorldFightAchievementBox::AttackCityNeedNum() const
{
	return box_.attack_city_need_num;
}

uint32 WorldFightAchievementBox::OccupyCityNeedNum() const
{
	return box_.occupy_city_need_num;
}

const IntPair& WorldFightAchievementBox::getAttackCityReward() const
{
	return attack_city_reward_;
}

const IntPair& WorldFightAchievementBox::getOccupyCityReward() const
{
	return occupy_city_reward_;
}

void WorldFightAchievementMgr::Init()
{
	FOREACH_DB_ITEM(ptr1,DB_WorldFightAchievement)
	{
		WorldFightAchievement *c = new WorldFightAchievement(*ptr1);
		achievement_map_[c->CityId()] = c;
	}
	FOREACH_DB_ITEM(ptr2,DB_WorldFightAchievementBox)
	{
		WorldFightAchievementBox *c = new WorldFightAchievementBox(*ptr2);
		achievement_box_map_[c->Id()] = c;
	}
}

const WorldFightAchievementMap& WorldFightAchievementMgr::getWorldFightAchievementMap() const
{
	return achievement_map_;
}

const WorldFightAchievementBoxMap& WorldFightAchievementMgr::getWorldFightAchievementBoxMap() const
{
	return achievement_box_map_;
}

const WorldFightAchievement* WorldFightAchievementMgr::getWorldFightAchievement(const uint32 city_id) const
{
	return achievement_map_.pfind(city_id);
}

const WorldFightAchievementBox* WorldFightAchievementMgr::getWorldFightAchievementBox(const uint32 id) const
{
	return achievement_box_map_.pfind(id);
}

bool WorldFightAchievementMgr::IsInvalidCityId(const uint32 city_id) const
{
	return achievement_map_.find(city_id) == achievement_map_.end();
}
