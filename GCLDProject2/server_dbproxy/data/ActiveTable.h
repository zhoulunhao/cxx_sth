#pragma once
//������Ų��ܸģ�һһ��Ӧ���ӱ���ʱ��ֻ���������¼ӣ�������������Ψһ������id���Ϸ�ʱ����ͻ
#define LOAD_ALL_PLAYER_MAP_TAB\
    LOAD_TAB (ItemBaseTable)\
    LOAD_TAB (ItemEquipTable)\
    LOAD_TAB (CharacterTable)\
	LOAD_TAB (CharacterJueXingTable)\
    LOAD_TAB (CharacterTrainingTable)\
    LOAD_TAB (QuestCounterTable)\
    LOAD_TAB (QuestLogTable)\
    LOAD_TAB (PlayerMailTable)\
	LOAD_TAB (FuBenPassStageTable)\
	LOAD_TAB (FuBenHideTable)\
	LOAD_TAB (DungeonHeroTable)\
	LOAD_TAB (PlayerStringTable)\
	LOAD_TAB (TechnologyStudyTable)\
	LOAD_TAB (MapLogicCityEventTable)\
	LOAD_TAB (ModuleTable)\
	LOAD_TAB (BuildingTable)\
	LOAD_TAB (FogRecMap )\
	LOAD_TAB (DailyQuestTable)\
	LOAD_TAB (GrowUpQuestTable)\
	LOAD_TAB (GrowUpQuestGroupTable)\
	LOAD_TAB (SignUpTable)\
	LOAD_TAB (PassStageRewardRecordTable)\
	LOAD_TAB (ActivityStarBoxTable)\
	LOAD_TAB (PalaceOfficeAchievement)\
	LOAD_TAB (GeneralRewardTable)\
	LOAD_TAB (WorldFightAchievementTable)\
	LOAD_TAB (SeigeForceTable)\
	LOAD_TAB (ContinueOccupyCityTable)\
	LOAD_TAB (CongratulateTable)\
	LOAD_TAB (CongratulateBeenCongratulatedTable)\
	LOAD_TAB (BranchFuBenTable)\
	LOAD_TAB (WannaBeStrongerTable)\



#define LOAD_ALL_PLAYER_KEY_TAB2\
    LOAD_TAB (BattleArrayTable)\
    LOAD_TAB (QuestCompleteTable)\
	LOAD_TAB (FuBenBaseTable)\
	LOAD_TAB (MarketTable)\
	LOAD_TAB (SmithyTable)\
	LOAD_TAB (AltarTable)\
	LOAD_TAB (BaoQiTable)\
	LOAD_TAB (DungeonTable)\
	LOAD_TAB (MapLogicTable)\
	LOAD_TAB (StageCrusadeTable)\
	LOAD_TAB (ZhengWuTable)\
	LOAD_TAB (PersonalECTable)\


#define LOAD_ALL_PLAYER_KEY_TAB\
    LOAD_TAB (PlayerTable)\
    LOAD_ALL_PLAYER_KEY_TAB2


// ����ı���Ϊÿ����Ϸ��һ�ݣ��������趨���Ǹ�������ϵ�����
// LOAD_TAB (ServerGobalValueTable)
//LOAD_TAB (DB_FriendInfo)
#define LOAD_ALL_GOBAL_TAB\
	LOAD_TAB (CountryBaseTable)\
	LOAD_TAB (PalaceOfficeTable)\
	LOAD_TAB (ActivityProtoTable)\
	LOAD_TAB (ActivityPlanTable)\
	LOAD_TAB (ServerOpenTimeTable)\
	LOAD_TAB (ActivityRewardTable)\
	LOAD_TAB (ActivityCountryOccupyCityTable)\



#define LOAD_ALL_GOBAL_MAP_TAB\
    LOAD_TAB (PlatfromMailTable)\
	LOAD_TAB (CountryCityTable)\
	LOAD_TAB (CountryLevelUpTaskPlayerTable)\


#define LOAD_ALL_TAB\
    LOAD_ALL_PLAYER_MAP_TAB\
    LOAD_ALL_PLAYER_KEY_TAB\
    LOAD_ALL_GOBAL_TAB\
    LOAD_ALL_GOBAL_MAP_TAB\

