/************************************************************************/
/*Add by:zhoulunhao													*/
/*Email	:zhoulunhao@hotmail.com											*/
/************************************************************************/

#include "PlayerTechnology.h"
#include "OS.h"
#include "Table/TechnologyTableMgr.h"
#include "../reward/reward.h"
#ifdef _MMO_SERVER_
#include "../AutomaticUpdate/AutoUpdateGame.h"
#endif
#include "Technology.pb.h"
#include "../object/Player.h"
#include "Opcode.pb.h"
#include "../CrossLogic/PlayerTechnologyDB.h"
#include "../Trigger/trigger.h"
#include "BaseDefine.pb.h"
#include "Technology.pb.h"
#include "character/CharacterStorage.h"
#include "Map/PlayerMapLogic.h"
#include "Map/Table/MapCity.h"
#include "Map/country/country.h"
#include "Market/PlayerMarketLog.h"
#include "BaseDefine.pb.h"
#include "BaseDefineMgr.h"
#include "Exploit/ExploitMgr.h"
#include "Character.pb.h"
#include "../Building/BuildingLogMgr.h"

#define TECHNOLOGY_SPEED_UP (GET_BASE_DEF_UINT(pb::BD_ACCELERATE_REBUILD_TIME_COST) * 60)

bool PlayerTechnology::TechIdIsInvalid(const uint32 tech_id)
{
	return m_StudyInfo.find(tech_id) == m_StudyInfo.end();
}

PlayerTechnology::PlayerTechnology()
{ 
	m_StudyInfo.clear();
	const TechnologyMap& tec_map = sTechnologyTableMgr.GetTechnologyMap();
	for(TechnologyMap::const_iterator iter = tec_map.begin();iter != tec_map.end();++iter)
	{
		TechnologyStudyStruct& ref = m_StudyInfo[iter->first];
		::memset(&ref,0,sizeof(TechnologyStudyStruct));
	}
}

//��m_StudyInfo�е����ݱ����msg
void PlayerTechnology::SaveTo(pb::GxDB_Technology_All_Info& msg)
{
    for (StudyMap::iterator it = m_StudyInfo.begin(); it != m_StudyInfo.end(); ++it)
    {
        pb::TechnologyStudy* pInfo = msg.add_study_info();
        pInfo->set_tec_id(it->first);
        pInfo->set_star_cnt(it->second.starNum);
        pInfo->set_end_time(it->second.endTime);
		pInfo->set_is_finish_study(it->second.is_finished);
    }
}

void PlayerTechnology::SaveTo(Player& player, pb::GS2C_Technology_All_Info& msg)
{
	for (StudyMap::iterator it = m_StudyInfo.begin(); it != m_StudyInfo.end(); ++it)
	{
		const TecTable* table = sTechnologyTableMgr.GetTable(it->first);
		if(table)
		{
			if(sTrigger.Check(&player, table->m_Trigger))
			{
				pb::TechnologyStudy* pInfo = msg.add_study_info();
				pInfo->set_tec_id(table->TypeId());//type_id
				pInfo->set_star_cnt(it->second.starNum);
				pInfo->set_end_time(it->second.endTime);
				pInfo->set_is_finish_study(it->second.is_finished);
				pInfo->set_table_id(table->Id());
				//NLOG("tec Id = %u,star_cnt = %u,end_time = %u,is_finish_study = %u,type_id = %u",table->Id(),it->second.starNum,(uint32)it->second.endTime,it->second.is_finished,table->TypeId());
			}
		}
	}
}

void PlayerTechnology::SaveTo(pb::GS2C_Technology_Finish& msg)
{
	msg.set_result(pb::FinishComplete);
}

//��msg�е����ݱ��浽����m_StudyInfo
void PlayerTechnology::LoadFrom(const pb::GxDB_Technology_All_Info& msg)
{
    for (int i = 0; i < msg.study_info_size(); ++i)
    {
        const pb::TechnologyStudy& info = msg.study_info(i);
        TechnologyStudyStruct& ref = m_StudyInfo[info.tec_id()];
        ref.starNum = info.star_cnt();
        ref.endTime = info.end_time();
		ref.is_finished = info.is_finish_study();
    }
}

//��ɶ������Ƿ���ָ�����о�
bool PlayerTechnology::Have(uint32 id)
{
	if(TechIdIsInvalid(id)) return false;
	TechnologyStudyStruct& ref = m_StudyInfo[id];
	return ref.is_finished == 1;
}
//����о������о���������о����ݿ��У�������о��б����о���
bool PlayerTechnology::Finish(Player* player, uint32 id) 
{
	if(TechIdIsInvalid(id)) return false;
	TechnologyStudyStruct& ref = m_StudyInfo[id];
	if(ref.is_finished == 1)
	{
		return false;
	}
	ref.is_finished = 1;
	PlayerTechnologyDB::UpdateStudyInfoToDb(*player,id,ref);
	OnAward(*player,id);
	pb::GS2C_Technology_Finish msg;
	SaveTo(msg);
	msg.set_id(id);
	player->Send(pb::SMSG_TECHNOLOGY_FINISH_RSP, msg);
	pb::GS2C_Technology_All_Info all_info_msg;
	SaveTo(*player, all_info_msg);
	player->Send(pb::SMSG_TECHNOLOGY_ALL_INFO, all_info_msg);
	return true;

}

//����
void PlayerTechnology::UpStar(Player& player, uint32 id, pb::GS2C_Technology_UpStar& msg) 
{
	if(TechIdIsInvalid(id))
	{
		msg.set_result(pb::UpStarIdInvalid); return;
	}
	if(Have(id))
	{
		msg.set_result(pb::UpStarAlreadyDown); return;
	}
	else
	{
		const TecTable* table = sTechnologyTableMgr.GetTable(id);
		if (LIKELY(table))		//����id���excel���е�һ��
		{
			if (LIKELY(sTrigger.Check(&player, table->m_Trigger)))	//���㴥������
			{
				TechnologyStudyStruct& ref = m_StudyInfo[id];	
				if(LIKELY(ref.starNum < table->NeedStarCnt()))		//�Ѿ����Ǹ���С����Ҫ���Ǹ���
				{
					if (LIKELY(sReward.Change(player,table->m_StarCost)))		//�۳����ǿ���
					{
						++ref.starNum;	//��������
						PlayerTechnologyDB::UpdateStudyInfoToDb(player, id, ref);		//��study���и���
						msg.set_result(pb::UpStarComplete);
						msg.set_id(id);
						msg.set_starnum(ref.starNum);
					}
					else
					{
						msg.set_result(pb::UpStarResourceEmpty);		//��������
					}
				}
				else		
				{
					msg.set_result(pb::UpStarResultElse);
				}
			}
			else
			{
				msg.set_result(pb::UpStarTriggerDecline);
			}
		}
		else
		{
			msg.set_result(pb::UpStarResultElse);
		}
	}

    return ;
}

//�о�
bool PlayerTechnology::Study(Player* player, uint32 id,pb::GS2C_Technology_Study& msg) 
{
    if (!Have(id))		//������о��Ѿ���ɣ��򷵻�
	{
		if (const TecTable* table = sTechnologyTableMgr.GetTable(id))	//����id���excel���е�һ��
		{
			TechnologyStudyStruct& ref = m_StudyInfo[id];
			if (LIKELY(ref.endTime == 0 && ref.starNum == table->NeedStarCnt()))	//�о�û�п�ʼ��������������������ʼ�о�
			{		
				ref.endTime = sOS.TimeSeconds() + table->NeedTimeSec();

				msg.set_id(id);
				msg.set_isstudystart(1);
				msg.set_end_time(ref.endTime);
#ifdef _MMO_SERVER_
				sAutoUpdateGame.AddEvent(boost::bind(&PlayerTechnology::Finish, this, player, id),table->NeedTimeSec());	//������о��ص�����
#endif
				PlayerTechnologyDB::UpdateStudyInfoToDb(*player, id, ref);			//�������ݱ�
				return true;
			}
			else if (LIKELY(ref.endTime != 0 && ref.starNum == table->NeedStarCnt()))	//�о��ѿ�ʼ����������������������������
			{
				uint64 nowTime = sOS.TimeSeconds();
				if(LIKELY(ref.endTime > nowTime))		//ʱ��С��0��ֱ������о�
				{
#ifdef _MMO_SERVER_
					uint64 restTime = ref.endTime - nowTime;
					sAutoUpdateGame.AddEvent(boost::bind(&PlayerTechnology::Finish, this, player, id),(uint32)restTime);	//������о��ص�����
#endif
				}
				else
				{
					Finish(player, id);
				}
				msg.set_id(id);
				msg.set_isstudystart(1);
				msg.set_end_time(ref.endTime);
				return true;
			}
			else 
			{
				msg.set_id(id);
				msg.set_isstudystart(1);
				msg.set_end_time(ref.endTime);
				return true;
			}
		}
		return false;
	}
	return false;
}

//����
void PlayerTechnology::SpeedUp(Player& player, uint32 id, pb::GS2C_Technology_SpeedUp& msg)
{
	if(LIKELY(!Have(id)))
	{
		const TecTable* table = sTechnologyTableMgr.GetTable(id);
		if(LIKELY(table))	//����id���excel���е�һ��
		{
			if (LIKELY(sTrigger.Check(&player, table->m_Trigger)))	//�����㴥������
			{
				TechnologyStudyStruct& ref = m_StudyInfo[id];
				if (LIKELY(ref.endTime != 0 && ref.starNum == table->NeedStarCnt()))	//�о��ѿ�ʼ����������������������������
				{
					uint64 nowTime = sOS.TimeSeconds();
					if(LIKELY(ref.endTime >= nowTime))		//ʱ��С��0��ֱ������о�
					{
						uint64 restTime = ref.endTime - nowTime;
						int costs = (((int)restTime/TECHNOLOGY_SPEED_UP)+1)*(GET_BASE_DEF_UINT(pb::BD_SCIENCE_ACCELERATE_COST_GOLD));
						if(LIKELY(sReward.Change(player, Reward::Coin_Gold,-costs)))			//�۳���������
						{
							Finish(&player, id);
							msg.set_result(pb::SpeedUpComplete);
						}
						else
							msg.set_result(pb::SpeedUpResourceEmpty);
					}
					else
					{
						Finish(&player, id);
						msg.set_result(pb::SpeedUpComplete);
					}
				}
				else
				{
					msg.set_result(pb::SpeedUpResultElse);
				}
			}
			else
			{
				msg.set_result(pb::SpeedUpTriggerDecline);
			}
		}
		else
		{
			msg.set_result(pb::SpeedUpResultElse);
		}
	}
	else
	{
		msg.set_result(pb::SpeedUpAlreadyDown);
	}
	msg.set_id(id);
	return ;
}

//����о�֮��ʵ�ֽ���
/////zhoulunhao
void PlayerTechnology::OnAward(Player& player,uint32 id)
{
	const TecTable *tec = NULL;
	uint32 ordernum = 0;
	uint32 exp = 0;
	switch(id)
	{
	case EXP_UP_1:		//ս����,�佫����+50%
		//tec = sTechnologyTableMgr.GetTable(id);
		//if(tec)
		//{
		//	player.SetHerosExpIpPercent(tec->Value1());
		//}
		break;
	case FIGHT_ENFORCE1: //��������Ϊ2��,ս��������,������������
	case ARMY_ENFORCE1:///�佫�ɴ�3��ʿ��
	case FIGHT_ENFORCE2:///���ӵĹ���,����,�������õ�����
	case FIGHT_ENFORCE4:///��������Ϊ3��,ս��������,������������
	case ARMY_ENFORCE3:///���ӿɴ�4��ʿ��
	case FIGHT_ENFORCE5:///ս��ǿ��V:��������Ϊ4����ս����������������������
		player.ChangeCharAttrByTecnology(id);
		break;
	case ARMY_ENFORCE2: ///�ɶ���ļһ������
	case ARMY_ENFORCE4: ///����ǿ��IV:���ٶ���ļһ������
		player.ZhaoMuOneMoreChar(id);
		break;
	case JUNGONGBOX1:///��߾����콱���ޣ�����2����������
	case JUNGONGBOX2:////��߾����콱���ޣ�����3����������
	case JUNGONGBOX3:///��ѫ����III:��߹�ѫ�������ޣ�����4����ѫ����
	case JUNGONGBOX4:///��߾����콱���ޣ�����5����������
	case JUNGONGBOX5://������������6
	case JUNGONGBOX6:///��߹�ѫ�콱���ޣ�����7����ѫ����
	case GONGGAOLIZHONG:///��������I:�߼���ѫ�����Ѷ�����������Ʒ�ʽ���
	case GONGGAOLIZHONG2://��������II:��߹�ѫ���ޣ���߹�ѫ����
		tec = sTechnologyTableMgr.GetTable(id);
		if(tec)
		{
			player.SetValueInt(pb::PLAYER_FIELD_EXPLOIT_RANK_BOX_LEVEL,tec->Value1());
			player.GiveExploitBox();
		}
		break;
	case SHIQI_UP: ///	���㽱����������
 		break;
	case QIANGZHENG:////����20%�����ٶ�
		player.m_buildingLogMgr->UpdateModuleGainValue(&player,MODULE_BINGYING,player.GetBuildingIncome(MODULE_BINGYING));
		break;
	case GONGCHENGGONGXUN2:///���ǹ�ѫII:�и��ʴ����������������ô�����ѫ
		break;
	case YUAN_ZHENG:		//Զ��:����Զ������ɿɻ�ô�������
		break;
	case YUAN_ZHENG2:///Զ��:Զ�����������Զ����������ɿɻ�ö���50%����
		break;
	case GAOJIYUANZHENG:		//�߼�Զ��:���Զ����������þ������50%
	case DIYUJUBEN:///�����籾:��������Զ������ɿɻ�ô�������
		break;

	case LIANBINZENGYI:///��������:��������������������Զ�ת��Ϊ����
		ordernum = player.GetAttrUint(pb::PLAYER_FIELD_TROOPSODER);
		exp =  ordernum * GET_BASE_DEF_UINT(pb::BD_NORMAL_TRAINING_ORDER_EXCHANGE_EXP);
		player.AddValue(pb::PLAYER_FIELD_CUR_XP,exp);
		break;
	case ZHANSHENGJUBEN:///����Զ��:���Զ����������ú������飬�������50%
		break;
	case BAOSHIHECHENG2:///��ʯ�ϳ�II:����10����ʯ����ʯ����
		break;
	case JISUZENGBING:///��������:��������������30��
		tec = sTechnologyTableMgr.GetTable(id);
		if(tec)
		{
			player.AddValue(pb::PLAYER_FIELD_DRAFT_ORDER_LIMIT,tec->Value1());//����������
		}
		break;
	case ZHANGETONGKUAI:///ս��ʹ��:�Զ���ս��ʼ����ȡ��
		break;
	case ZHUCHENGGAIZAO:///���Ǹ���:�������Ǹ��죬��������Դ�����Ͳ���ʵ��
		break;
	case ZENGBINSHANGXIAN:///��������:������������������40��
		tec = sTechnologyTableMgr.GetTable(id);
		if(tec)
		{
			player.AddValue(pb::PLAYER_FLIELD_TROOPS_ORDER_LIMIT,tec->Value1());//use this
		}
		break;
	case ZHENGDIANJIANGLI:///���㽱��:���㽱����������50%
		break;
	case BOXSHANGXIAN://��������:������������50��
		tec = sTechnologyTableMgr.GetTable(id);
		if(tec)
		{
			player.AddValue(pb::PLAYER_FIELD_NORMAL_EXPLOIT_BOX_LIMIT,tec->Value1());
		}
		break;
	case ZHAOMUFUJIANG:///��ļ����:�ɶ���ļһ�����ӣ���Ϊ�����ĵ����ű�����
		break;
	case BUDUIJUEXING://���Ӿ���:120�����ϲ��ӿ��Ծ���
		if(player.GetLevel() > 120)
		{
			//�佫���Ծ���
		}
		break;
	case SHENGSHIJINGHUA://��ʯ����:����21����ʯ����Ϊ��ʯ
		break;
	case ZHANGONGHEHE:///ս���պ�:��ѫ��8000�󽫿����߼���ѫ����
		player.SetValueInt(pb::PLAYER_FIELD_IS_ADVANCE_BOX,1);
		player.GiveExploitBox();
		break;
	case SHENGSHIJINGHUA2:///��ʯ����II:����30����ʯ��10������ʯ��
		break;
	case SHENGSHIJINGHUA3:///��ʯ����III:����20������ʯ
		break;
	default:
		break;
	}
}

void PlayerTechnology::GM_Finish( Player* player,uint32 id )
{
	if(!Have(id))//��ֹ�����ɿƼ�
	{
		TechnologyStudyStruct& ref = m_StudyInfo[id];
		const TecTable* table = sTechnologyTableMgr.GetTable(id);
		ref.endTime = sOS.TimeSeconds() + table->NeedTimeSec();
		ref.is_finished = 1;
		ref.starNum = table->NeedStarCnt();
		PlayerTechnologyDB::UpdateStudyInfoToDb(*player,id,ref);
		OnAward(*player,id);
		pb::GS2C_Technology_Finish msg;
		SaveTo(msg);
		msg.set_id(id);
		player->Send(pb::SMSG_TECHNOLOGY_FINISH_RSP, msg);
		pb::GS2C_Technology_All_Info all_info_msg;
		SaveTo(*player, all_info_msg);
		player->Send(pb::SMSG_TECHNOLOGY_ALL_INFO, all_info_msg);
	}
}

uint32  PlayerTechnology::HasFinishUpStar( uint32 id )
{
	if(m_StudyInfo[id].starNum == sTechnologyTableMgr.GetTable(id)->NeedStarCnt())
		return id;
	return 0;
}

uint32 PlayerTechnology::HaveTec(uint32 tec_id )
{
	if(Have(tec_id))
		return tec_id;
	return 0;
}

uint32 PlayerTechnology::StudyTec(Player* player, uint32 tec_id )
{
	pb::GS2C_Technology_Study msg;
	if(Study(player,tec_id,msg))
		return tec_id;
	return 0;
}