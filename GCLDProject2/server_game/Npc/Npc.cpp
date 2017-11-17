#include "Npc.h"
#include "OS.h"
#ifdef  _MMO_SERVER_
#include "../GameServer.h"
#include "Dungeon/PlayerDungeonLog.h"
#endif
#include "../Map/city/city.h"
#include "../Map/Table/MapTableMgr.h"
#include "Table/NpcTableMgr.h"
#include "datastruct/struct_npc.h"
#include "../Map/country/country.h"
#include "../Service/ServiceMgr.h"
#include "session/PlayerPool.h"
#include "object/Player.h"

#include "../Map/base/BaseMap.h"
#include "utility/Utility.h"
#include "../Combat/combat_define.h"
#include "Combat/SkillDataMgr.h"
#include "World.pb.h"
#include "../Map/WorldMap.h"
#include "Opcode.pb.h"

std::set<Npc*> Npc::m_deleteNpc;

Npc::Npc(const NpcProto& proto)
    : m_npcProto(proto)
    , m_countryId(0)
    , m_curCityId(0)
    , m_level(0)
	, m_unique_id(0)
	, m_canMove(false)
{
	_CreateUniqueId();
}
Npc::~Npc()
{
	//LLOG("[1003] fresh NPC class");
    ServiceMgr::UnRegister(SERVICE_NPC_MOVE, this);
}
uint32 _Service_NpcMove(void* p){
    Npc* pNpc = (Npc*)p;
    pNpc->_Move();
    return GET_BASE_DEF_INT( pb::BD_EXPEDITION_TROOPS_MOVE_INTERVAL_TIME) * 1000; // 1min loop
}
void Npc::_Move()
{
	if (isFighting || isInCombat) return; //����ս���У������ƶ�
	
	if(soldiers.empty())
	{
		PLOG("[1000] IMPORTANT BUG!!! SOLDER IS DEAD TO MOVE!!!");
		//OnDead(NULL);
		//ServiceMgr::UnRegister(SERVICE_NPC_MOVE, this);
		return;
	}

    if (m_path.size() > 0)
    {
		//LLOG("Npc::_Move path>0,m_curCityId=%d,desciti=%d",m_curCityId,m_path.front());
        uint16 destCityId = m_path.front();

        City* pCurCity = m_pMap->GetCity(m_curCityId);
        City* pDestCity = m_pMap->GetCity(destCityId);

        if (isInCombat || !pCurCity || !pDestCity || !pCurCity->CanExit(*this) || !pDestCity->CanEnter(*this))
        {
			LLOG("move failed!!");
            return;
        }
#ifdef _MMO_SERVER_
		if(isExpedition)
		{
			if(!sWorldMap.isExpeditionMoveTo(expeid,destCityId))
			{
				sWorldMap.setExpeditionMoveTo(expeid,destCityId);
				pb::GS2C_Expedition_Army_Info sendmsg;
				sendmsg.set_expedition_id(expeid);
				sendmsg.set_city_id(destCityId);
				sendmsg.set_hero_id(GetTableId());
				sendmsg.set_org_city_id(m_curCityId);
				sWorldMap.BroadcastInCountry(pb::SMSG_NPC_POS_LIST,sendmsg,GetCountryId());
			}
		}
#endif
        _ExitCurCity(*pCurCity); //���˳���ǰ����
		if (!pCurCity->_Maze(*this))
		{
			_EnterCity(*pDestCity);
		}
        m_path.pop();
    }
}
void Npc::MoveTo(uint32 cityId)
{
    if (!m_pMap) return;

	if (!m_canMove) {
		m_canMove = true;
		ServiceMgr::Register(SERVICE_NPC_MOVE, this);	//һ����ִ��һ��move
	}
    m_pMap->m_cityGraph.GetCityPath(*m_pMap, m_curCityId, cityId, m_path);
}
bool Npc::_ExitCurCity(City& city)
{
    city.NpcExit(*this);
    return true;
}
bool Npc::_EnterCity(City& city)
{
    m_curCityId = city.id;

    city.NpcEnter(*this);

    return true;
}
void Npc::OnDead(CombatObj* killer)
{
    CombatObj::OnDead(killer);
#ifdef  _MMO_SERVER_
	if( killer != NULL && killer->IsPlayer() && !killer->IsShadow() )		//��ɱ�ֲ��ǻ�ӰҲ����npcʱ�����Բ�׽�����佫
	{
		if (Player* player = sPlayerPool.GetByPlayerId(killer->GetPlayerId()))
		{
			player->m_DungeonLog->CatchObj(*player, *this);
		}
	}
#endif
    if (m_pMap)
    {
        if (City* city = m_pMap->GetCity(m_curCityId))
        {
            city->NpcExit(*this);

            //if (city->IsMonsterCity())
            //{
            //    RecoverFullSoldier();
            //    city->NpcEnter(*this);
            //    return;
            //}
        }
    }

	if(isExpedition)
	{
		Country* country = sWorldMap.GetCountry(m_countryId);
		if(country)
		{
			for(std::vector<uint32>::iterator it = (country->m_npc_expedirion[expeid]).begin(); it != (country->m_npc_expedirion[expeid]).end(); it++)
			{
				if( *it == m_unique_id )
				{
					(country->m_npc_expedirion[expeid]).erase(it);
					break;
				}
			}
			if((country->m_npc_expedirion[expeid]).empty())
			{
				pb::GS2C_Delete_Npc_World sendmsg;
				sendmsg.set_expeid(expeid);
				sendmsg.set_city_id(m_curCityId);
				sWorldMap.Broadcast(pb::SMSG_DELETE_NPC_WORLD,sendmsg);
			}
		}

	}

    //delete this; //Notice��ֱ��delete����������pNpc�ͺ�Σ����
    m_deleteNpc.insert(this);
}
float Npc::SkillRatio()
{
    return sSkillDataMgr.GetDamageFactor(SkillID());
}
uint8 Npc::SkillRange()
{
    return sSkillDataMgr.GetObjSkillRange(SkillID());
}

uint32 Npc::SkillID()
{
	return m_npcProto.skill_id;
}

int32 Npc::GetLevel() const
{
    if (m_pMap)
    {
        if (Country* pCountry = m_pMap->GetCountry(m_countryId))
        {
            return pCountry->m_countryNpcLv;
        }
    }
    return m_level;
}
const string& Npc::GetPlayerName()
{
    static string s_name;
    return s_name;
}
uint32 Npc::GetTableId()
{
	return m_npcProto.Id;
}
uint32 Npc::GetMaxRow()
{
	return m_npcProto.char_row;
}
void Npc::InitCombatData()
{
	Combat::CombatObj::InitCombatData();
    sNpcTableMgr.RecalculateAttr(m_npcProto.Id, GetLevel());
    stPro.tong = (int)sNpcTableMgr.GetAttr(UNIT_TONG);  //ͳ
    stPro.yong = (int)sNpcTableMgr.GetAttr(UINT_YONG);         //��
    stPro.attack = (int)sNpcTableMgr.GetAttr(UNIT_ATTACK);   //����
    stPro.defence = (int)sNpcTableMgr.GetAttr(UNIT_DEFENSE);     //����
    stPro.skillAttack = (int)sNpcTableMgr.GetAttr(UNIT_BROKEN_ARRAY);//��������ս��������
    stPro.skillDefence = (int)sNpcTableMgr.GetAttr(UNIT_IMPECCABLE);//��и������ս��������
    stPro.attackEx = (int)sNpcTableMgr.GetAttr(UNIT_PUNCTURE);         //���̣�����ʱ�����˺�
    stPro.defenceEx = (int)sNpcTableMgr.GetAttr(UNIT_PARRY);//�񵲣������������˺�
    stPro.rowSoldierMaxCnt = (int)sNpcTableMgr.GetAttr(UNIT_TROOPS) * 3;//(һ����������)
}

void Npc::InitCombatDataSecond(uint32 geography)
{
	CombatObj::InitCombatDataSecond(geography);
}

void Npc::RecoverFullSoldier()
{
	if (isInCombat) return;

	soldiers.clear();

	for (uint32 i = 0; i < m_npcProto.char_row; ++i) {
		Combat::SoldierRaw soldierone;
		soldierone.hp = stPro.rowSoldierMaxCnt;
		soldiers.push_back(soldierone);
	}
}
void Npc::_CreateUniqueId()
{
    static uint16 s_auto_id = 0;

    m_unique_id = ((++s_auto_id) << 16) + m_index;
}
Npc* Npc::FindNpc(uint32 uniqueId)
{
    int idx = uniqueId & 0xFFFF;

    if (Npc* ret = Npc::FindByIdx(idx))
        if (ret->m_unique_id == uniqueId)
            return ret;

    return NULL;
}
void Npc::_DeleteNpc()
{
    for (std::set<Npc*>::iterator it = m_deleteNpc.begin(); it != m_deleteNpc.end(); ++it)
    {
        delete *it;
    }
    m_deleteNpc.clear();
}
void Npc::DeleteNpc(Npc* p)
{
    m_deleteNpc.insert(p);
}

uint32 Npc::GetEnableTactics(uint32 geography /* = 0 */)
{
	int retnum = 7;
	LLOG("Npc::GetEnableTactics geography=%d",geography);
	//�������ս��
	IntPairVec::iterator intTacs = m_enableTacs.begin();
	for(; intTacs != m_enableTacs.end(); intTacs++)
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

void Npc::SetEnableTactic(std::string& tacs)
{
	std::vector<std::string> tacticstrs;
	Utility::SplitStr(m_npcProto.teshuzhanshu, tacticstrs, ',');
	{
		for(std::vector<std::string>::iterator ite = tacticstrs.begin(); ite != tacticstrs.end(); ite++)
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

bool Npc::NewAutoTactic(Combat::CombatType cTyp/* = Combat::Country*/, Combat::TacticEnum tTyp/* = Combat::Attack*/)
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
			SelectTactic(GetFinalTacticOrSkill(true,tTyp,m_tmpAiLevel));
		}
		else	//���һ�зż���
		{
			if(soldiers.size() == 1)	//ֻʣһ����
			{
				SelectTactic(GetFinalTacticOrSkill(true,tTyp,m_tmpAiLevel));
			}
			else		//��ֹһ����ѡ��ս��
			{
				SelectTactic(GetFinalTacticOrSkill(false,tTyp,m_tmpAiLevel));
			}
		}
	}
	else					//����ѡ����
	{
		SelectTactic(GetFinalTacticOrSkill(false,tTyp,m_tmpAiLevel));
	}
	return true;
}

void Npc::SetMovePath(std::vector<int>& path)
{
	std::queue<uint32> empty;
	std::swap( m_path, empty );

	for(std::vector<int>::iterator it = path.begin();
			it != path.end(); it++)
	{
		m_path.push(*it);
	}

	if (!m_pMap) return;

	if (!m_canMove) {
		m_canMove = true;
		ServiceMgr::Register(SERVICE_NPC_MOVE, this);	//һ����ִ��һ��move
	}
}

uint32 Npc::GetExtraAwardID()
{
	return m_npcProto.extra_resouce_id;
}

uint32 Npc::GetExtraAwardNum()
{
	return m_npcProto.extra_resouce_num;
}

void Npc::reCalcPower()
{
	return;
}

int Npc::GetTotalFightPercent()
{
	return 0;
}
 int Npc::GetTotalSkillPercent(const Combat::CombatType type,const uint32 obj_type/* = 0*/)
 {
	 return 0;
 }
 int Npc::GetTotalExpPercent(Combat::CombatType type )
 {
	 return 0;
 }
 int Npc::GetTotalReduceCostHP()
 {
	 return 0;
 }
 int Npc::GetTotalDoubleKill(Combat::CombatType type)
 {
	 return 0;
 }
 int Npc::GetTotalDeSkillPercent()
 {
	 return 0;
 }

 uint32 Npc::GetType() const
 {
	return m_npcProto.army_type;
 }

 void Npc::AddSkillPercentBySeigeArmySpecificId(int add_percent, int enemyType/* = 0*/)
 {
	(void)add_percent;
 }

 uint32 Npc::GetTerrDebuffAddtion(uint32 terrType)
 {
	if(m_npcProto.kezhijiacheng3 > 0)
		return m_npcProto.kezhijiacheng3;

	if(terrType == (uint32)Combat::Terr_City)
		return m_npcProto.kezhijiacheng1;
	else
		return m_npcProto.kezhijiacheng2;
 }

 uint32 Npc::GetPlayerLevel()
 {
	return GetLevel();
 }

 double Npc::GetTerrTalentAddtion(uint32 terrType)
 {
	 std::string terrTalent = m_npcProto.tian_fu;
	 std::vector<int> terrTalentVec;
	 Utility::SplitStr(terrTalent, terrTalentVec, ',');
	 for(std::vector<int>::iterator it = terrTalentVec.begin(); it != terrTalentVec.end(); it++)
	 {
		 if(*it == terrType)
			 return m_npcProto.tian_fu_buff;
	 }
	 return 0;
 }

 int Npc::GetMaxHp()
 {
	return GetMaxRow() * stPro.rowSoldierMaxCnt;
 }