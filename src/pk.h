/* ************************************************************************
*   File: pk.h                                          Part of Bylins    *
*  Usage: header file: constants and fn prototypes for �� �������         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _PK_H_
#define _PK_H_

#include "char.hpp"

#include <string>

class OBJ_DATA;	// forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.

//*************************************************************************
// �������� ������ � ���������� ����������� �������� ����� ��������

// ������������� �������:
//   1. ����� ������� may_kill_here()
//      ����� �� ����� ��������� ������ (����������� �� �����, ����� � �.�.)
//   2. ����� ������� check_pkill()
//      ���������� ������������� ����������� �������� � ����������
//      ������� ��� ������ ���������
//   3. ����� ������� pk_agro_action() ��� ������ ��������
//   4. ����� ������� pk_thiefs_action() ��� ���������
//   5. ����� ������� pk_revenge_action() � ������ ��������

// ��������� ��� ���������� �� ���������
struct PK_Memory_type
{
	long unique;		// unique ������
	long kill_num;		// ���������� ������ �������� ��������� �� unique
	long kill_at;		// ����� ��������� ���������� ����� (��� spamm)
	long revenge_num;	// ���������� ������� ���������� ����� �� ������� unique
	long battle_exp;	// ����� ��������� ��������
	long thief_exp;		// ����� ��������� ����� ���������
	long clan_exp;		// ����� ��������� ����-�����
	struct PK_Memory_type *next;
};

#define		PK_ACTION_NO		1	// ������� ����������
#define		PK_ACTION_FIGHT     2	// �������� � �������� ��������
#define		PK_ACTION_REVENGE	4	// ������� ����������� �����
#define		PK_ACTION_KILL		8	// ���������

// agressor ��������� ������ victim
// ��������� - ��� �������� (��. ����)
int pk_action_type(CHAR_DATA * agressor, CHAR_DATA * victim);

// �������� ����� �� ch ������ ����������� �������� ������ victim
// TRUE - �����
// FALSE - �� �����
int may_kill_here(CHAR_DATA * ch, CHAR_DATA * victim);

// �������� �� ������ ��� �����
bool check_agrobd(CHAR_DATA *ch);

// ����������� ������������� ������� ��� ������ ���������
bool need_full_alias(CHAR_DATA * ch, CHAR_DATA * opponent);

//�����������, �������� �� ������ arg ������ ������ ch
int name_cmp(CHAR_DATA * ch, const char *arg);

// ����������� ����������� ������������
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const char *arg);
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const std::string &arg);

// agressor �������� �������� ������ victim
bool pk_agro_action(CHAR_DATA * agressor, CHAR_DATA * victim);

// thief �������� �������� ������ victim, ���������� � �������� �����
void pk_thiefs_action(CHAR_DATA * thief, CHAR_DATA * victim);

// killer ������� victim
void pk_revenge_action(CHAR_DATA * killer, CHAR_DATA * victim);

// ������� ������ ��
void pk_free_list(CHAR_DATA * ch);

// ��������� ���������� ������ �����
int pk_count(CHAR_DATA * ch);

//���������� ������ �� ������� (���������� ����)
int pk_player_count(CHAR_DATA * ch);

//*************************************************************************
// �������������� ������� ����������� ������� ��

void aura(CHAR_DATA * ch, int lvl, CHAR_DATA * victim, char *s);
const char *CCPK(CHAR_DATA * ch, int lvl, CHAR_DATA * victim);
inline const char *CCPK(CHAR_DATA* ch, int lvl, const CHAR_DATA::shared_ptr& victim) { return CCPK(ch, lvl, victim.get()); }
void pk_list_sprintf(CHAR_DATA * ch, char *buff);

//*************************************************************************
// ��������� ������� ����������/�������� �� ������
void save_pkills(CHAR_DATA * ch, FILE * saved);

//*************************************************************************
bool has_clan_members_in_group(CHAR_DATA * ch);
//Polud
void pkPortal(CHAR_DATA* ch);

//�������� ����
namespace bloody
{
	//��������� ���� ���������� �� �����
	void update(); 
	//������� ����
	void remove_obj(const OBJ_DATA* obj);

	//��������� �������� �������� obj �� ch � victim
	//ch ����� ���� NULL (���)
	//victim ����� ���� null (� ������ � �������, ���, �������..)
	//���������� true, ���� �������� ����� ����������, � false � ��������� ������
	bool handle_transfer(CHAR_DATA* ch, CHAR_DATA* victim, OBJ_DATA* obj, OBJ_DATA* container=NULL);
	//�������� ���� � ����� ��� ��������
	void handle_corpse(OBJ_DATA* corpse, CHAR_DATA* ch, CHAR_DATA* killer);
	bool is_bloody(const OBJ_DATA* obj);
}

//��������� ��� �������� ���������� � �������� �����
//����� �� ��������� ����� ����� � OBJ_DATA, ������ ������ ���������� ������������ ITEM_BLOODY � ����������� ������ � bloody_map
struct BloodyInfo
{
	long owner_unique; //� ���� ����� ������
	long kill_at; //����� ��������� ��������
	OBJ_DATA * object; //���� ������
	//�����-���� ID ������ ��������� ������� �� ����������, ������ ��� ����� ��� ������� ������� ���������� ��� ������ ������ �������� ������ ������ �� id �� ������ ������
	//� ��� O(n)
	//��-����� � ����������� OBJ_DATA ����� ������� ������ � ������ �� bloody_map. ����� ��� ������� � ���������� shared_ptr'�� � ��� � ����
	BloodyInfo(const long _owner_unique=0, const long _kill_at=0, OBJ_DATA* _object=0):
		owner_unique(_owner_unique), kill_at(_kill_at), object(_object) { }
};

typedef std::map<const OBJ_DATA*, BloodyInfo> BloodyInfoMap;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
