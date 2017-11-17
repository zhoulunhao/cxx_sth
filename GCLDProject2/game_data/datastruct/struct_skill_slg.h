#pragma once
#include "struct_base.h"

#pragma pack(push,1)

struct DB_SkillSlg
{
	INIT_GAMEDATA(DB_SkillSlg)
	static const bool HasIndex(){ return true; }
	const uint32 GetKey() const{ return id; }
	static const char* GetKeyName(){ return "id"; }
	static const char* GetTableName(){ return "skill_slg_proto"; }
	static const char* GetFormat() {
		return "usuusufusssuuuuuu";
	}

	uint32 id;				//����id
	string comment;
	uint32 name_id;
	uint32 description_id;
	string skill_proto_name;
	uint32 attack_range;	//������Χ
	float  damage_factor;	//�˺�ϵ��
	uint32 addtion_buff_id;	//���ܸ���buffID;

	//TODO�����׶β���
	string trigger_condition_list;	//���������б�
	string trigger_skill_id_list;	//���������б�
	string trigger_probability_list;//���������б�

	uint32 effect_id;
	uint32 release_times;		//���ʩ�Ŵ���
	uint32 effect_mount;
	uint32 prefeb_effect_id;
	uint32 damage_special_id;
	uint32 area_or_single;
};

struct DB_Buff_Fight
{
	INIT_GAMEDATA(DB_Buff_Fight)
	static const bool HasIndex(){ return true; }
	const uint32 GetKey() const{ return id; }
	static const char* GetKeyName(){ return "id"; }
	static const char* GetTableName(){ return "buff_fight"; }
	static const char* GetFormat() {
		return "usuusuufffuuuuuuuus";
	}

	uint32          id;
	string          comment;
	uint32          name_id;
	uint32          description_id;
	string          image_name;
	uint32          buff_level;
	uint32          buff_type;
	float           value1;
	float           value2;
	float           value3;
	uint32          last_time;
	uint32          target;
	uint32          is_save;
	uint32			effect_id;
	uint32			rows;
	uint32			who_first;
	uint32			when_take_effect;
	uint32			execute_id;
	string			show_effect;
};

struct DB_Buff_Relation
{
	INIT_GAMEDATA(DB_Buff_Relation)
	static const bool HasIndex(){ return true; }
	const uint32 GetKey() const{ return id; }
	static const char* GetKeyName(){ return "id"; }
	static const char* GetTableName(){ return "buff_relation"; }
	static const char* GetFormat() {
		return "uus";
	}

	uint32          id;
	uint32          buff_type;
	string			buff_relation;
};

#pragma pack(pop)