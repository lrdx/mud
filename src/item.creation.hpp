/*************************************************************************
*   File: item.creation.hpp                            Part of Bylins    *
*   Item creation from magic ingidients functions header                 *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                      *
**************************************************************************/

#ifndef __ITEM_CREATION_HPP__
#define __ITEM_CREATION_HPP__

#include "skills.h"
#include "interpreter.h"
#include "features.hpp"

#include <string>
#include <list>

#define MAX_ITEMS 	9

#define MAX_PARTS	3

#define MAX_PROTO 	3
#define COAL_PROTO 	311
#define WOOD_PROTO      313
#define TETIVA_PROTO    314


#define MREDIT_MAIN_MENU	0
#define MREDIT_OBJ_PROTO 	1
#define MREDIT_SKILL		2
#define MREDIT_LOCK	 	3
#define MREDIT_INGR_MENU	4
#define MREDIT_INGR_PROTO	5
#define MREDIT_INGR_WEIGHT	6
#define MREDIT_INGR_POWER	7
#define MREDIT_DEL 	 	8
#define MREDIT_CONFIRM_SAVE     9

#define MAKE_ANY 	0
#define MAKE_POTION	1
#define MAKE_WEAR	2
#define MAKE_METALL	3
#define MAKE_CRAFT	4
#define MAKE_AMULET 5

// ���������� ����������� ���������� ����� ��� ����������� ���-�� �������.
#define MIN_MAKE_MOVE   10

using std::string;
using std::ifstream;
using std::ofstream;
using std::list;
using std::endl;

void mredit_parse(struct DESCRIPTOR_DATA *d, char *arg);
void mredit_disp_menu(struct DESCRIPTOR_DATA *d);
void mredit_disp_ingr_menu(struct DESCRIPTOR_DATA *d);

void do_list_make(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_edit_make(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_make_item(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

void init_make_items();
// ������ ��������� �� �� ���������� � ���������.
struct create_item_type
{
	int obj_vnum;
	int material_bits;
	int min_weight;
	int max_weight;
	std::array<int, MAX_PROTO> proto;
	ESkill skill;
	std::underlying_type<EWearFlag>::type wear;
};
// ����� ��������� �� �� ���������� ��� �������� ����� �� ������������
struct ingr_part_type
{
	int proto;
	int min_weight;
	int min_power;
};
struct make_skill_type
{
	const char *name;
	const char *short_name;
	ESkill num;
};

class MakeRecept;

class MakeReceptList
{
	list < MakeRecept * >recepts;

public:
	// �������� ���������� �������� ��������.
	MakeReceptList();

	~MakeReceptList();

	// ����� ������ �������� �� ���� ����������� � ���������
	int can_make_list(CHAR_DATA * ch);

	// ��������� ������� .
	int load();

	// ��������� �������.
	int save();

	// ������� ������ �� �������� ��� ��������� �� �����.
	MakeRecept *get_by_name(string & rname);

	MakeReceptList *can_make(CHAR_DATA * ch, MakeReceptList * canlist, int use_skill);

	// ����� ��������� ��������
	size_t size();

	MakeRecept *operator[](size_t i);

	// ������������ ������ ��������.
	void clear();

	void add(MakeRecept * recept);

	void add(MakeReceptList * recept);

	void del(MakeRecept * recept);

	void sort();
};

class MakeRecept
{
	int stat_modify(CHAR_DATA * ch, int value, float devider);

	int add_flags(CHAR_DATA * ch, FLAG_DATA * base_flag, const FLAG_DATA* add_flag, int delta);

	int add_affects(CHAR_DATA * ch, std::array<obj_affected_type, MAX_OBJ_AFFECT>& base, const std::array<obj_affected_type, MAX_OBJ_AFFECT>& add, int delta);

	int get_ingr_lev(OBJ_DATA *ingrobj);

	void make_object(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *ingrs[MAX_PARTS], int ingr_cnt);

	void make_value_wear(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *ingrs[MAX_PARTS]);
        //� ��������� � ��� �� ��������. ��������� ����������� ��������
        float count_mort_requred(OBJ_DATA * obj);
        
        float count_affect_weight(int num, int mod);      

	int get_ingr_pow(OBJ_DATA *ingrobj);

	void add_rnd_skills(CHAR_DATA * ch, OBJ_DATA * obj_from, OBJ_DATA *obj_to);
        
public:
	bool locked;

	ESkill skill;
	int obj_proto;
	std::array<ingr_part_type, MAX_PARTS> parts;

	// ����������� ���������� �������� �� �������.
	// ������������ ������� ��������� �����.
	MakeRecept();
	// ���������� ����� �� � �������� �� ����������� ����������� � ���������
	int can_make(CHAR_DATA *ch);
	// ������� ������� �� �������
	int make(CHAR_DATA *ch);
	// �������� ������ �� ������.
	int load_from_str(string & rstr);
	// ��������� ������ � ������.
	int save_to_str(string & rstr);
};

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
