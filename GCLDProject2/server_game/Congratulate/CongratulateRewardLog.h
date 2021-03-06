/************************************************************************/
/*Add by:zhoulunhao													*/
/*Email	:zhoulunhao@hotmail.com											*/
/************************************************************************/

#ifndef INCLUDE_CONGRATULATE_LOG_H
#define INCLUDE_CONGRATULATE_LOG_H

#include "def/TypeDef.h"
class Player;
namespace pb
{
	class GxDB_CongratulateRewardLog;
	class GxDB_CongratulateInfo;
	class GS2C_Achieve_Congratulate;
	class GxDB_CongratulateBeenCongratualted;
	class C2GS_Congratulate_Accept;
	class GS2C_ListOfCurrentCanCongratulate;
}
struct CongratulateStruct
{
	uint8 is_achieve_;
	//uint8 is_reward_;
	uint32 been_congratulated_count_;
	uint32 table_id_;
	CongratulateStruct()
		:is_achieve_(0)
		//,is_reward_(0)
		,been_congratulated_count_(0)
	,table_id_(0){}
	CongratulateStruct(uint8 is_achieve,uint32 been_congratulated_count,uint32 table_id)
		:is_achieve_(is_achieve)
		//,is_reward_(is_reward)
		,been_congratulated_count_(been_congratulated_count)
		,table_id_(table_id)
	{

	}
};

struct CongratulateBeenStruct
{
	uint64 to_player_id_;
	uint32 rank_ ;
	uint32 suit_id_ ;
	uint32 congratulate_type_ ;
	uint32 table_id_;
	CongratulateBeenStruct()
		:to_player_id_(0)
		,rank_(0)
		,suit_id_(0)
		,congratulate_type_(0)
		,table_id_(0){}
	CongratulateBeenStruct(uint64 to_player_id,uint32 rank,uint32 suit_id,uint32 congratulate_type,uint32 table_id)
		:to_player_id_(to_player_id)
		,rank_(rank)
		,suit_id_(suit_id)
		,congratulate_type_(congratulate_type)
	,table_id_(table_id){}

};
typedef std::map<uint64,CongratulateBeenStruct> CongratulateBeenStructMap; ///active_time ��Ϊkey

typedef std::map<uint64,CongratulateStruct> CongratulateStructMap;


class CongratulateLog
{
	CongratulateStructMap congratulate_struct_map_;
	CongratulateBeenStructMap congratulate_been_struct_map_;
public:
	CongratulateLog();
	bool isInvalidActiveTimeInStructMap(const uint64 active_time);
	bool isInvalidActiveTimeInBeenStructMap(const uint64 active_time );
	void SaveTo(const uint64 active_time,pb::GxDB_CongratulateInfo& msg);
	void SaveTo(const CongratulateStruct& ref,pb::GxDB_CongratulateInfo& msg);
	void SaveTo(const uint64 active_time,pb::C2GS_Congratulate_Accept& msg);
	void SaveTo(pb::GS2C_ListOfCurrentCanCongratulate& msg);
	void LoadFrom(const pb::C2GS_Congratulate_Accept& msg);
	void LoadFrom(const pb::GxDB_CongratulateRewardLog& msg);
	void LoadFrom(const pb::GxDB_CongratulateBeenCongratualted& msg);
	void EraseFromCongratulateBeenMap(const uint32 rank,const uint32 table_id);
	void SetAchieve(const uint64 activeTime,const uint32 id);
	//void SetTakeReward(const uint32 id);
	void AddBeenCongratulated(Player& player,const uint32 table_id);
	CongratulateStructMap& getCongratulateStructMap();
	uint64 GetToPlayerId(const pb::C2GS_Congratulate_Accept& msg) const;
	void ResetCongratulateData(Player& player);
	void ClearCongratulateBeenStructMap(Player& player,const uint64 act_time);
	const uint64 GetCongratulateStructMapBeginTime() const;
	const uint64 GetCongratulateBeenStructMapBeginTime() const;
};



#endif