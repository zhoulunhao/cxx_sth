#include "Character.h"
#include "memory_buffer/NetPack.h"
#include "def/ObjectDefines.h"
#include "def/MmoAssert.h"
#include "DbTool.h"
#include "System.h"  
#include "utility/Utility.h"

#include "CharacterProto.h"
#include "CharacterManager.h"
#include "CharacterStorage.h" 

#include "datastruct/struct_character.h" 
#include "WannaBeStronger/WannaBeStrongerLog.h"
#include "Enum.pb.h"
#include "Character.pb.h"
#include "Opcode.pb.h"
#include "ServerOpcode.pb.h"
#include "BaseDefineMgr.h"
#include "BaseDefine.pb.h"
#include "../object/Player.h"
#include "../Map/Table/MapTableMgr.h"
#ifdef _MMO_SERVER_
#include "../GameServer.h"
#include "../Dungeon/PlayerDungeonLog.h"
#endif
#include "../Map/WorldMap.h"
#include "../Map/city/city.h"
#include "../Map/PlayerMapLogic.h"
#include "../Map/country/country.h"
#include "World.pb.h"
#include "../session/PlayerPool.h"
#include "Combat/SkillDataMgr.h"
#include "Combat/CombatGroup.h"
#include "../Technology/PlayerTechnology.h"
#include "../Technology/Table/TechnologyTableMgr.h"
#include "../Service/ServiceMgr.h"
#include "seige_force/SeigeForceLog.h"
#include "ActivityControl/ActivityMgr.h"
#ifdef _SERVER_CROSS_
#include "../../server_cross/cross_wall/CrossWorldMap.h"
#include "cross_wall/CrossWarManager/CrossWarMgr.h"
#endif
#include "CrossWar.pb.h"

using namespace pb;

std::vector<Character*> Character::m_deleteShadow;

static const uint32 HERO_MOVE_TO_NEXT_CITY = 3000; //��һ�������ƶ�����һ������ʱ��

//��������ƶ�
uint32 _Service_CharMove(void* p){
	Character* pChar = (Character*)p;
	pChar->_Move();
	int orgSpeed = GET_BASE_DEF_INT( pb::BD_INITIAL_MOVE_SPEED)*1000;
	uint32 finalSpeed = (uint32)((float)orgSpeed * pChar->m_player->GetHeroSpeedPercent());
	return finalSpeed; // 2.5��
}

//�Զ���ս�ƶ�
uint32 _Service_CharAutoCountryWarMove(void* p){
	Character* pChar = (Character*)p;
	pChar->_AutoCountryWarMove();
	int orgSpeed = GET_BASE_DEF_INT( pb::BD_INITIAL_MOVE_SPEED)*1000;
	uint32 finalSpeed = (uint32)((float)orgSpeed * pChar->m_player->GetHeroSpeedPercent());
	return finalSpeed; // 2.5��
}

Character::Character(const CharacterProto& proto, CharacterStorage* pStorage, Player* player)
:  m_player(player)
, m_IsShadow(false)
, m_DungeonOwner(0)
, m_proto( proto)
, m_pStorage( pStorage)
, m_values( pb::CHARACTER_FIELD_ARRAYSIZE)
, m_isReloadMove(false)
, m_CurSoliderColNum(3)
, m_lastMoveTime(0)
, m_canMove(false)
, m_escapeTime(0)
, m_isInAutoCountryWar(false)
{
#ifdef _MMO_SERVER_
	m_pMap = &sWorldMap;
#endif

#ifdef _SERVER_CROSS_
	m_pMap = &sCrossWorldMap;
#endif

	if(m_player->GetCurSolderRowNum() < GetSolderRowNum() )
	{
		m_player->SetCurSolderRowNum(GetSolderRowNum());
	}
	SetId( 0 ) ; 
    SetLevel( m_proto.GetBirthLevel());
    SetCurXP( 0);
    SetQuality( m_proto.GetBirthQuality());
    SetReinforce( m_proto.GetBirthReinForce());
    m_curCityId = m_pMap->GetCountry(GetCountryId())->m_capitalCityId;
	//LLOG("[1002] m_curCityId=%d, countryid = %d",m_curCityId,GetCountryId());
	InitCombatData();
	RecoverFullSoldier();
	SetEnableTactic(m_proto.GetEnableTactics());
	reCalcPower();
	SetObjType(Combat::Obj_Char);
	
	m_isTuntian = false;
}
Character::Character(const Character& ref)
: m_player(ref.m_player)
, m_curCityId(ref.m_curCityId)				//����ս���У���Ӱ���ڳ��б���Ϊ����ʱ�佫���ڳ���
, m_IsShadow(ref.m_IsShadow)
, m_DungeonOwner(0/*ref.m_DungeonOwner*/)	//�����񣬻�Ӱ��ʼ����ʱ��û�б�����
, m_charId(ref.m_charId)
, m_proto(ref.m_proto)
, m_pStorage(ref.m_pStorage)				//CharacterStorage
, m_values(ref.m_values)					//�佫�ȼ����������Եȣ�װ��Ҳ����
//, m_path(ref.m_path)						//��Ӱ�ƶ�·����ʼ��Ϊ0
, m_CurSoliderColNum(ref.m_CurSoliderColNum)
{
	initPath();
	m_pMap = ref.m_pMap;
	stPro = ref.stPro;
	InitCombatData();
	CopyAllFromParent(ref);
	reCalcPower();
	SetObjType(Combat::Obj_Shadow);
}

void Character::CopyAllFromParent(const Character& ref)
{
	terrainPercent = ref.terrainPercent;
	guardAddPercent = ref.guardAddPercent;
	//fightPercent = ref.fightPercent;
	//skillPercent = ref.skillPercent;
	//expPercent = ref.expPercent;
	//reduceCostHP = ref.reduceCostHP;
	//doubleKill = ref.doubleKill;
	//trainUpStartTime = ref.trainUpStartTime;
	//ʿ���ȼ�
	soldierLv = ref.soldierLv;
	isAutoFight = ref.isAutoFight;
	
}

Character::~Character()
{
	ServiceMgr::UnRegister(SERVICE_CHAR_MOVE, this);
}

void Character::SaveTo( pb::GS2C_CharacterCreate& msg ) const
{
    msg.set_id( GetID());
    msg.set_protoid( ProtoID());
    m_values.BuildFullUpdateMsg( *msg.mutable_values());
    msg.set_cur_city_id(m_curCityId);
    msg.set_dungeon_owner(m_DungeonOwner);
	msg.set_combatgroupid(nowCombatGroupID);
	msg.set_isincombat(isInCombat);
	msg.set_combattype(combatType);
	msg.set_is_fighting(isFighting);
	msg.set_isfakechar((m_objType == Combat::Obj_FakeChar));
}

void Character::SaveTo(pb::GS2C_CharacterUpdate& msg) const
{
	m_values.BuildFullUpdateMsg( *msg.mutable_update_values());
	msg.set_isfakechar((m_objType == Combat::Obj_FakeChar));
}
void Character::LoadFrom( const pb::GS2C_CharacterCreate& msg )
{
    SetId( msg.id());
    m_values.ApplyFullUpdateMsg( msg.values());
#ifdef _MMO_SERVER_
	m_curCityId = msg.cur_city_id();
	//LLOG("[1002] m_curCityId=%d",m_curCityId);
    m_DungeonOwner = msg.dungeon_owner();
	isInCombat = (msg.isincombat()==true? 1:0);
	nowCombatGroupID = msg.combatgroupid();
#endif

	//if(CombatGroup* group = CombatGroup::FindGroup(nowCombatGroupID))
	//{
	//	if()
	//}
	//else
	//{
	//	nowCombatGroupID = 0;
	//	isInCombat = false;
	//}

}


const CharacterProto& Character::Proto() const
{
    return m_proto;
}

uint32 Character::ProtoID() const
{
    return m_proto.CharacterID();
}

uint32 Character::GetMaxRow()
{
	return m_player->GetCurSolderRowNum();
}

int Character::GetMaxLevelCanUpgrade(Player* player)
{
	if ( player == NULL )
	{
		return MAX_CHARACTER_LEVEL;
	}
	else
	{
		return (std::min)(
			MAX_CHARACTER_LEVEL,
			uint32(player->GetLevel() * GET_BASE_DEF_FLOAT(pb::BD_CHAR_UPGRADE_LEVEL_LIMIT_PROPORTION))
			);
	}  

}
int32 Character::GetLevel() const
{
    return Values().GetInt( pb::CHAR_FIELD_LEVEL);
}

int32 Character::GetCurXP() const
{
    return Values().GetInt( pb::CHAR_FIELD_CUR_XP);
}

int32 Character::GetAttack() const
{
	return Values().GetInt(pb::CHAR_FIELD_ATTACK);
}

int32 Character::GetQuality() const
{
    return m_proto.GetBirthQuality();
}


int32 Character::GetReinforce() const
{
    return m_proto.GetBirthReinForce();
}

int32 Character::GetRuneInlay() const
{
    return 0;//Values().GetInt( pb::CHAR_FIELD_RUNE_INLAY);
}


int32 Character::GetSlotEquip( uint32 slot ) const
{
    if ( slot >= pb::EQUIP_SLOT_COUNT)
    {
        return 0;
    }
    else
    {
        return Values().GetInt( pb::CHAR_FIELD_WEAPON + slot);
    }
}


bool Character::IsAllSlotEquip() const
{
    for ( uint32 i = pb::EQUIP_SLOT_WEAPON; i < pb::EQUIP_SLOT_COUNT; ++i)
    {
        if ( Values().GetInt( pb::CHAR_FIELD_WEAPON + i) == 0)
        {
            return false;
        }
    }
    return true;
}

uint32 Character::GetEnableTactics(uint32 geography)
{
	int retnum = 7;

	//�������ս��
	IntPairVec::iterator intTacs = m_enableTacs.begin();
	for(; intTacs != m_enableTacs.end(); ++intTacs)
	{
		if(intTacs->first == geography)
		{
			if( intTacs->second>=1 && intTacs->second <= 3 )
			retnum |= (1<<(intTacs->second+2));
			retnum &= ~(1<<(intTacs->second-1));
		}
	}
	return retnum;
}

int32 Character::GetSkillLevel( uint32 skill_index ) const
{
    if ( skill_index > DB_CharSetSkills::MAX_SKILL_INDEX)
    {
        return 0;
    }
    else
    {
        return 0;//Values().GetInt( pb::CHAR_FIELD_SKIL_LEVEL_0 + skill_index);
    }
}
int32 Character::GetTrainingCount( uint32 training_type ) const
{
    switch ( training_type )
    {
    case 1:
    case 2:
    case 3:
        return 0;//Values().GetInt( pb::CHAR_FIELD_TRAINING_COUNT_0 + training_type - 1);
    default:
        return 0;
    }
}

int32 Character::GetTrainedAttr( uint32 attr_idx ) const
{
    if ( attr_idx > DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM)
    {
        return 0;
    }
    else
    {
        return 0;// Values().GetInt( pb::CHAR_FIELD_0 + attr_idx);
    }
}



int32 Character::GetRandomAttr( uint32 attr_idx ) const
{
    if ( attr_idx > DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM)
    {
        return 0;
    }
    else
    {
        return 0;//Values().GetInt( pb::CHAR_FIELD_RANDOM_0 + attr_idx);
    }
}

// ��δȷ�ϵ�����������һ����ֵ���Ҿ��Ǹ�
//bool Character::HasGoodRandomAttr() const
//{
//    int tmpValue = 0, positiveCount = 0;
//    for ( uint32 i = 0; i <= DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM; ++i)
//    {
//        tmpValue = 0;// Values().GetInt( pb::CHAR_FIELD_RANDOM_0 + i);
//        if ( tmpValue < 0)
//        {
//            return false;
//        }else if( tmpValue > 0)
//        {
//            ++positiveCount;
//        }                 
//    }
//    return positiveCount > 0;    
//}


bool Character::IsRandomAttrAllZero() const
{
    /*for ( uint32 i = 0; i <= DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM; ++i)
    {
        if( Values().GetInt( pb::CHAR_FIELD_RANDOM_0 + i) != 0)
        {
            return false;      
        }                 
    }*/
    return true;
}

bool Character::IsSkillActive( uint32 skill_index ) const
{
    return GetSkillLevel(skill_index) != 0;
}

void Character::LevelUpTo( int32 level )
{
	int32 preLevel = GetLevel();
    SetLevel(level);
	//m_player->UpdateWannaBeStrongerInfo();
#ifdef _MMO_SERVER_
	m_player->UpdateAllGrowUpQuestInfo(pb::CHARACTER_LEVEL,level);
	if (m_player)
	{
		m_player->AddEventLog(pb::EVENT_PLAYER_ARMY_LV_UP,
			3,
			ProtoID(),
			GetLevel(),
			preLevel
			);
	}
#endif
	NotifyStorageModified();	
}

void Character::ExpUpTo( int32 xp )
{
	SetCurXP(xp);
	NotifyStorageModified();	
}

void Character::NotifyStorageModified()
{
    if (m_pStorage && GetID() != 0)
    {
        m_pStorage->SetModifiedID( GetID());
    }
}

void Character::SetLevel( int32 level )
{
    MutableValues().SetInt( pb::CHAR_FIELD_LEVEL, level);
}

void Character::SetCurXP( int32 xp )
{
    MutableValues().SetInt( pb::CHAR_FIELD_CUR_XP, xp); 
}

void Character::SetQuality( int32 quality )
{
    /*MutableValues().SetInt( pb::CHAR_FIELD_QUALITY, quality);
    NotifyStorageModified();
    
    const std::vector<uint32>* skill_index;
    for ( int qualityIter = MIN_CHAR_QUALITY; qualityIter <= quality; ++qualityIter)
    {
        skill_index = Proto().GetSkillUnLockIds(qualityIter);
        if ( skill_index != NULL)
        {
            for ( size_t i = 0; i < skill_index->size(); ++i)
            {
                TryActiveSkill( (*skill_index)[i]);
            }            
        }
    } */
}

void Character::SetReinforce( int32 renforce )
{
   /* MutableValues().SetInt( pb::CHAR_FIELD_REINFORCE, renforce);
    NotifyStorageModified();*/
}


void Character::SetRuneInlay( int32 runeInlay )
{
    /*MutableValues().SetInt( pb::CHAR_FIELD_RUNE_INLAY, runeInlay);
    NotifyStorageModified();*/
}

bool Character::SetSlotEquip( uint32 slot, uint32 itemId )
{
    if ( slot >= pb::EQUIP_SLOT_COUNT)
    {
        return false;
    }
    else
    {
        MutableValues().SetInt( pb::CHAR_FIELD_WEAPON + slot , itemId);
        NotifyStorageModified();
        return true;
    }
}

void Character::SetEnableTactic(const std::string& tacs)
{
	std::vector<std::string> tacticstrs;
	Utility::SplitStr(tacs, tacticstrs, ',');
	{
		for(std::vector<std::string>::iterator ite = tacticstrs.begin(); ite != tacticstrs.end(); ++ite)
		{
			std::vector<int> tactics;
			Utility::SplitStr(*ite, tactics, '|');
			if(tactics.size() == 2)
			{
				IntPair	tacsint;
				tacsint.first = tactics[0];
				tacsint.second = tactics[1];
				m_enableTacs.push_back(tacsint);
			}

		}
	}
}

int  Character::GetEquipSlot( uint32 bagSolt )
{
	for (uint32  i = 0; i < pb::EQUIP_SLOT_COUNT; i++ )
	{
		uint32 curBagSolt = (uint32)(Values().GetInt( pb::CHAR_FIELD_WEAPON + i));
		if (curBagSolt == bagSolt)
		{
			return (int)i;
		}
	}
	return -1;
}


bool Character::TryActiveSkill( uint32 skill_index )
{
    if ( GetSkillLevel(skill_index) == 0)
    {
        return TryAddSkillLevel(skill_index);;
    }
    return false;
}

bool Character::TryAddSkillLevel( uint32 skill_index)
{
    /*if ( skill_index <= DB_CharSetSkills::MAX_SKILL_INDEX)
    {
        MutableValues().SetInt( pb::CHAR_FIELD_SKIL_LEVEL_0 + skill_index , GetSkillLevel( skill_index) + 1);
        NotifyStorageModified();
        return true;
    }*/
    return false;

}

void Character::WriteUpdateInfoTo( pb::GS2C_CharacterUpdate& msg ) const
{
	msg.set_id( GetID());
    if (m_values.IsModified())
    {
        m_values.BuildUpdateMsg( *msg.mutable_update_values());
    }
	msg.set_cur_city_id(m_curCityId);
	msg.set_isincombat(isInCombat==true? 1:0);
	msg.set_combatgroupid(nowCombatGroupID);
	msg.set_isfakechar((m_objType == Combat::Obj_FakeChar));
}

void Character::ApplyUpdateInfoFrom( const pb::GS2C_CharacterUpdate& msg )
{
    m_values.ApplyUpdateMsg( msg.update_values());
}



void Character::SetUnmodified()
{
	m_values.SetUnmodified();
}

bool Character::HasUpdateInfo() const
{
    return m_values.IsModified();
}

int32 Character::GetSumXp() const
{
    int32 sumXp = 0, tmpXp = 0;
    for ( int i = MIN_CHAR_LEVEL; i < GetLevel(); ++i)
    {
        m_proto.TryGetUpgradeXp( i, tmpXp);
        sumXp += tmpXp;
    }
    return sumXp + GetCurXP();
}

int32 Character::GetSumXpLvUpTo( int toLv) const
{
	int32 sumXp = 0, tmpXp = 0;
	for ( int i = GetLevel(); i < toLv; ++i)
	{
		m_proto.TryGetUpgradeXp( i, tmpXp);
		sumXp += tmpXp;
	}
	if (sumXp > GetCurXP())
	{
		return sumXp - GetCurXP();
	}
	return 0;
}

bool Character::TryAddXP(int32 xp, bool isFromCross /* = false */)
{
    int curXp = GetCurXP();
    int lv = GetLevel();
	int limitLevel = GetMaxLevelCanUpgrade(m_player);

    if ( m_proto.TryAddXp(lv, curXp, xp, limitLevel))
    {
        LevelUpTo( lv);
        ExpUpTo( curXp);

		//���¼�������
		CalculateAttr();

		if(!isFromCross)
		{
			if(!IsShadow())
			{
				//������������ֵ�ı䣬��֪ͨ��Ϸ������/���������
				//cross->game
	#ifdef _SERVER_CROSS_
				LLOG("[1002] cross to game Update Character Value");
				pb::GS2R_Update_Player_Character_Exp msg;
				msg.set_charid(m_charId);
				msg.set_addexp(xp);
				m_player->SendToServer(pb::SG2R_UPDATE_PLAYER_CHAR_VALUE,msg,PACKET_TYPE_SERVER_CROSS);
	#endif
				//game->cross
	#ifdef _MMO_SERVER_
				pb::GS2R_Update_Player_Character_Exp msg;
				msg.set_charid(m_charId);
				msg.set_addexp(xp);
				LLOG("[1002] game to cross Update Character Value");
				m_player->SendCrossServerMsg(pb::SG2R_UPDATE_PLAYER_CHAR_VALUE,msg,PACKET_TYPE_SERVER_GAME,m_player->GetGuid());
	#endif
			}
		}

        return true;
    }
    return false;
}

const ValueArray& Character::Values() const
{
    return m_values;
}

ValueArray& Character::MutableValues()
{
    return m_values;
}


bool Character::TryInlayRune( uint32 slot )
{
    uint32 inlayInfo = GetRuneInlay();
    
    if ( IsRuneInlayed (inlayInfo, slot) )
    {
        return false;
    }
    Utility::SetBit(inlayInfo, slot);
    SetRuneInlay( (int)inlayInfo );
    return true;
}

void Character::ClearRune()
{
    SetRuneInlay(0);
}


//bool Character::IsAllInlayed() const
//{
//    return GetRuneInlay() == 0x001F; // ������11111
//}


bool Character::IsRuneInlayed( int32 inlayInfo , uint32 slot )
{
    return Utility::TestBit(inlayInfo, slot);
}

void Character::Rebirth()
{
    //for ( uint32 i = 0 ; i <= DB_CharSetSkills::MAX_SKILL_INDEX; ++i)
    //{
    //    MutableValues().SetInt( pb::CHAR_FIELD_SKIL_LEVEL_0 + i , 0);
    //}

    //// clear count
    //for ( uint32 i = 0 ; i <= DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM; ++i)
    //{
    //    MutableValues().SetInt( pb::CHAR_FIELD_0 + i, 0);
    //}

    //// clear training 
    //for ( uint32 i = 0 ; i < DB_CharUpgradeRandom::TRAINING_TYPE_COUNT; ++i)
    //{
    //    MutableValues().SetInt( pb::CHAR_FIELD_TRAINING_COUNT_0 + i, 0);
    //}


    SetLevel(MIN_CHAR_LEVEL);
    SetReinforce(MIN_CHAR_REINFORCE);
    SetQuality( MIN_CHAR_QUALITY);
    SetCurXP(0);
    ClearRune();
    NotifyStorageModified();
}

int Character::GetTrainingValue( float maxValue, float factor )
{
    return (int)ceilf(maxValue * factor);
}


int Character::GenerateTrainingCount( uint32 training_type )
{
    switch( training_type)
    {
    case 1: return System::Rand(1,4); 
    case 2: return System::Rand(2,5); 
    case 3: return System::Rand(3,6);
    default: return 0;
    }
}

float Character::GenerateTrainingFactor( uint32 training_type , uint32 addtion )
{
    switch( training_type)
    {
    case 1: return System::Randf(-0.001f,0.001f*addtion);
    case 2: return System::Randf(-0.001f,0.002f*addtion);
    case 3: return System::Randf(-0.001f,0.003f*addtion);
    default: return 0.0f;
    }
}


bool Character::Training( uint32 training_type , uint32 addtion )
{
    //uint32 trainingCount = GenerateTrainingCount(training_type);
    //
    //MMO_ASSERT( trainingCount <= DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM + 1) ;      //0->4 sum = 5, so +1
    //if ( trainingCount > DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM + 1)
    //    return false;

    //const std::vector<float>* classMax
    //    = sCharacterMgr.GetAttrRandomMaxVecOfClass(Proto().CharClass());

    //if ( classMax == NULL)
    //    return false;
    //
    //vector<uint32> randomlist;
    //for ( uint32 i = 0; i <= DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM; ++i )
    //{
    //    randomlist.push_back(i);     
    //}
    //std::random_shuffle( randomlist.begin(), randomlist.end());
    //do
    //{
    //    // clear 
    //    for ( uint32 i = 0 ; i <= DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM ; ++i )
    //    {
    //        MutableValues().SetInt( pb::CHAR_FIELD_RANDOM_0  + i, 0 );
    //    }
    //    for ( uint32 i = 0 ; i < trainingCount; ++i)
    //    {
    //        uint32 trainingIdx = randomlist[i];
    //        int32 changeValue = GetTrainingValue( (*classMax)[trainingIdx], GenerateTrainingFactor(training_type, addtion));             
    //        MutableValues().SetInt( pb::CHAR_FIELD_RANDOM_0 + trainingIdx, changeValue);
    //    }
    //} while (  IsRandomAttrAllZero());

    //MutableValues().ModifyInt(pb::CHAR_FIELD_TRAINING_COUNT_0 + training_type - 1, 1);
    NotifyStorageModified();
    return true;
}

bool Character::ConfirmTraining()
{
	if ( IsRandomAttrAllZero())
		return false;

    const DB_CharAttrRandom* curQualityRandom 
        = sCharacterMgr.GetAttrRandomMaxOfQuality(Proto().CharClass(), GetQuality());

    if ( curQualityRandom == NULL)
        return false;

 //   for ( uint32 i = 0; i <= DB_CharAttrRandom::MAX_ATTR_INDEX_CAN_RANDOM; ++i) 
	//{
 //       int attr = MutableValues().GetInt( pb::CHAR_FIELD_0 + i) + MutableValues().GetInt(pb::CHAR_FIELD_RANDOM_0 + i);
 //       Utility::LimitValue<int>( attr, 0, (int)curQualityRandom->random_value[i]);         
 //       MutableValues().SetInt( pb::CHAR_FIELD_0 + i, attr );
 //       MutableValues().SetInt( pb::CHAR_FIELD_RANDOM_0 + i, 0);
	//}

	//NotifyStorageModified();
	return true;
}


void Character::GM_SetAttr( int level , int quality, int reinforce )
{
    LevelUpTo(level);
    SetQuality(quality);
   // SetReinforce(reinforce);
}

uint32 Character::GetCountryId()
{
	if (m_player)
	{
		 return m_player->m_worldMapLogic->GetCountryId();
	}
	else
		return 1;
}
uint64 Character::GetPlayerId()
{
	if (m_player)
	{
		return m_player->GetGuid();
	}
	else
		return 0;
}
const string& Character::GetPlayerName()
{
	
	if (m_player)
	{
		return m_player->CharName();
	}
	else
	{
		return CombatObj::GetPlayerName();
	}
}

void Character::MoveTo(uint32 cityId)
{
	LLOG("Character::MoveTo,charid=%d",ProtoID());
    if (!m_pMap) return;

	initPath();	//ֹͣ�佫�ƶ�

#ifdef _SERVER_CROSS_
	Country* mycountry = sCrossWorldMap.GetCountry(GetCountryId());
	if(mycountry)
	{
		if(mycountry->IsCrossCountryOccypied())
		{
			pb::GS2C_Hero_Move sendmsg;
			sendmsg.set_char_id(GetID());
			m_player->Send(pb::SMSG_HERO_MOVE, sendmsg);
			return;
		}
	}
#endif

	if(CurHp() == 0)
	{
		LLOG("cant move soldier empty");
		pb::GS2C_Hero_Move sendmsg;
		sendmsg.set_char_id(GetID());
		m_player->Send(pb::SMSG_HERO_MOVE, sendmsg);
		return;
	}

    m_pMap->m_cityGraph.GetCityPath(*m_pMap, m_curCityId, cityId, m_path);

	LLOG("$$$$$$$ m_curCityId=%d, cityId=%d,m_canMove=%d,m_path.size=%d",m_curCityId,cityId,m_canMove,m_path.size());

	if(m_path.size() == 0)
	{
		LLOG("path is empty!!");
		pb::GS2C_Hero_Move sendmsg;
		sendmsg.set_char_id(GetID());
		m_player->Send(pb::SMSG_HERO_MOVE, sendmsg);
		return;
	}

	if(!m_canMove)
	{
		m_canMove = true;
		ServiceMgr::Register(SERVICE_CHAR_MOVE, this);
	}

	pb::GS2C_Hero_Move sendmsg;
	std::queue<uint32> tmpqueue;
	for(uint32 i=0; i<m_path.size();)
	{
		//LLOG("$$$$$$$$$$ cityid = %d",m_path.front());
		sendmsg.add_city_id(m_path.front());
		tmpqueue.push(m_path.front());
		m_path.pop();
	}
	for(uint32 j=0; j<tmpqueue.size();)
	{
		m_path.push(tmpqueue.front());
		tmpqueue.pop();
	}
	sendmsg.set_char_id(GetID());
	m_player->Send(pb::SMSG_HERO_MOVE, sendmsg);


	//_Move();
}
void Character::_Move()
{
	//LLOG("_move once");

	//if(isInCombat) return;		//ս�������У������ƶ�

	if(CurHp() == 0) return;	//���������ƶ�

#ifdef _SERVER_CROSS_
	if(sCrossWarMgr.GetCrossState() != State_Start)
	{
		m_canMove = false;
		ServiceMgr::UnRegister(SERVICE_CHAR_MOVE, this);
	}
#endif

    if (!m_path.empty())
    {
		//LLOG("not empty path");
        uint16 destCityId = m_path.front();

        //if (FlyTo(destCityId) == false)
        //{
        //    return;
        //}

		//pb::SMSG_HERO_POS posemsg;
		if (FlyTo(destCityId,false) == false)
		{
			pb::GS2C_Hero_Pos posemsg;
			posemsg.set_char_id(GetID());
			posemsg.set_city_id(destCityId);
			posemsg.set_is_jump(false);
			posemsg.set_is_move_shutdown(true);
			m_player->Send(pb::SMSG_HERO_POS, posemsg);
			//LLOG("NNNNNNN destCityId = %d",destCityId);
			initPath();

			return;
		}
		else
		{
			//�������ĳ�����ս���У���ֹͣѰ·������shutshow
			City* pDestCity = m_pMap->GetCity(destCityId);
			if(pDestCity)
			{
				if(pDestCity->IsFighting())
				{
					pb::GS2C_Hero_Pos shutmsg;
					shutmsg.set_char_id(GetID());
					shutmsg.set_city_id(destCityId);
					shutmsg.set_is_jump(false);
					shutmsg.set_is_move_shutdown(true);
					m_player->Send(pb::SMSG_HERO_POS, shutmsg);
					//LLOG("NNNNNNN move shut down destCityId = %d",destCityId);
					initPath();

					return;
				}
			}
		}

		if(m_path.size()>0)
		{
			m_path.pop();

			if(m_path.empty())
			{
				m_canMove = false;
				ServiceMgr::UnRegister(SERVICE_CHAR_MOVE, this);
			}
		}
    }
	else
	{
		m_canMove = false;
		ServiceMgr::UnRegister(SERVICE_CHAR_MOVE, this);
	}
}

void Character::AutoMoveTo(uint32 desCity)
{
	LLOG("Character::AutoMoveTo,charid=%d,timestart =%llu",ProtoID(),sOS.TimeMS());

	if (!m_pMap) return;

	initPath();	//ֹͣ�佫�ƶ�

#ifdef _SERVER_CROSS_
	//�����������ұ�ռ���ˣ��������Զ��ƶ�
	Country* mycountry = sCrossWorldMap.GetCountry(GetCountryId());
	if(mycountry)
	{
		if(mycountry->IsCrossCountryOccypied())
			return;
	}
#endif

	if(CurHp() == 0)
	{
		LLOG("cant auto move soldier empty");
		return;
	}

	//������ս��·��
	m_pMap->m_cityGraph.GetCityPath(*m_pMap, m_curCityId, desCity, m_path);

	LLOG("$$$$$$$ m_curCityId=%d, cityId=%d,m_canMove=%d,m_path.size=%d",m_curCityId,desCity,m_canMove,m_path.size());

	//���û����ս��·�ߣ�����ս��·��
	if(m_path.size() == 0)
	{
		m_pMap->m_cityGraph.GetCityFightPath(*m_pMap, m_curCityId, desCity, m_path);
		LLOG("$$$$$$$ m_curCityId=%d, cityId=%d,m_canMove=%d,m_path.size=%d",m_curCityId,desCity,m_canMove,m_path.size());
	}

	if(m_path.size() == 0)
	{
		LLOG("auto path is empty!!");
		return;
	}

	if(!m_canMove)
	{
		m_canMove = true;
		ServiceMgr::Register(SERVICE_CHAR_AUTO_COUNTRY_WAR_MOVE, this);
	}

	pb::GS2C_Hero_Move sendmsg;
	std::queue<uint32> tmpqueue;
	for(uint32 i=0; i<m_path.size();)
	{
		//LLOG("$$$$$$$$$$ cityid = %d",m_path.front());
		sendmsg.add_city_id(m_path.front());
		tmpqueue.push(m_path.front());
		m_path.pop();
	}
	for(uint32 j=0; j<tmpqueue.size();)
	{
		m_path.push(tmpqueue.front());
		tmpqueue.pop();
	}
	sendmsg.set_char_id(GetID());
	m_player->Send(pb::SMSG_HERO_MOVE, sendmsg);

	LLOG("Character::AutoMoveTo Over,charid=%d,timeend =%llu",ProtoID(),sOS.TimeMS());
	//_Move();
}

void Character::GetNextCityId(uint32& cityid)
{
	if(m_path.size()>0)
		cityid = m_path.front();
}

void Character::_AutoCountryWarMove()
{
	//LLOG("_move once");

	if(isInCombat) return;		//ս�������У������ƶ� �ȴ���һ��update

	if(CurHp() == 0)		//�Ѿ����������ػ���
	{
		FlyTo(sWorldMap.GetCountry(GetCountryId())->m_capitalCityId,false);
		initPath();
		return;
	}
#ifdef _SERVER_CROSS_
	if(sCrossWarMgr.GetCrossState() != State_Start)
	{
		m_canMove = false;
		ServiceMgr::UnRegister(SERVICE_CHAR_AUTO_COUNTRY_WAR_MOVE, this);
	}
#endif

	if (!m_path.empty())
	{
		//LLOG("not empty path");
		uint16 destCityId = m_path.front();

		//if (FlyTo(destCityId) == false)
		//{
		//    return;
		//}

		//pb::SMSG_HERO_POS posemsg;
		if (FlyTo(destCityId,false) == false)
		{
			pb::GS2C_Hero_Pos posemsg;
			posemsg.set_char_id(GetID());
			posemsg.set_city_id(destCityId);
			posemsg.set_is_jump(false);
			posemsg.set_is_move_shutdown(true);
			m_player->Send(pb::SMSG_HERO_POS, posemsg);
			//LLOG("NNNNNNN destCityId = %d",destCityId);
			initPath();

			return;
		}
		else
		{
			////�������ĳ�����ս���У���ֹͣѰ·������shutshow
			//City* pDestCity = m_pMap->GetCity(destCityId);
			//if(pDestCity)
			//{
			//	if(pDestCity->IsFighting())
			//	{
			//		pb::GS2C_Hero_Pos shutmsg;
			//		shutmsg.set_char_id(GetID());
			//		shutmsg.set_city_id(destCityId);
			//		shutmsg.set_is_jump(false);
			//		shutmsg.set_is_move_shutdown(true);
			//		m_player->Send(pb::SMSG_HERO_POS, shutmsg);
			//		//LLOG("NNNNNNN move shut down destCityId = %d",destCityId);
			//		initPath();

			//		return;
			//	}
			//}
		}

		if(m_path.size()>0)
		{
			m_path.pop();

			if(m_path.empty())
			{
				m_canMove = false;
				ServiceMgr::UnRegister(SERVICE_CHAR_AUTO_COUNTRY_WAR_MOVE, this);
			}
		}
	}
	else
	{
		m_canMove = false;
		ServiceMgr::UnRegister(SERVICE_CHAR_AUTO_COUNTRY_WAR_MOVE, this);
	}
}

bool Character::FlyTo(uint32 cityId, bool is_jump/* = true*/)
{
    if (!m_pMap) return false;

    City* pCurCity = m_pMap->GetCity(m_curCityId);
    City* pDestCity = m_pMap->GetCity(cityId);

    if (isInCombat || !pCurCity || !pDestCity || !pCurCity->CanExit(*this) || !pDestCity->CanEnter(*this))
    {
        return false;
    }

    _ExitCity(*pCurCity); //���˳���ǰ����
    if (!pCurCity->_Maze(*this))
    {
        _EnterCity(*pDestCity,is_jump);
    }
	m_isReloadMove = true;
	LLOG("hero move success,CurCity=%d, DestCity=%d",pCurCity->GetCityId(), pDestCity->GetCityId());
    return true;
}




//���·��
void Character::initPath()
{
	std::queue<uint32> empty;
	std::swap( m_path, empty );

	if(m_canMove)
	{
		m_canMove = false;
		ServiceMgr::UnRegister(SERVICE_CHAR_MOVE, this);
		ServiceMgr::UnRegister(SERVICE_CHAR_AUTO_COUNTRY_WAR_MOVE, this);
	}
}

bool Character::SetMap(CMap* p)
{
    if (!p) return false;

    City* pCurCity = m_pMap->GetCity(m_curCityId);

    uint32 dstCityId = sMapTableMgr.GetCapitalCity(GetCountryId(), m_pMap->Tid);
    City* pDestCity = p->GetCity(dstCityId);

    if (isInCombat || !pCurCity || !pDestCity || !pCurCity->CanExit(*this) || !pDestCity->CanEnter(*this))
    {
        return false;
    }

    _ExitCity(*pCurCity);
    m_pMap = p;
    _EnterCity(*pDestCity);
    return true;
}
bool Character::_ExitCity(City& city)
{
	if (m_player)
	{
		city.HeroExit(*m_player, *this);
		return true;
	}
	return false;
}
bool Character::_EnterCity(City& city,bool is_jump/* = true*/)
{
	m_curCityId = city.id;
	//LLOG("[1002] m_curCityId=%d",m_curCityId);

    //�ȷ��佫λ�ã��ٽ���ǳأ����佫ս��״̬
    pb::GS2C_Hero_Pos msg;
    msg.set_char_id(GetID());
    msg.set_city_id(m_curCityId);
	msg.set_is_jump(is_jump);
	msg.set_is_move_shutdown(false);
    //m_player->Send(pb::SMSG_HERO_POS, msg);

	if (m_player)
	{
		m_player->Send(pb::SMSG_HERO_POS, msg);
		LLOG("hero countryid=%d, city countryid=%d, city=%d",GetCountryId(),city.GetCountryId(),city.id);
		city.HeroEnter(*m_player, *this);
		if(city.IsFighting() || city.GetCountryId() != GetCountryId())		//�ǵжԳ��л���ս���У�����Ѱ·
		{
			if(m_path.size()>0)
			{
				if(!GetIsInAutoCountryWar())
				{
					initPath();
					m_canMove = false;
					ServiceMgr::UnRegister(SERVICE_CHAR_MOVE, this);
				}
			}
		}
		return true;
	}
	return false;
}
void Character::_ReEnterCurCity()
{
    if (City* pCity = m_pMap->GetCity(m_curCityId))
    {
		if (m_player)
		{
			pCity->HeroEnter(*m_player, *this);
		}
    }
}
void Character::OnDead(CombatObj* killer)
{
    CombatObj::OnDead(killer);

#ifdef  _MMO_SERVER_
	if( killer != NULL && killer->IsPlayer())		//��ɱ�ֲ��ǻ�ӰҲ����npcʱ�����Բ�׽�����佫
	{
		if (Player* player = sPlayerPool.GetByPlayerId(killer->GetPlayerId()))
		{
			player->m_DungeonLog->CatchObj(*player, *this);
		}
	}
#endif
    if (m_IsShadow)
    {
		if (City* pCity = m_pMap->GetCity(m_curCityId))	//��Ӱ�ӳ��ж�����ɾ��
		{
			if (m_player)
			{
				LLOG("[1003] Hero Dead City %d",pCity->id);
				pCity->_DelCombatObj(*this, GetCountryId());
			}
		}
		m_deleteShadow.push_back(this);
        return;
    }

	SetCurSolderNum(CurHp());
    //�������׶�
    uint32 cityId = sMapTableMgr.GetCapitalCity(GetCountryId());
    FlyTo(cityId);

	if(GetIsInAutoCountryWar())	//���Զ���ս�У�����������Զ�����
	{
		if(isInPrison())
		{
			if(_GetEscapeTime() == 0)	//û������(�����������)
			{
				int seconds = EscapeFromDungeon();
				pb::GS2C_Dungeon_Escape msgBack;
				msgBack.set_hero_tid(GetID());
				msgBack.set_need_second(seconds);
				m_player->Send(SMSG_DUNGEON_ESCAPE, msgBack);
			}
		}
	}
}
float Character::SkillRatio()
{
    return sSkillDataMgr.GetDamageFactor(SkillID());
}
uint8 Character::SkillRange()
{
    return sSkillDataMgr.GetObjSkillRange(SkillID());
}

uint32 Character::SkillID()
{
	return m_proto.GetCharProto().skill_id;
}

uint32 Character::GetSolderRowNum()
{
	return m_proto.GetCharProto().char_row;
}

uint32 Character::GetSolderColNum()
{
	return m_CurSoliderColNum;//��ֵ
}


void Character::SetCurSolderNum(int num)
{
	if(Values().GetInt(pb::CHAR_FIELD_TROOPS) < num)	//����ֵ�������Ѫ������Ϊ���Ѫ��
	{
		MutableValues().SetInt(pb::CHAR_FIELD_CUR_TROOPS, Values().GetInt(pb::CHAR_FIELD_TROOPS));
	}
	else
	{
		MutableValues().SetInt(pb::CHAR_FIELD_CUR_TROOPS, num);
	}
}

//��ս���������
void Character::CalculateAttr(const uint32 siege_army_id)
{
	if(isFighting || isInCombat)
	{
		FreshAttr(siege_army_id);
		return;
	}

	CharacterAttr p;
	//std::auto_ptr<CharacterAttr> p(new CharacterAttr());
	if (m_player)
	{
		if(!m_player->HaveTechnology(MIJI))
			p.RecalculateAttr(*m_player->m_bag, *m_player->m_BaoQiLog, *this,m_player, siege_army_id,false);
		else
			p.RecalculateAttr(*m_player->m_bag, *m_player->m_BaoQiLog, *this,m_player,siege_army_id);
	}
	stPro.tong = (int)p.GetAttr(UNIT_TONG);        //ͳ
	stPro.yong = (int)p.GetAttr(UINT_YONG);        //��
	stPro.attack = (int)p.GetAttr(UNIT_ATTACK);    //����
	stPro.defence = (int)p.GetAttr(UNIT_DEFENSE);  //����
	stPro.skillAttack = (int)p.GetAttr(UNIT_BROKEN_ARRAY); //��������ս��������
	stPro.skillDefence = (int)p.GetAttr(UNIT_IMPECCABLE);  //��и������ս��������
	stPro.attackEx = (int)p.GetAttr(UNIT_PUNCTURE);        //���̣�����ʱ�����˺�
	stPro.defenceEx = (int)p.GetAttr(UNIT_PARRY);          //�񵲣������������˺�
	stPro.rowSoldierMaxCnt = (int)p.GetAttr(UNIT_TROOPS) * GetSolderColNum();//(һ����������)
	//ReFreshCombatData();
	int nCurHp = CurHp();
	soldiers.clear();
	for (uint32 i = 0; i < m_player->GetCurSolderRowNum(); ++i) {
		if (nCurHp > stPro.rowSoldierMaxCnt)
		{
			Combat::SoldierRaw soldierone;
			soldierone.hp = stPro.rowSoldierMaxCnt;
			soldiers.push_back(soldierone);
			nCurHp -= stPro.rowSoldierMaxCnt;
		}
		else if (nCurHp > 0)
		{
			Combat::SoldierRaw soldierone;
			soldierone.hp = nCurHp;
			soldiers.insert(soldiers.begin(), soldierone);
			nCurHp = 0;
			break;
		}
		else
		{
			break;
		}
	}
	SetCurSolderNum(CurHp());
	if(m_objType == Combat::Obj_Char)
		m_player->setMilitaryPower();
}

//ս���������
void Character::FreshAttr(const uint32 siege_army_id)
{
	CharacterAttr p;
	//std::auto_ptr<CharacterAttr> p(new CharacterAttr());
	if (m_player)
	{
		if(!m_player->HaveTechnology(MIJI))
			p.RecalculateAttr(*m_player->m_bag, *m_player->m_BaoQiLog, *this,m_player,siege_army_id, false);
		else
			p.RecalculateAttr(*m_player->m_bag, *m_player->m_BaoQiLog, *this,m_player,siege_army_id);
	}

	//stPro.tong = (int)p->GetAttr(UNIT_TONG);        //ͳ
	//stPro.yong = (int)p->GetAttr(UINT_YONG);        //��
	//stPro.attack = (int)p->GetAttr(UNIT_ATTACK);    //����
	//stPro.defence = (int)p->GetAttr(UNIT_DEFENSE);  //����
	//stPro.skillAttack = (int)p->GetAttr(UNIT_BROKEN_ARRAY); //��������ս��������
	//stPro.skillDefence = (int)p->GetAttr(UNIT_IMPECCABLE);  //��и������ս��������
	//stPro.attackEx = (int)p->GetAttr(UNIT_PUNCTURE);        //���̣�����ʱ�����˺�
	//stPro.defenceEx = (int)p->GetAttr(UNIT_PARRY);          //�񵲣������������˺�
	//stPro.rowSoldierMaxCnt = (int)p->GetAttr(UNIT_TROOPS) * GetSolderColNum();//(һ����������)
}

void Character::InitCombatData()
{
    CombatObj::InitCombatData();
	CalculateAttr();
}

void Character::ReFreshCombatData(const uint32 siege_army_id)
{
	//int nCurHp = CurHp();
	//CombatObj::InitCombatData();
	CalculateAttr(siege_army_id);
	
}

int Character::GetMaxHp()
{
	return m_player->GetCurSolderRowNum() * stPro.rowSoldierMaxCnt;
}

int Character::GetZhaoMuNeedCost() const
{
	return m_proto.GetZhaoMuCost();
}

uint32 Character::NeedHp()
{
	if (isInCombat)
	{
		return 0;
	}
	int nMaxHp = GetMaxHp();
	int nCurHp = CurHp();
	
	SetCurSolderNum(nCurHp);		//У׼

	if (nCurHp < nMaxHp)
	{
		return nMaxHp - nCurHp;
	}
	else
		return 0;
}

uint32 Character::RecoverHp(uint32 solders)
{
	if (isInCombat || solders == 0)
	{
		return 0;
	}
	int nHpRecoverUsed = 0;
	int nMaxHp = GetMaxHp();
	int nCurHp = CurHp();
	SetCurSolderNum(CurHp());	//У׼һ��
	if (nCurHp < nMaxHp)
	{
		int nNeedHp = nMaxHp - nCurHp;	//��Ҫ�ָ���hp
		if ((int)solders > nNeedHp)		//�������ӵ�hp ���� ��Ҫ�ָ��ģ���ȫ���ָ�
		{
			RecoverFullSoldier();
			nHpRecoverUsed = nNeedHp;
		}
		else			//�������ӵĲ���������ȫ���ָ�
		{
			int nRemainAddHp = (int)solders;	//ʣ��������ӵ�hp
			for (vector<Combat::SoldierRaw>::reverse_iterator it = soldiers.rbegin(); it != soldiers.rend(); ++it)
			{
				int rowHp = it->hp;
				if (rowHp < stPro.rowSoldierMaxCnt)		//���ű���δ��
				{
					if (nRemainAddHp > 0)	//��ʣ���hp��
					{
						if (nRemainAddHp >= (stPro.rowSoldierMaxCnt - rowHp))
						{
							nRemainAddHp -= (stPro.rowSoldierMaxCnt - rowHp);
							it->hp = stPro.rowSoldierMaxCnt;
						}
						else
						{
							it->hp = nRemainAddHp + rowHp;
							nRemainAddHp = 0;
						}
					}
					else
					{
						break;
					}
				}
			}

			uint32 curRows = (uint32)soldiers.size();	//��������
			while (nRemainAddHp > 0 && curRows < m_player->GetCurSolderRowNum())
			{
				if (nRemainAddHp >= stPro.rowSoldierMaxCnt)
				{
					Combat::SoldierRaw soldierone;
					soldierone.hp = stPro.rowSoldierMaxCnt;
					soldiers.push_back(soldierone);
					nRemainAddHp -= stPro.rowSoldierMaxCnt;
				}
				else
				{
					Combat::SoldierRaw soldierone;
					soldierone.hp = nRemainAddHp;
					soldiers.insert(soldiers.begin(),soldierone);
					nRemainAddHp = 0;
				}
				curRows = (uint32)soldiers.size();
			}

			nHpRecoverUsed = (int)solders - nRemainAddHp;
			
			SetCurSolderNum(CurHp());
		}
	}

	return nHpRecoverUsed;
}

void Character::SetDungeonOwner(uint64 playerId)
{
    m_DungeonOwner = playerId;

    pb::GS2C_HERO_BE_CATCH msg;
    msg.set_hero_tid(GetID());
    msg.set_dungeon_owner_player_id(playerId);
	if (m_player)
	{
		m_player->Send(pb::SMSG_HERO_BE_CATCH, msg);
	}
}

bool Character::isInPrison()
{
	return m_DungeonOwner>0;
}
int Character::EscapeFromDungeon()
{
#ifdef  _MMO_SERVER_
    if (Player* player = sPlayerPool.GetByPlayerId(m_DungeonOwner))
    {
		int seconds = player->m_DungeonLog->GetEscapeSecond();		//��������ʱ��

		if(_GetEscapeTime()>0)		//������ʱ�䣬�ٴ�����Ϊ��������
		{
			if(_GetEscapeTime() < sOS.TimeSeconds())	//��ʱ�����Ѿ����ܵ���
			{
				player->m_DungeonLog->Escape(player,this,_GetEscapeTime());
				_SetEscapeTime(0);
				return 0;
			}
			else		//��û�����ܣ���۳�5���Ȼ����������
			{
				if(m_player->EnoughCurrency(eSysGold,GET_BASE_DEF_INT(pb::BD_ESCAPE_AT_ONCE_COST_DIAMOND)))		//����㹻
				{
					m_player->TryDeductCurrency(IR_INSTANT_ESCAPE_COST,eSysGold,GET_BASE_DEF_INT(pb::BD_ESCAPE_AT_ONCE_COST_DIAMOND));
					player->m_DungeonLog->Escape(player,this,_GetEscapeTime());
					return 0;
				}
				else
					return (int)(sOS.TimeSeconds() - _GetEscapeTime());
			}
		}

		player->m_DungeonLog->TryPrisonerEscape(player, this);

        return seconds;
    }
	SetDungeonOwner(0);
    //MMO_ASSERT(0);
#endif
    return 0;
}
Character* Character::CreateShadow() const
{
    if (Character* pShadow = new Character(*this))
    {
        pShadow->m_IsShadow = true;
        pShadow->InitCombatData();
        pShadow->RecoverFullSoldier();
		m_player->AddValue(pb::PLAYER_FIELD_CREATESHADOW_NUM,1);
		if(m_player->IsActivityStillActive(pb::ACTIVITY_ADD_SOLIDER_RANK))
		{
			m_player->AddValue(pb::PLAYER_FIELD_ACTIVITY_ADD_SOLDIER,1);
#ifdef _MMO_SERVER_
			sActivityMgr.MarkTakeInActivity(pb::ACTIVITY_OCCUPATION_RANK,m_player->GetGuid());
#endif
		}
        return pShadow;
    }
    return NULL;
}
void Character::_DeleteShadow()
{
	for (std::vector<Character*>::iterator it = m_deleteShadow.begin(); it != m_deleteShadow.end(); ++it)
	{
		delete *it;
	}
	m_deleteShadow.clear();
}
void Character::DeleteShadow(Character* p)
{
	m_deleteShadow.push_back(p);
}
bool Character::RecoverFullSoldier()
{
    if (isInCombat) return false;

    soldiers.clear();

	for (uint32 i = 0; i < m_player->GetCurSolderRowNum(); ++i) {
		Combat::SoldierRaw soldierone;
		soldierone.hp = stPro.rowSoldierMaxCnt;
        soldiers.push_back(soldierone);
    }
	SetCurSolderNum(CurHp());
	return true;
}

//bool Character::CleanAllSoldiers()
//{
//	soldiers.clear();
//	SetCurSolderNum(0);
//	return true;
//}

bool Character::IsFullHp()
{
	return CurHp() >= GetMaxHp();
}

void Character::ClearCombatStatus()
{
	 CombatObj::ClearCombatStatus();
	 SetCurSolderNum(CurHp());
	 CalculateAttr();
}

void Character::InitCombatDataSecond(uint32 geography)
{
	m_hasShodowTactic = false;
	CombatObj::InitCombatDataSecond(geography);
}

//��������һ��ǻ�Ӱ�����ո߼�npc��ս��
bool Character::NewAutoTactic(Combat::CombatType cTyp/* = Combat::Country*/, Combat::TacticEnum tTyp/* = Combat::Attack*/)
{
	bool isSkillEnable = false;
	if(false == GetIsSelectTactic())	// ��ǰʿ����û��ѡս��
	{
		if(leftSkillTimes <= 0)		//����ʩ�Ŵ���û����
			isSkillEnable = false;
		else
			isSkillEnable = true;
	}
	else		//ʿ���Ѿ�ѡ����ս��
	{
		if(tactic == Combat::Skill_Tactic && (GetIsUseSkill() || leftSkillTimes <= 0))	//���ʿ��ѡ����Ǽ��ܣ����ҵ�һ���Ѿ��ù�һ�μ��ܣ�����ʩ�ż���
			isSkillEnable = false;
		else
			return true;	//�Ѿ�ѡ����ս��������������
	}

	if(isSkillEnable)		//����ѡ����
	{
		if(cTyp == Combat::CT_Country || cTyp == Combat::CT_Fuben || cTyp == Combat::CT_Teach_Map)	//����ǹ�ս����ѧ������ ֱ�ӷż���
		{
			SelectTactic(GetFinalTacticOrSkill(true,tTyp,1));
		}
		else	//���һ�зż���
		{
			if(soldiers.size() == 1)	//ֻʣһ����
			{
				SelectTactic(GetFinalTacticOrSkill(true,tTyp,1));
			}
			else		//��ֹһ����ѡ��ս��
			{
				if(IsShadow())	//����ǻ�Ӱ����̶�ս��
				{
					if(!m_hasShodowTactic)	//��Ӱ��û��ѡ��ȷ���̶�ս��
						m_shadowTactic = GetFinalTacticOrSkill(false,tTyp,1);
					m_hasShodowTactic = true;
					SelectTactic(m_shadowTactic);
				}
				else
				{
					SelectTactic(GetFinalTacticOrSkill(false,tTyp,1));
				}
			}
		}
	}
	else					//����ѡ����
	{
		if(IsShadow())	//����ǻ�Ӱ����̶�ս��
		{
			if(!m_hasShodowTactic)	//��Ӱ��û��ѡ��ȷ���̶�ս��
				m_shadowTactic = GetFinalTacticOrSkill(false,tTyp,1);
			m_hasShodowTactic = true;
			SelectTactic(m_shadowTactic);
		}
		else
		{
			SelectTactic(GetFinalTacticOrSkill(false,tTyp,1));
		}
	}
	return true;
}

void Character::reCalcPower(Combat::CombatType type/* = CT_Country*/)
{
	//���¼���Ƽ�Ӱ���ս�����������
	keji_deskillPercent = 0;
	keji_fightPercent = 0;
	keji_expPercent = 0;
	keji_skillPercent = 0;
	crossDoubleKill = 0;
	const TecTable* tec = NULL;

	//���¼����������õ�Ч��
	int trainFightPercent = 0;
	int trainSkillPercent = 0;
	int trainExpPercent = 0;
	int trainDoubleKill = 0;

	//Ĭ�ϿƼ��������ڻ�Ӱ���� Ĭ������������ڻ�Ӱ����
	if(!IsShadow())
	{
		if(m_player->HaveTechnology(FIGHT_ENFORCE3))
		{
			tec = sTechnologyTableMgr.GetTable(FIGHT_ENFORCE3);
			keji_skillPercent  = tec->Value1() ;	//����ս����10%		���ܼ���
		}
		if(m_player->HaveTechnology(FIGHT_ENFORCE6))
		{
			tec = sTechnologyTableMgr.GetTable(FIGHT_ENFORCE6);
			keji_deskillPercent = tec->Value1();	//����ս����5%	���ܼ���
		}
		if(m_player->HaveTechnology(EXP_UP_1))
		{
			tec = sTechnologyTableMgr.GetTable(EXP_UP_1);
			keji_expPercent +=  tec->Value1() ;//����ս����50%		��������
		}

		if(type == Combat::CT_Country)
		{
			if(m_player->HaveTechnology(EXP_UP_5)){
				tec = sTechnologyTableMgr.GetTable(EXP_UP_5);
				keji_expPercent +=  tec->Value1();//��ս��30%		��������
			}
			if(m_player->HaveTechnology(EXP_UP_3))
			{
				tec = sTechnologyTableMgr.GetTable(EXP_UP_3);
				keji_expPercent +=  tec->Value1() ;//��ս��50%		��������
			}
			crossDoubleKill = (m_player->GetCrossDoubleKill() - 1)*100; //��ս ���˫����ɱ

			trainFightPercent += fightPercent;
			trainSkillPercent += skillPercent;
			trainExpPercent += expPercent;
			trainDoubleKill += doubleKill;
		}
	}

	total_fightPercent = trainFightPercent ;
	total_expPercent = trainExpPercent + keji_expPercent;
	total_doubleKill = MIN(trainDoubleKill + crossDoubleKill,100);
	total_reduceCostHP = reduceCostHP;
	total_skillPercent = trainSkillPercent + keji_skillPercent;
	total_deSkillPercent = keji_deskillPercent;
}

bool Character::CharacterMoved()
{
	return m_isReloadMove;
}

int Character::GetTotalFightPercent()
{
	reCalcPower();
	return total_fightPercent;
}

int Character::GetTotalSkillPercent(const Combat::CombatType type,const uint32 obj_type/* = 0*/)
{
	reCalcPower(type);
	return total_skillPercent;
}
int Character::GetTotalExpPercent(Combat::CombatType type )
{
	reCalcPower(type);
	return total_expPercent;
}
int Character::GetTotalReduceCostHP()
{
	reCalcPower();
	return total_reduceCostHP;
}
int Character::GetTotalDoubleKill(Combat::CombatType type)
{
	reCalcPower(type);
	return total_doubleKill;
}
int Character::GetTotalDeSkillPercent()
{
	reCalcPower();
	return total_deSkillPercent;
}

bool Character::CharIsIdle()
{
	return !isFighting && !isInCombat  && !isInPrison() && !isInTraining && !m_isTuntian;
}

bool Character::GetIsTunTian()
{
	return m_isTuntian;
}

void Character::SetIsTunTian( bool is_tuntian )
{
	m_isTuntian = is_tuntian;
}

uint32 Character::GetPlayerSolderRowNum()
{
	return m_player->GetCurSolderRowNum();
}

int32 Character::GetlvMaxXp(const int32 level)
{
	int32 max_xp = 0;
	m_proto.TryGetUpgradeXp(level,max_xp);
	return max_xp;
}

void Character::AddSkillPercentBySeigeArmySpecificId(int add_percent, int enemyType/* = 0*/)
{
	//total_fightPercent += add_percent;
}

uint32 Character::GetType() const
{
	return m_proto.CharType();
}

uint32 Character::CalculateCharacterMilitaryPower() const
{
	uint32 char_military_power = 0;
	for(int i = pb::CHAR_FIELD_TROOPS;i <= CHAR_FIELD_YONG;++i)
	{
		float a = sCharacterMgr.GetFightValueByAttrId(i);
		int j = m_values.GetInt(i);
		float temp_military_power = a * j;
		char_military_power += static_cast<uint32>(temp_military_power);
	}
	return char_military_power;
}

uint32 Character::GetCharacterMilitaryPower() const
{
	return m_values.GetInt(pb::CHAR_FIELD_MILITARY_POWER);
}

uint32 Character::GetTerrDebuffAddtion(uint32 terrType)
{
	if(m_proto.GetKeZhiJiaCheng3() > 0)
		return m_proto.GetKeZhiJiaCheng3();

	if(terrType == (uint32)Combat::Terr_City)
		return m_proto.GetKeZhiJiaCheng1();
	else
		return m_proto.GetKeZhiJiaCheng2();
}

uint32 Character::GetPlayerLevel()
{
	return m_player->GetLevel();
}

double Character::GetTerrTalentAddtion(uint32 terrType)
{
	std::string terrTalent = m_proto.GetTerrTalent();
	std::vector<int> terrTalentVec;
	Utility::SplitStr(terrTalent, terrTalentVec, ',');
	for(std::vector<int>::iterator it = terrTalentVec.begin(); it != terrTalentVec.end(); it++)
	{
		if(*it == terrType)
			return m_proto.GetTerrTalentBuff();
	}
	return 0;
}