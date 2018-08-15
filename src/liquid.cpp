/*************************************************************************
*   File: liquid.cpp                                   Part of Bylins    *
*   ��� �� ���������                                                     *
*                                                                        *
*  $Author$                                                      *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "liquid.hpp"

#include "obj.hpp"
#include "char.hpp"
#include "char_obj_utils.inl"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "skills.h"
#include "room.hpp"
#include "screen.h"
#include "features.hpp"
#include "interpreter.h"

#include <cmath>

// ���� ����������, ���������� � ����������
const int LIQ_WATER = 0;
const int LIQ_BEER = 1;
const int LIQ_WINE = 2;
const int LIQ_ALE = 3;
const int LIQ_QUAS = 4;
const int LIQ_BRANDY = 5;
const int LIQ_MORSE = 6;
const int LIQ_VODKA = 7;
const int LIQ_BRAGA = 8;
const int LIQ_MED = 9;
const int LIQ_MILK = 10;
const int LIQ_TEA = 11;
const int LIQ_COFFE = 12;
const int LIQ_BLOOD = 13;
const int LIQ_SALTWATER = 14;
const int LIQ_CLEARWATER = 15;
const int LIQ_POTION = 16;
const int LIQ_POTION_RED = 17;
const int LIQ_POTION_BLUE = 18;
const int LIQ_POTION_WHITE = 19;
const int LIQ_POTION_GOLD = 20;
const int LIQ_POTION_BLACK = 21;
const int LIQ_POTION_GREY = 22;
const int LIQ_POTION_FUCHSIA = 23;
const int LIQ_POTION_PINK = 24;
const int LIQ_POISON_ACONITUM = 25;
const int LIQ_POISON_SCOPOLIA = 26;
const int LIQ_POISON_BELENA = 27;
const int LIQ_POISON_DATURA = 28;
// ����������
const int NUM_LIQ_TYPES = 29;

// LIQ_x
const char *drinks[] = { "����",
						 "����",
						 "����",
						 "��������",
						 "�����",
						 "��������",
						 "�����",
						 "�����",
						 "�����",
						 "����",
						 "������",
						 "���",
						 "�����",
						 "�����",
						 "������� ����",
						 "���������� ����",
						 "����������� �����",
						 "�������� ����������� �����",
						 "������ ����������� �����",
						 "������ ����������� �����",
						 "����������� ����������� �����",
						 "������� ����������� �����",
						 "������ ����������� �����",
						 "����������� ����������� �����",
						 "�������� ����������� �����",
						 "�������� �������",
						 "�������� ��������",
						 "�������� ������",
						 "�������� �������",
						 "\n"
					   };

// one-word alias for each drink
const char *drinknames[] = { "�����",
							 "�����",
							 "�����",
							 "���������",
							 "������",
							 "���������",
							 "������",
							 "������",
							 "������",
							 "�����",
							 "�������",
							 "����",
							 "����",
							 "������",
							 "������� �����",
							 "���������� �����",
							 "���������� ������",
							 "������� ���������� ������",
							 "����� ���������� ������",
							 "����� ���������� ������",
							 "���������� ���������� ������",
							 "������ ���������� ������",
							 "����� ���������� ������",
							 "���������� ���������� ������",
							 "������� ���������� ������",
							 "��������� �������",
							 "��������� ��������",
							 "��������� ������",
							 "��������� �������",
							 "\n"
						   };

// effect of drinks on DRUNK, FULL, THIRST -- see values.doc
const int drink_aff[][3] = { 
	{0, 1, -10},	        // ����
	{2, -2, -3},			// ����
	{5, -2, -2},			// ����
	{3, -2, -3},			// ��������
	{1, -2, -5},			// ����
	{8,  0, 4},				// ������� (��� � ����� �����, ����� �� ������ �� ����� ����� ���� ���� �������, ������ �������� �������)
	{0, -1, -8},			// ����
	{10, 0, 3},			    // ����� (����� �� �����! ����� ��� �������������, ���� ����� ������� ������, �� ������ �����)
	{3, -3, -3},			// �����
	{0, -2, -8},			// ��� (��� ���� � ����������)
	{0, -3, -6},			// ������
	{0, -1, -6},			// ���
	{0, -1, -6},			// ����
	{0, -2, 1},			// �����
	{0, -1, 2},			// ������� ����
	{0, 0, -13},			// ���������� ����
	{0, -1, 1},			// ���������� �����
	{0, -1, 1},			// ������� ���������� �����
	{0, -1, 1},			// ����� ���������� �����
	{0, -1, 1},			// ����� ���������� �����
	{0, -1, 1},			// ���������� ���������� �����
	{0, -1, 1},			// ������ ���������� �����
	{0, -1, 1},			// ����� ���������� �����
	{0, -1, 1},			// ���������� ���������� �����
	{0, -1, 1},			// ������� ���������� �����
	{0, 0, 0},			// �������� �������
	{0, 0, 0},			// �������� ��������
	{0, 0, 0},			// �������� ������
	{0, 0, 0}			// �������� �������
};

// color of the various drinks
const char *color_liquid[] = { "����������",
							   "����������",
							   "��������",
							   "����������",
							   "�����",
							   "����������",
							   "���������",
							   "�������",
							   "������",
							   "������ �������",
							   "�����",
							   "����������",
							   "�����-����������",
							   "�������",
							   "����������",
							   "���������� ������",
							   "����������",
							   "��������",
							   "��������",
							   "�����������",
							   "����������",
							   "������ ������",
							   "����������",
							   "����������",
							   "�������",
							   "��������",
							   "��������",
							   "��������",
							   "��������",
							   "\n"
							 };

/**
* �����, ��������� � ����������, ����� ���� �� ����� ���.
* �� ������, ����� �������� ��������� ��� �������, �������
* ��� ����� ���� �� ������ �� �������.
*/
bool is_potion(const OBJ_DATA *obj)
{
	switch(GET_OBJ_VAL(obj, 2))
	{
	case LIQ_POTION:
	case LIQ_POTION_RED:
	case LIQ_POTION_BLUE:
	case LIQ_POTION_WHITE:
	case LIQ_POTION_GOLD:
	case LIQ_POTION_BLACK:
	case LIQ_POTION_GREY:
	case LIQ_POTION_FUCHSIA:
	case LIQ_POTION_PINK:
		return true;
		break;
	}
	return false;
}

namespace drinkcon
{

ObjVal::EValueKey init_spell_num(int num)
{
	return num == 1
		? ObjVal::EValueKey::POTION_SPELL1_NUM
		: (num == 2
			? ObjVal::EValueKey::POTION_SPELL2_NUM
			: ObjVal::EValueKey::POTION_SPELL3_NUM);
}

ObjVal::EValueKey init_spell_lvl(int num)
{
	return num == 1
		? ObjVal::EValueKey::POTION_SPELL1_LVL
		: (num == 2
			? ObjVal::EValueKey::POTION_SPELL2_LVL
			: ObjVal::EValueKey::POTION_SPELL3_LVL);
}

void reset_potion_values(CObjectPrototype *obj)
{
	obj->set_value(ObjVal::EValueKey::POTION_SPELL1_NUM, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL1_LVL, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL2_NUM, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL2_LVL, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL3_NUM, -1);
	obj->set_value(ObjVal::EValueKey::POTION_SPELL3_LVL, -1);
	obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, -1);
}

/// ������� � ������ (GET_OBJ_VAL(from_obj, 0)) ���� ���� �� ��� �����
bool copy_value(const CObjectPrototype *from_obj, CObjectPrototype *to_obj, int num)
{
	if (GET_OBJ_VAL(from_obj, num) > 0)
	{
		to_obj->set_value(init_spell_num(num), GET_OBJ_VAL(from_obj, num));
		to_obj->set_value(init_spell_lvl(num), GET_OBJ_VAL(from_obj, 0));
		return true;
	}
	return false;
}

/// ���������� values ������� (to_obj) �� ����� (from_obj)
void copy_potion_values(const CObjectPrototype *from_obj, CObjectPrototype *to_obj)
{
	reset_potion_values(to_obj);
	bool copied = false;

	for (int i = 1; i <= 3; ++i)
	{
		if (copy_value(from_obj, to_obj, i))
		{
			copied = true;
		}
	}

	if (copied)
	{
		to_obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, GET_OBJ_VNUM(from_obj));
	}
}

} // namespace drinkcon

using namespace drinkcon;

int cast_potion_spell(CHAR_DATA *ch, OBJ_DATA *obj, int num)
{
	const int spell = obj->get_value(init_spell_num(num));
	const int level = obj->get_value(init_spell_lvl(num));

	if (spell >= 0 && level >= 0)
	{
		return call_magic(ch, ch, NULL, world[ch->in_room],
			spell, level, CAST_POTION);
	}
	return 1;
}

// ����� ���
void do_drink_poison (CHAR_DATA *ch, OBJ_DATA *jar,int amount) {
	if ((GET_OBJ_VAL(jar, 3) == 1) && !IS_GOD(ch))
	{
		send_to_char("���-�� ���� �����-�� ��������!\r\n", ch);
		act("$n ���������$u � ��������$g.", TRUE, ch, 0, 0, TO_ROOM);
		AFFECT_DATA<EApplyLocation> af;
		af.type = SPELL_POISON;
		//���� ����� 0 - 
		af.duration = pc_duration(ch, amount == 0 ? 3 : amount==1 ? amount : amount * 3, 0, 0, 0, 0);
		af.modifier = -2;
		af.location = APPLY_STR;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = AF_SAME_TIME;
		affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
		af.type = SPELL_POISON;
		af.modifier = amount == 0? GET_LEVEL(ch) * 3 : amount * 3;
		af.location = APPLY_POISON;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = AF_SAME_TIME;
		affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
		ch->Poisoner = 0;
	}
}

int cast_potion(CHAR_DATA *ch, OBJ_DATA *jar)
{
	// Added by Adept - ������ ���� � ������� ��� ������� �����	
	if (is_potion(jar) && jar->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) >= 0)
	{
		act("$n �����$g ����� �� $o1.", TRUE, ch, jar, 0, TO_ROOM);
		send_to_char(ch, "�� ������ ����� �� %s.\r\n", OBJN(jar, ch, 1));
		
		//�� ����� �������, �� ��� ����
		for (int i = 1; i <= 3; ++i)
			if (cast_potion_spell(ch, jar, i) <= 0)
				break;
		
		WAIT_STATE(ch, PULSE_VIOLENCE);
		jar->dec_weight();
		// ��� ������
		jar->dec_val(1);

		if (GET_OBJ_VAL(jar, 1) <= 0
			&& GET_OBJ_TYPE(jar) != OBJ_DATA::ITEM_FOUNTAIN)
		{
			name_from_drinkcon(jar);
			jar->set_skill(SKILL_INVALID);
			reset_potion_values(jar);
		}
		do_drink_poison(ch,jar,0);
		return 1;
	}
	return 0;
}

int do_drink_check(CHAR_DATA *ch, OBJ_DATA *jar) 
{
	//�������� � ���?
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND))
	{
		send_to_char("�� ����� ����������� � ���!\r\n", ch);
		return 0;
	}

	//��������� �� ������ ������� ���������� �����
	if (GET_OBJ_TYPE(jar) == OBJ_DATA::ITEM_MING)
	{
		send_to_char("�� ������ ����������� - ������� �������!\r\n", ch);
		return 0;
	}
	//��������� ����� �� �� ����� ����

	if (GET_OBJ_TYPE(jar) != OBJ_DATA::ITEM_DRINKCON
		&& GET_OBJ_TYPE(jar) != OBJ_DATA::ITEM_FOUNTAIN)
	{
		send_to_char("�� �����. ������ � ��� �����!\r\n", ch);
		return 0;
	}

	// ���� �������� - ���� ������
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char("�� ��� ������ � ������ ������� �� ����������!\r\n", ch);
		return 0;
	}

	// ������ ���������
	if (!GET_OBJ_VAL(jar, 1))
	{
		send_to_char("�����.\r\n", ch);
		return 0;
	}

	return 1;
}

//��������� ���������� ��� �����
OBJ_DATA* do_drink_get_jar (CHAR_DATA *ch, char *jar_name)
{
	OBJ_DATA* jar = NULL;
	if (!(jar = get_obj_in_list_vis(ch, jar_name, ch->carrying)))
	{
		if (!(jar = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents)))
		{
			send_to_char("�� �� ������ ��� �����!\r\n", ch);
			return jar;
		}

		if (GET_OBJ_TYPE(jar) == OBJ_DATA::ITEM_DRINKCON)
		{
			send_to_char("������ ��� ����� �������.\r\n", ch);
			return jar;
		}
	}
	return jar;
}

int do_drink_get_amount(CHAR_DATA *ch, OBJ_DATA *jar, int subcmd) {
	
	int amount = 1; //�� ��������� 1 ������
	float V = 1;

	// ���� �������� � ��������
	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] > 0)
	{
		if (GET_COND(ch,DRUNK)>= CHAR_MORTALLY_DRUNKED){
				amount = -1; //���� �����
		} else {
			//��� ����� ��-�� /4
			amount = ( 2 * CHAR_MORTALLY_DRUNKED - GET_COND(ch,DRUNK) ) / drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK];
			amount = MAX(1,amount); // �� ��� ����-����
		}
	}
	// ���� ��� �������
	else
	{
		// ������ 3-10 �������
		amount = number(3, 10);
	}
	
	// ���� �������� ������� ����
	if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST]<0)
	{
		V = (float) - GET_COND(ch,THIRST)/drink_aff[GET_OBJ_VAL(jar, 2)][THIRST];
	} 
	// ���� �������� �������� ������
	else if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST]>0)
	{
		V = (float) (MAX_COND_VALUE-GET_COND(ch,THIRST))/drink_aff[GET_OBJ_VAL(jar, 2)][THIRST];
	} else {
		V = 999.0;
	}
	amount = MIN(amount, round(V+0.49999));

	if (subcmd != SCMD_DRINK) // ���� �����, �� �������, �� �������
	{
		amount = MIN(amount,1);
	}

	//�������� �� ������ ����������
	amount = MIN(amount, GET_OBJ_VAL(jar, 1));
	return amount;
}

int do_drink_check_conditions(CHAR_DATA *ch, OBJ_DATA *jar, int amount)
{
	//������, ���� � ���� ����� - � ������� ����� (����� � �������!!!), ������ ������� ������ �� �������
	if (
		drink_aff[GET_OBJ_VAL(jar, 2)][THIRST]>0 &&
	 	GET_COND_M(ch,THIRST) > 5 // ����� ������ ��� ������� ������� ��� ������ :)
	) {
		send_to_char("� ��� ��������� � �����, ����� ���-�� ������.\r\n",ch);
		return 0;
	}

	
	// ���� �������� � ��������
	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK]>0)
	{
		// ���� � ���� ����� - ����� �����������, ������ ������
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_ABSTINENT)) {
			if (GET_SKILL(ch,SKILL_DRUNKOFF)>0) {//���� ������� ����
				send_to_char("��� ����������� �� ����� ����� � ��� ��� �� ������.\r\n������, ��� ����� ������������.\r\n", ch);
			} else {//���� �������� ���
				send_to_char("�� ��������... �� �� ������ ��������� ���� ������...\r\n", ch);				
			}
			return 0;
		}
		// ���� ��� ��� ������
		if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED) {
			// ���� ������� �� �������� ��� ��� ����� ��������
			if (GET_DRUNK_STATE(ch) == CHAR_MORTALLY_DRUNKED || GET_COND(ch, DRUNK) < GET_DRUNK_STATE(ch))
			{
				send_to_char("�� ������� ��� ����������, ������ ��� �������...\r\n", ch);
				return 0;
			}
		}
	}

	// �� ����� ����, �� ��� ������ �� ����� ������
	if ( amount <= 0 && !IS_GOD(ch) ) {
		send_to_char("� ��� ������ �� �����.\r\n", ch);
		return 0;
	}

	// ������ ���� � ���
	if (ch->get_fighting())
	{
		send_to_char("�� ����� ����������� � ���.\r\n", ch);
		return 0;
	}
	return 1;
}

void do_drink_drunk(CHAR_DATA *ch, OBJ_DATA *jar, int amount){
	int duration;

	if (drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK]<=0)
		return;

	if (amount == 0)
		return;

	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
	{
		if (GET_COND(ch, DRUNK) >= CHAR_MORTALLY_DRUNKED)
		{
			send_to_char("�������� �� �����, �� ����� ��� �� ����....\r\n", ch);
		}
		else
		{
			send_to_char("�������� ����� ��������� �� ������ ����.\r\n", ch);
		}
		
		duration = 2 + MAX(0, GET_COND(ch, DRUNK) - CHAR_DRUNKED);
		
		if (can_use_feat(ch, DRUNKARD_FEAT))
			duration += duration/2;
		
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_ABSTINENT)
				&& GET_DRUNK_STATE(ch) < MAX_COND_VALUE
					&& GET_DRUNK_STATE(ch) == GET_COND(ch, DRUNK))
		{
			send_to_char("������ ���� ������� ��� � ������.\r\n", ch);
			// **** Decrease AC ***** //
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_DRUNKED;
			af.duration = pc_duration(ch, duration, 0, 0, 0, 0);
			af.modifier = -20;
			af.location = APPLY_AC;
			af.bitvector = to_underlying(EAffectFlag::AFF_DRUNKED);
			af.battleflag = 0;
			affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
			// **** Decrease HR ***** //
			af.type = SPELL_DRUNKED;
			af.duration = pc_duration(ch, duration, 0, 0, 0, 0);
			af.modifier = -2;
			af.location = APPLY_HITROLL;
			af.bitvector = to_underlying(EAffectFlag::AFF_DRUNKED);
			af.battleflag = 0;
			affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
			// **** Increase DR ***** //
			af.type = SPELL_DRUNKED;
			af.duration = pc_duration(ch, duration, 0, 0, 0, 0);
			af.modifier = (GET_LEVEL(ch) + 4) / 5;
			af.location = APPLY_DAMROLL;
			af.bitvector = to_underlying(EAffectFlag::AFF_DRUNKED);
			af.battleflag = 0;
			affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);			
		}
	}
}

void do_drink(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	OBJ_DATA *jar;
	int amount;
	
	//���� �� ����
	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("���� �� ����?\r\n", ch);
		return;
	}

	// �������� ��������� � �����
	if (!(jar = do_drink_get_jar(ch,arg)))
		return;

	// ����� ��������
	if (!do_drink_check(ch,jar))
		return;
	
	// ������ �����
	if (cast_potion(ch, jar))
		return;
	
	// ��������� ����� �������� ��������� ��� �����������	
	amount = do_drink_get_amount(ch,jar,subcmd);
	
	// ��������� ����� �� ��� ����
	if (!do_drink_check_conditions(ch,jar,amount))
		return;
	
	if (subcmd == SCMD_DRINK)
	{
		sprintf(buf, "$n �����$g %s �� $o1.", drinks[GET_OBJ_VAL(jar, 2)]);
		act(buf, TRUE, ch, jar, 0, TO_ROOM);
		sprintf(buf, "�� ������ %s �� %s.\r\n", drinks[GET_OBJ_VAL(jar, 2)], OBJN(jar, ch, 1));
		send_to_char(buf, ch);
	}
	else
	{
		act("$n ���������$g �� $o1.", TRUE, ch, jar, 0, TO_ROOM);
		sprintf(buf, "�� ������ ���� %s.\r\n", drinks[GET_OBJ_VAL(jar, 2)]);
		send_to_char(buf, ch);
	}

	// ��� ���������� ��� �� ����� ���� ������ ���� ����������

	if (GET_OBJ_TYPE(jar) != OBJ_DATA::ITEM_FOUNTAIN)
	{
		weight_change_object(jar, - MIN(amount, GET_OBJ_WEIGHT(jar)));	// Subtract amount
	}

	if (
		(drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK]>0) && //���� �������� � ��������
		(
			// ��� ��� ��� �� ���������� ���� � �� ����� ��������
			(GET_DRUNK_STATE(ch) < CHAR_MORTALLY_DRUNKED && GET_DRUNK_STATE(ch) == GET_COND(ch, DRUNK)) ||
			// ��� ��� ��� �� ����
			(GET_COND(ch, DRUNK) < CHAR_DRUNKED)
		)
	)
	{
		// �� ������� ����� ������ �� 4, �� ������� ��� ������ 2
		gain_condition(ch, DRUNK, (int)((int) drink_aff[GET_OBJ_VAL(jar, 2)][DRUNK] * amount) / 2);
		GET_DRUNK_STATE(ch) = MAX(GET_DRUNK_STATE(ch), GET_COND(ch, DRUNK));
	}

	if (drink_aff[GET_OBJ_VAL(jar, 2)][FULL]!=0) {
		gain_condition(ch, FULL, drink_aff[GET_OBJ_VAL(jar, 2)][FULL] * amount);
		if (drink_aff[GET_OBJ_VAL(jar, 2)][FULL]<0 && GET_COND(ch, FULL) <= NORM_COND_VALUE)
			send_to_char("�� ���������� �������� ������� � �������.\r\n", ch);
	}

	if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST]!=0) {
		gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(jar, 2)][THIRST] * amount);
		if (drink_aff[GET_OBJ_VAL(jar, 2)][THIRST]<0 && GET_COND(ch, THIRST) <= NORM_COND_VALUE)
			send_to_char("�� �� ���������� �����.\r\n", ch);
	}
	
	// ���������
	do_drink_drunk(ch,jar,amount);
	// ����������
	do_drink_poison(ch,jar,amount);

	// empty the container, and no longer poison. 999 - whole fountain //
	if (GET_OBJ_TYPE(jar) != OBJ_DATA::ITEM_FOUNTAIN
		|| GET_OBJ_VAL(jar, 1) != 999)
	{
		jar->sub_val(1, amount);
	}
	if (!GET_OBJ_VAL(jar, 1))  	// The last bit //
	{
		jar->set_val(2, 0);
		jar->set_val(3, 0);
		name_from_drinkcon(jar);
	}

	return;
}

void do_drunkoff(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	OBJ_DATA *obj;
	struct timed_type timed;
	int amount, weight, prob, percent, duration;
	int on_ground = 0;

	if (IS_NPC(ch))		// Cannot use GET_COND() on mobs. //
		return;

	if (ch->get_fighting())
	{
		send_to_char("�� ����� ����������� � ���.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DRUNKED))
	{
		send_to_char("�� ������ ��������� ���� ���� �����?\r\n" "��� �� ���� �� ������!\r\n", ch);
		return;
	}

	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_ABSTINENT) && GET_COND(ch, DRUNK) < CHAR_DRUNKED)
	{
		send_to_char("�� ����� ������ ����� �� ������� ������.\r\n", ch);
		return;
	}

	if (timed_by_skill(ch, SKILL_DRUNKOFF))
	{
		send_to_char("�� �� � ��������� ��� ����� �����������.\r\n"
					 "��������� ����� ������������ ���.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg)
	{
		for (obj = ch->carrying; obj; obj = obj->get_next_content())
		{
			if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON)
			{
				break;
			}
		}
		if (!obj)
		{
			send_to_char("� ��� ��� ����������� ������� ��� ��������.\r\n", ch);
			return;
		}
	}
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents)))
		{
			send_to_char("�� �� ������ ��� �����!\r\n", ch);
			return;
		}
		else
		{
			on_ground = 1;
		}
	}

	if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_DRINKCON
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_FOUNTAIN)
	{
		send_to_char("���� �� ����-�� ������� �����������.\r\n", ch);
		return;
	}

	if (on_ground && (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON))
	{
		send_to_char("������ ��� ����� �������.\r\n", ch);
		return;
	}

	if (!GET_OBJ_VAL(obj, 1))
	{
		send_to_char("�����.\r\n", ch);
		return;
	}

	switch (GET_OBJ_VAL(obj, 2))
	{
	case LIQ_BEER:
	case LIQ_WINE:
	case LIQ_ALE:
	case LIQ_QUAS:
	case LIQ_BRANDY:
	case LIQ_VODKA:
	case LIQ_BRAGA:
		break;
	default:
		send_to_char("��������� �������� �������� :\r\n" "\"���� �������� ������...\"\r\n", ch);
		return;
	}

	amount = MAX(1, GET_WEIGHT(ch) / 50);
	if (amount > GET_OBJ_VAL(obj, 1))
	{
		send_to_char("��� ����� �� ������ ����� ���������� ��� ��������...\r\n", ch);
		return;
	}

	timed.skill = SKILL_DRUNKOFF;
	timed.time = can_use_feat(ch, DRUNKARD_FEAT) ? feature_mod(DRUNKARD_FEAT, FEAT_TIMER) : 12;
	timed_to_char(ch, &timed);

	percent = number(1, skill_info[SKILL_DRUNKOFF].max_percent);
	prob = train_skill(ch, SKILL_DRUNKOFF, skill_info[SKILL_DRUNKOFF].max_percent, 0);
	amount = MIN(amount, GET_OBJ_VAL(obj, 1));
	weight = MIN(amount, GET_OBJ_WEIGHT(obj));
	weight_change_object(obj, -weight);	// Subtract amount //
	obj->sub_val(1, amount);
	if (!GET_OBJ_VAL(obj, 1))  	// The last bit //
	{
		obj->set_val(2, 0);
		obj->set_val(3, 0);
		name_from_drinkcon(obj);
	}

	if (percent > prob)
	{
		sprintf(buf, "�� ���������� %s �� $o1, �� ���� ������ ����� ��� �������...", drinks[GET_OBJ_VAL(obj, 2)]);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		act("$n ����������$g �����������, �� ��� �� ����� $m �� ������.", FALSE, ch, 0, 0, TO_ROOM);
		duration = MAX(1, amount / 3);
		AFFECT_DATA<EApplyLocation> af[3];
		af[0].type = SPELL_ABSTINENT;
		af[0].duration = pc_duration(ch, duration, 0, 0, 0, 0);
		af[0].modifier = 0;
		af[0].location = APPLY_DAMROLL;
		af[0].bitvector = to_underlying(EAffectFlag::AFF_ABSTINENT);
		af[0].battleflag = 0;
		af[1].type = SPELL_ABSTINENT;
		af[1].duration = pc_duration(ch, duration, 0, 0, 0, 0);
		af[1].modifier = 0;
		af[1].location = APPLY_HITROLL;
		af[1].bitvector = to_underlying(EAffectFlag::AFF_ABSTINENT);
		af[1].battleflag = 0;
		af[2].type = SPELL_ABSTINENT;
		af[2].duration = pc_duration(ch, duration, 0, 0, 0, 0);
		af[2].modifier = 0;
		af[2].location = APPLY_AC;
		af[2].bitvector = to_underlying(EAffectFlag::AFF_ABSTINENT);
		af[2].battleflag = 0;
		switch (number(0, ch->get_skill(SKILL_DRUNKOFF) / 20))
		{
		case 0:
		case 1:
			af[0].modifier = -2;
		case 2:
		case 3:
			af[1].modifier = -2;
		default:
			af[2].modifier = 10;
		}
		for (prob = 0; prob < 3; prob++)
		{
			affect_join(ch, af[prob], TRUE, FALSE, TRUE, FALSE);
		}
		gain_condition(ch, DRUNK, amount);
	}
	else
	{
		sprintf(buf, "�� ���������� %s �� $o1 � ������������� �������� �������� �� ���� ����...",
			drinks[GET_OBJ_VAL(obj, 2)]);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		act("$n ��������$u � �������$g ���� �� ������.", FALSE, ch, 0, 0, TO_ROOM);
		affect_from_char(ch, SPELL_ABSTINENT);
	}

	return;
}

void generate_drinkcon_name(OBJ_DATA *to_obj, int spell)
{
	switch (spell)
	{
		// �������������� (�������) //
	case SPELL_REFRESH:
	case SPELL_GROUP_REFRESH:
		to_obj->set_val(2, LIQ_POTION_RED);
		name_to_drinkcon(to_obj, LIQ_POTION_RED);
		break;
		// ��������� (�����) //
	case SPELL_FULL:
	case SPELL_COMMON_MEAL:
		to_obj->set_val(2, LIQ_POTION_BLUE);
		name_to_drinkcon(to_obj, LIQ_POTION_BLUE);
		break;
		// ������� (�����) //
	case SPELL_DETECT_INVIS:
	case SPELL_ALL_SEEING_EYE:
	case SPELL_DETECT_MAGIC:
	case SPELL_MAGICAL_GAZE:
	case SPELL_DETECT_POISON:
	case SPELL_SNAKE_EYES:
	case SPELL_DETECT_ALIGN:
	case SPELL_GENERAL_SINCERITY:
	case SPELL_SENSE_LIFE:
	case SPELL_EYE_OF_GODS:
	case SPELL_INFRAVISION:
	case SPELL_SIGHT_OF_DARKNESS:
		to_obj->set_val(2, LIQ_POTION_WHITE);
		name_to_drinkcon(to_obj, LIQ_POTION_WHITE);
		break;
		// �������� (����������) //
	case SPELL_ARMOR:
	case SPELL_GROUP_ARMOR:
	case SPELL_CLOUDLY:
		to_obj->set_val(2, LIQ_POTION_GOLD);
		name_to_drinkcon(to_obj, LIQ_POTION_GOLD);
		break;
		// ����������������� �������� (������) //
	case SPELL_CURE_CRITIC:
	case SPELL_CURE_LIGHT:
	case SPELL_HEAL:
	case SPELL_GROUP_HEAL:
	case SPELL_CURE_SERIOUS:
		to_obj->set_val(2, LIQ_POTION_BLACK);
		name_to_drinkcon(to_obj, LIQ_POTION_BLACK);
		break;
		// ��������� ������� ������� (�����) //
	case SPELL_CURE_BLIND:
	case SPELL_REMOVE_CURSE:
	case SPELL_REMOVE_HOLD:
	case SPELL_REMOVE_SILENCE:
	case SPELL_CURE_PLAQUE:
	case SPELL_REMOVE_DEAFNESS:
	case SPELL_REMOVE_POISON:
		to_obj->set_val(2, LIQ_POTION_GREY);
		name_to_drinkcon(to_obj, LIQ_POTION_GREY);
		break;
		// ������ ���������� (����������) //
	case SPELL_INVISIBLE:
	case SPELL_GROUP_INVISIBLE:
	case SPELL_STRENGTH:
	case SPELL_GROUP_STRENGTH:
	case SPELL_FLY:
	case SPELL_GROUP_FLY:
	case SPELL_BLESS:
	case SPELL_GROUP_BLESS:
	case SPELL_HASTE:
	case SPELL_GROUP_HASTE:
	case SPELL_STONESKIN:
	case SPELL_STONE_WALL:
	case SPELL_BLINK:
	case SPELL_EXTRA_HITS:
	case SPELL_WATERBREATH:
		to_obj->set_val(2, LIQ_POTION_FUCHSIA);
		name_to_drinkcon(to_obj, LIQ_POTION_FUCHSIA);
		break;
	case SPELL_PRISMATICAURA:
	case SPELL_GROUP_PRISMATICAURA:
	case SPELL_AIR_AURA:
	case SPELL_EARTH_AURA:
	case SPELL_FIRE_AURA:
	case SPELL_ICE_AURA:
		to_obj->set_val(2, LIQ_POTION_PINK);
		name_to_drinkcon(to_obj, LIQ_POTION_PINK);
		break;
	default:
		to_obj->set_val(2, LIQ_POTION);
		name_to_drinkcon(to_obj, LIQ_POTION);	// ��������� ����� ������� //
	}
}

int check_potion_spell(OBJ_DATA *from_obj, OBJ_DATA *to_obj, int num)
{
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (GET_OBJ_VAL(from_obj, num) != to_obj->get_value(spell))
	{
		// �� ������� �����
		return 0;
	}
	if (GET_OBJ_VAL(from_obj, 0) < to_obj->get_value(level))
	{
		// ������������ ����� ���� ������ ����� � �������
		return -1;
	}
	return 1;
}

/// \return 1 - ����� ����������
///         0 - ������ ��������� ������ �����
///        -1 - ������� �������� ����� � ������� ������� �����
int check_equal_potions(OBJ_DATA *from_obj, OBJ_DATA *to_obj)
{
	// ������� � ��� ��������� ����� ������
	if (to_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) > 0
		&& GET_OBJ_VNUM(from_obj) != to_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM))
	{
		return 0;
	}
	// ���������� ������ � �� �������� ������
	for (int i = 1; i <= 3; ++i)
	{
		if (GET_OBJ_VAL(from_obj, i) > 0)
		{
			int result = check_potion_spell(from_obj, to_obj, i);
			if (result <= 0)
			{
				return result;
			}
		}
	}
	return 1;
}

/// �� check_equal_drinkcon()
int check_drincon_spell(OBJ_DATA *from_obj, OBJ_DATA *to_obj, int num)
{
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (from_obj->get_value(spell) != to_obj->get_value(spell))
	{
		// �� ������� �����
		return 0;
	}
	if (from_obj->get_value(level) < to_obj->get_value(level))
	{
		// ������������ ����� ���� ������ ����� � �������
		return -1;
	}
	return 1;
}

/// ��������� ������ check_equal_potions ��� ���� ��������, ���� � �������
/// ��� ��� values � �������/��������.
/// \return 1 - ����� ����������
///         0 - ������ ��������� ������ �����
///        -1 - ������� �������� ����� � ������� ������� �����
int check_equal_drinkcon(OBJ_DATA *from_obj, OBJ_DATA *to_obj)
{
	// ���������� ������ � �� �������� ������ (� � ��� �� �������, ��� ����)
	for (int i = 1; i <= 3; ++i)
	{
		if (GET_OBJ_VAL(from_obj, i) > 0)
		{
			int result = check_drincon_spell(from_obj, to_obj, i);
			if (result <= 0)
			{
				return result;
			}
		}
	}
	return 1;
}

/// ����������� ����� ���������� ��� ����������� �� ������� � ������ � ������ �������
void spells_to_drinkcon(OBJ_DATA *from_obj, OBJ_DATA *to_obj)
{
	// ���� ������
	for (int i = 1; i <= 3; ++i)
	{
		const auto spell = init_spell_num(i);
		const auto level = init_spell_lvl(i);
		to_obj->set_value(spell, from_obj->get_value(spell));
		to_obj->set_value(level, from_obj->get_value(level));
	}
	// ���������� ���� � �������������� ��������� �����
	const int proto_vnum = from_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) > 0
		? from_obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM)
		: GET_OBJ_VNUM(from_obj);
	to_obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, proto_vnum);
}

void do_pour(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *from_obj = NULL, *to_obj = NULL;
	int amount;

	two_arguments(argument, arg1, arg2);

	if (subcmd == SCMD_POUR)
	{
		if (!*arg1)  	// No arguments //
		{
			send_to_char("������ ����������?\r\n", ch);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		{
			send_to_char("� ��� ��� �����!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != OBJ_DATA::ITEM_DRINKCON
			&& GET_OBJ_TYPE(from_obj) != OBJ_DATA::ITEM_POTION)
		{
			send_to_char("�� �� ������ �� ����� ����������!\r\n", ch);
			return;
		}
	}
	if (subcmd == SCMD_FILL)
	{
		if (!*arg1)  	// no arguments //
		{
			send_to_char("��� � �� ���� �� ������ �� ���������?\r\n", ch);
			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		{
			send_to_char("� ��� ����� ���!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != OBJ_DATA::ITEM_DRINKCON)
		{
			act("�� �� ������ ��������� $o3!", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!*arg2)  	// no 2nd argument //
		{
			act("�� ���� �� ���������� ��������� $o3?", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room]->contents)))
		{
			sprintf(buf, "�� �� ������ ����� '%s'.\r\n", arg2);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != OBJ_DATA::ITEM_FOUNTAIN)
		{
			act("�� �� ������� ������ ��������� �� $o1.", FALSE, ch, from_obj, 0, TO_CHAR);
			return;
		}
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0)
	{
		act("�����.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
	}
	if (subcmd == SCMD_POUR)  	// pour //
	{
		if (!*arg2)
		{
			send_to_char("���� �� ������ ����? �� ����� ��� �� ���-��?\r\n", ch);
			return;
		}
		if (!str_cmp(arg2, "out") || !str_cmp(arg2, "�����"))
		{
			act("$n ���������$g $o3.", TRUE, ch, from_obj, 0, TO_ROOM);
			act("�� ���������� $o3.", FALSE, ch, from_obj, 0, TO_CHAR);

			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1));	// Empty //

			from_obj->set_val(1, 0);
			from_obj->set_val(2, 0);
			from_obj->set_val(3, 0);
			from_obj->set_skill(SKILL_INVALID);
			name_from_drinkcon(from_obj);
			reset_potion_values(from_obj);

			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
		{
			send_to_char("�� �� ������ ����� �����!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != OBJ_DATA::ITEM_DRINKCON
			&& GET_OBJ_TYPE(to_obj) != OBJ_DATA::ITEM_FOUNTAIN)
		{
			send_to_char("�� �� ������� � ��� ������.\r\n", ch);
			return;
		}
	}
	if (to_obj == from_obj)
	{
		send_to_char("����� ������ ������� �� ���������, �������, �� �����.\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 1) != 0
		&& GET_OBJ_TYPE(from_obj) != OBJ_DATA::ITEM_POTION
		&& GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))
	{
		send_to_char("�� ������� �������� �������, �� �� � ����� ����.\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(to_obj, 1) >= GET_OBJ_VAL(to_obj, 0))
	{
		send_to_char("��� ��� �����.\r\n", ch);
		return;
	}
		if (OBJ_FLAGGED(from_obj, EExtraFlag::ITEM_NOPOUR))
		{
			send_to_char(ch,"�� ����������� %s, ���������, �� ������ �������� �� �������.\r\n",
					GET_OBJ_PNAME(from_obj, 3).c_str());
			return;
		}
//Added by Adept - ����������� ����� �� ������� ��� ������� � �������

	//���������� �� ������� � ������ � �������
	if (GET_OBJ_TYPE(from_obj) == OBJ_DATA::ITEM_POTION)
	{
		int result = check_equal_potions(from_obj, to_obj);
		if (GET_OBJ_VAL(to_obj, 1) == 0 || result > 0)
		{
			send_to_char(ch, "�� �������� ������������ ����� � %s.\r\n",
				OBJN(to_obj, ch, 3));
				int n1 = GET_OBJ_VAL(from_obj, 1);
				int n2 = GET_OBJ_VAL(to_obj, 1);
				int t1 = GET_OBJ_VAL(from_obj, 3);
				int t2 = GET_OBJ_VAL(to_obj, 3);
				to_obj->set_val(3, (n1*t1 + n2*t2) / (n1 + n2)); //�������� ������ � ����������� �� ������������� ����� ��������
//				send_to_char(ch, "n1 == %d, n2 == %d, t1 == %d, t2== %d, ��������� %d\r\n", n1, n2, t1, t2, GET_OBJ_VAL(to_obj, 3));
			if (GET_OBJ_VAL(to_obj, 1) == 0)
			{
				copy_potion_values(from_obj, to_obj);
				// ����������� �������� ����� �� ������������� ���������� //
				generate_drinkcon_name(to_obj, GET_OBJ_VAL(from_obj, 1));
			}
			weight_change_object(to_obj, 1);
			to_obj->inc_val(1);
			extract_obj(from_obj);
			return;
		}
		else if (result < 0)
		{
			send_to_char("�� ��������� ��������� ����� ������ �����!\r\n", ch);
			return;
		}
		else
		{
			send_to_char(
				"��������� ������ �����?! �� ��, ��������, ������!\r\n", ch);
			return;
		}
	}

	//���������� �� ������� ��� ������� � ������ ����-��
	if ((GET_OBJ_TYPE(from_obj) == OBJ_DATA::ITEM_DRINKCON
		|| GET_OBJ_TYPE(from_obj) == OBJ_DATA::ITEM_FOUNTAIN)
		&& is_potion(from_obj))
	{
		if (GET_OBJ_VAL(to_obj, 1) == 0)
		{
			spells_to_drinkcon(from_obj, to_obj);
		}
		else
		{
			const int result = check_equal_drinkcon(from_obj, to_obj);
			if (result < 0)
			{
				send_to_char("�� ��������� ��������� ����� ������ �����!\r\n", ch);
				return;
			}
			else if (!result)
			{
				send_to_char(
					"��������� ������ �����?! �� ��, ��������, ������!\r\n", ch);
				return;
			}
		}
	}
//����� ��������� Adept'��

	if (subcmd == SCMD_POUR)
	{
		send_to_char(ch, "�� �������� ������������ %s � %s.\r\n",
			drinks[GET_OBJ_VAL(from_obj, 2)], OBJN(to_obj, ch, 3));
	}
	if (subcmd == SCMD_FILL)
	{
		act("�� ��������� $o3 �� $O1.", FALSE, ch, to_obj, from_obj, TO_CHAR);
		act("$n ��������$g $o3 �� $O1.", TRUE, ch, to_obj, from_obj, TO_ROOM);
	}

	// �������� ��� �������� //
	to_obj->set_val(2, GET_OBJ_VAL(from_obj, 2));

	int n1 = GET_OBJ_VAL(from_obj, 1);
	int n2 = GET_OBJ_VAL(to_obj, 1);
	int t1 = GET_OBJ_VAL(from_obj, 3);
	int t2 = GET_OBJ_VAL(to_obj, 3);
	to_obj->set_val(3, (n1*t1 + n2*t2) / (n1 + n2)); //�������� ������ � ����������� �� ������������� ����� ��������
//	send_to_char(ch, "n1 == %d, n2 == %d, t1 == %d, t2== %d, ��������� %d\r\n", n1, n2, t1, t2, GET_OBJ_VAL(to_obj, 3));

	// New alias //
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));
	// Then how much to pour //
	amount = (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1));
	if (GET_OBJ_TYPE(from_obj) != OBJ_DATA::ITEM_FOUNTAIN
		|| GET_OBJ_VAL(from_obj, 1) != 999)
	{
		from_obj->sub_val(1, amount);
	}
	to_obj->set_val(1, GET_OBJ_VAL(to_obj, 0));

	// Then the poison boogie //


	if (GET_OBJ_VAL(from_obj, 1) <= 0)  	// There was too little //
	{
		to_obj->add_val(1, GET_OBJ_VAL(from_obj, 1));
		amount += GET_OBJ_VAL(from_obj, 1);
		from_obj->set_val(1, 0);
		from_obj->set_val(2, 0);
		from_obj->set_val(3, 0);
		name_from_drinkcon(from_obj);
		reset_potion_values(from_obj);
	}

	// And the weight boogie //
	if (GET_OBJ_TYPE(from_obj) != OBJ_DATA::ITEM_FOUNTAIN)
	{
		weight_change_object(from_obj, -amount);
	}
	weight_change_object(to_obj, amount);	// Add weight //
}

size_t find_liquid_name(const char * name)
{
	std::string tmp = std::string(name);
	size_t pos, result = std::string::npos;
	for (int i = 0; strcmp(drinknames[i],"\n"); i++)
	{
		pos = tmp.find(drinknames[i]);
		if (pos != std::string::npos)
		{
			result = pos;
		}
	}
	return result;
}

void name_from_drinkcon(OBJ_DATA * obj)
{
	char new_name[MAX_STRING_LENGTH];
	std::string tmp;

	size_t pos = find_liquid_name(obj->get_aliases().c_str());
	if (pos == std::string::npos) return;
	tmp = obj->get_aliases().substr(0, pos - 1);

	sprintf(new_name, "%s", tmp.c_str());
	obj->set_aliases(new_name);

	pos = find_liquid_name(obj->get_short_description().c_str());
	if (pos == std::string::npos) return;
	tmp = obj->get_short_description().substr(0, pos - 3);

	sprintf(new_name, "%s", tmp.c_str());
	obj->set_short_description(new_name);

	for (int c = 0; c < CObjectPrototype::NUM_PADS; c++)
	{
		pos = find_liquid_name(obj->get_PName(c).c_str());
		if (pos == std::string::npos) return;
		tmp = obj->get_PName(c).substr(0, pos - 3);
		sprintf(new_name, "%s", tmp.c_str());
		obj->set_PName(c, new_name);
	}
}

void name_to_drinkcon(OBJ_DATA * obj, int type)
{
	int c;
	char new_name[MAX_INPUT_LENGTH];

	sprintf(new_name, "%s %s", obj->get_aliases().c_str(), drinknames[type]);
	obj->set_aliases(new_name);

	sprintf(new_name, "%s � %s", obj->get_short_description().c_str(), drinknames[type]);
	obj->set_short_description(new_name);

	for (c = 0; c < CObjectPrototype::NUM_PADS; c++)
	{
		sprintf(new_name, "%s � %s", obj->get_PName(c).c_str(), drinknames[type]);
		obj->set_PName(c, new_name);
	}
}

std::string print_spell(CHAR_DATA *ch, const OBJ_DATA *obj, int num)
{
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (obj->get_value(spell) == -1)
	{
		return "";
	}

	char buf_[MAX_INPUT_LENGTH];
	snprintf(buf_, sizeof(buf_), "�������� ����������: %s%s (%d ��.)%s\r\n",
		CCCYN(ch, C_NRM),
		spell_name(obj->get_value(spell)),
		obj->get_value(level),
		CCNRM(ch, C_NRM));

	return buf_;
}

namespace drinkcon
{

std::string print_spells(CHAR_DATA *ch, const OBJ_DATA *obj)
{
	std::string out;
	char buf_[MAX_INPUT_LENGTH];

	for (int i = 1; i <= 3; ++i)
	{
		out += print_spell(ch, obj, i);
	}

	if (!out.empty() && !is_potion(obj))
	{
		snprintf(buf_, sizeof(buf_), "%s��������%s: ��� �������� �� �������� ������\r\n",
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		out += buf_;
	}
	else if (out.empty() && is_potion(obj))
	{
		snprintf(buf_, sizeof(buf_), "%s��������%s: � ������� ����� ����������� ����������\r\n",
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		out += buf_;
	}

	return out;
}

void identify(CHAR_DATA *ch, const OBJ_DATA *obj)
{
	std::string out;
	char buf_[MAX_INPUT_LENGTH];

	snprintf(buf_, sizeof(buf_), "����� �������� �����: %s%d %s%s\r\n",
		CCCYN(ch, C_NRM),
		GET_OBJ_VAL(obj, 0), desc_count(GET_OBJ_VAL(obj, 0), WHAT_GULP),
		CCNRM(ch, C_NRM));
	out += buf_;

	// ������� �� �����
	if (GET_OBJ_VAL(obj, 1) > 0)
	{
		// ���� �����-�� �����
		if (obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) >= 0)
		{
			if (IS_IMMORTAL(ch))
			{
				snprintf(buf_, sizeof(buf_), "�������� %d %s %s (VNUM: %d).\r\n",
					GET_OBJ_VAL(obj, 1),
					desc_count(GET_OBJ_VAL(obj, 1), WHAT_GULP),
					drinks[GET_OBJ_VAL(obj, 2)],
					obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM));
			}
			else
			{
				snprintf(buf_, sizeof(buf_), "�������� %d %s %s.\r\n",
					GET_OBJ_VAL(obj, 1),
					desc_count(GET_OBJ_VAL(obj, 1), WHAT_GULP),
					drinks[GET_OBJ_VAL(obj, 2)]);
			}
			out += buf_;
			out += print_spells(ch, obj);
		}
		else
		{
			snprintf(buf_, sizeof(buf_), "�������� %s �� %d%%\r\n",
				drinknames[GET_OBJ_VAL(obj, 2)],
				GET_OBJ_VAL(obj, 1)*100/(GET_OBJ_VAL(obj, 0) + 1));
			out += buf_;
			// ����� ������ ������� �� ���� ����� ��� ������
			if (is_potion(obj))
			{
				out += print_spells(ch, obj);
			}
		}
	}
	if (GET_OBJ_VAL(obj, 1) >0) //���� ���-�� ����������
	{
		sprintf(buf1, "��������: %s \r\n", diag_liquid_timer(obj)); // ��������� �����
		out += buf1;
	}
	send_to_char(out, ch);
}

} // namespace drinkcon

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
