/* ************************************************************************
*   File: act.item.cpp                                  Part of Bylins    *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "logger.hpp"
#include "obj.hpp"
#include "char.hpp"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "depot.hpp"
#include "dg_scripts.h"
#include "features.hpp"
#include "fight.h"
#include "handler.h"
#include "house.h"
#include "im.h"
#include "interpreter.h"
#include "liquid.hpp"
#include "named_stuff.hpp"
#include "objsave.h"
#include "pk.h"
#include "poison.hpp"
#include "room.hpp"
#include "skills.h"
#include "spells.h"
#include "meat.maker.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "char_obj_utils.inl"

#include <boost/format.hpp>
// extern variables
extern struct house_control_rec house_control[];
extern std::array<int, MAX_MOB_LEVEL / 11 + 1> animals_levels;
// from act.informative.cpp
char *find_exdesc(char *word, const EXTRA_DESCR_DATA::shared_ptr& list);

// local functions
int can_take_obj(CHAR_DATA * ch, OBJ_DATA * obj);
void get_check_money(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *cont);
int perform_get_from_room(CHAR_DATA * ch, OBJ_DATA * obj);
void perform_give_gold(CHAR_DATA * ch, CHAR_DATA * vict, int amount);
void perform_give(CHAR_DATA * ch, CHAR_DATA * vict, OBJ_DATA * obj);
int perform_drop(CHAR_DATA * ch, OBJ_DATA * obj, byte mode, const int sname, room_rnum RDR);
void perform_drop_gold(CHAR_DATA * ch, int amount, byte mode, room_rnum RDR);
CHAR_DATA *give_find_vict(CHAR_DATA * ch, char *arg);
void weight_change_object(OBJ_DATA * obj, int weight);
int perform_put(CHAR_DATA * ch, OBJ_DATA::shared_ptr obj, OBJ_DATA * cont);
void perform_wear(CHAR_DATA * ch, OBJ_DATA * obj, int where);
int find_eq_pos(CHAR_DATA * ch, OBJ_DATA * obj, char *arg);
bool perform_get_from_container(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * cont, int mode);
void perform_remove(CHAR_DATA * ch, int pos);
int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
void feed_charmice(CHAR_DATA * ch, char *arg);
OBJ_DATA *create_skin(CHAR_DATA * mob, CHAR_DATA* ch);
int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
bool unique_stuff(const CHAR_DATA *ch, const OBJ_DATA *obj);

// from class.cpp
int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);

void do_split(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_split(CHAR_DATA *ch, char *argument, int cmd, int subcmd,int currency);
void do_remove(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_put(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_get(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_drop(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_give(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_eat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wear(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wield(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_grab(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_upgrade(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_refill(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

// ����� ������� ������������� �������� � ����-������,
// ����� ��� ��� ��� ��� ����� ���� �� ������ �������, ��� ����
// 0 - ��� ��, 1 - ������ �������� � ������ �� ������������ (��� ������), 2 - ������ �������� � ���� ������
int perform_put(CHAR_DATA * ch, OBJ_DATA::shared_ptr obj, OBJ_DATA * cont)
{
	if (!bloody::handle_transfer(ch, NULL, obj.get(), cont))
	{
		return 2;
	}

	if (!drop_otrigger(obj.get(), ch))
	{
		return 2;
	}

	// ���� ������ � �������� ������
	if (Clan::is_clan_chest(cont))
	{
		if (!Clan::PutChest(ch, obj.get(), cont))
		{
			return 1;
		}
		return 0;
	}

	// ����-��������� ��� �����
	if (ClanSystem::is_ingr_chest(cont))
	{
		if (!Clan::put_ingr_chest(ch, obj.get(), cont))
		{
			return 1;
		}
		return 0;
	}

	// ������������ ������
	if (Depot::is_depot(cont))
	{
		if (!Depot::put_depot(ch, obj))
		{
			return 1;
		}
		return 0;
	}

	if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0))
	{
		act("$O : $o �� ���������� ����.", FALSE, ch, obj.get(), cont, TO_CHAR);
	}
	else if (obj->get_type() == OBJ_DATA::ITEM_CONTAINER)
	{
		act("���������� �������� ��������� � ���������.", FALSE, ch, 0, 0, TO_CHAR);
	}
	else if (obj->get_extra_flag(EExtraFlag::ITEM_NODROP))
	{
		act("��������� ���� �������� �������� $o3 � $O3.", FALSE, ch, obj.get(), cont, TO_CHAR);
	}
	else if (obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY)
		|| obj->get_type() == OBJ_DATA::ITEM_KEY)
	{
		act("��������� ���� �������� �������� $o3 � $O3.", FALSE, ch, obj.get(), cont, TO_CHAR);
	}
	else
	{
		obj_from_char(obj.get());
		// ����� ��� �� 1 ���� ��� �� ����, ���� ��� ������������ �� ����, � �� � ��������� ������
		if (obj->get_type() == OBJ_DATA::ITEM_MONEY
			&& obj->get_vnum() == -1)
		{
			OBJ_DATA *temp, *obj_next;
			for (temp = cont->get_contains(); temp; temp = obj_next)
			{
				obj_next = temp->get_next_content();
				if (GET_OBJ_TYPE(temp) == OBJ_DATA::ITEM_MONEY)
				{
					// ��� ����� ������ � ���� ���������, �� ��� �������� ��� ��� ������ �� ���-��
					int money = GET_OBJ_VAL(temp, 0);
					money += GET_OBJ_VAL(obj, 0);
					obj_from_obj(temp);
					extract_obj(temp);
					obj_from_obj(obj.get());
					extract_obj(obj.get());
					obj = create_money(money);
					if (!obj)
					{
						return 0;
					}
					break;
				}
			}
		}
		obj_to_obj(obj.get(), cont);

		act("$n �������$g $o3 � $O3.", TRUE, ch, obj.get(), cont, TO_ROOM | TO_ARENA_LISTEN);

		// Yes, I realize this is strange until we have auto-equip on rent. -gg
		if (obj->get_extra_flag(EExtraFlag::ITEM_NODROP) && !cont->get_extra_flag(EExtraFlag::ITEM_NODROP))
		{
			cont->set_extra_flag(EExtraFlag::ITEM_NODROP);
			act("�� ������������� ���-�� ��������, ����� �������� $o3 � $O3.",
				FALSE, ch, obj.get(), cont, TO_CHAR);
		}
		else
			act("�� �������� $o3 � $O3.", FALSE, ch, obj.get(), cont, TO_CHAR);
		return 0;
	}
	return 2;
}
const int effects_l[5][40][2]{ 
	{{0,0}},
	{{0,	26}, // ���������� �����
	{APPLY_ABSORBE,	5},
	{APPLY_C1,	3},
	{APPLY_C2,	3},
	{APPLY_C3,	2},
	{APPLY_C4,	2},
	{APPLY_C5,	1},
	{APPLY_C6,	1},
	{APPLY_CAST_SUCCESS,	3},
	{APPLY_HIT,	20},
	{APPLY_HITREG,	35},
	{APPLY_INITIATIVE,	5},
	{APPLY_MANAREG,	15},
	{APPLY_MORALE,	5},
	{APPLY_MOVE,	35},
	{APPLY_RESIST_AIR,	15},
	{APPLY_RESIST_EARTH,	15},
	{APPLY_RESIST_FIRE,	15},
	{APPLY_RESIST_IMMUNITY,	5},
	{APPLY_RESIST_MIND,	5},
	{APPLY_RESIST_VITALITY,	5},
	{APPLY_RESIST_WATER,	15},
	{APPLY_SAVING_CRITICAL,	-5},
	{APPLY_SAVING_REFLEX,	-5},
	{APPLY_SAVING_STABILITY,	-5},
	{APPLY_SAVING_WILL,	-5},
	{APPLY_SIZE,	10},
	{APPLY_RESIST_DARK , 15},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0}},
	{{0,	37},
	{APPLY_ABSORBE,	10},
	{APPLY_C1,	3},
	{APPLY_C2,	3},
	{APPLY_C3,	3},
	{APPLY_C4,	3},
	{APPLY_C5,	2},
	{APPLY_C6,	2},
	{APPLY_C7,	2},
	{APPLY_C8,	1},
	{APPLY_C9,	1},
	{APPLY_CAST_SUCCESS,	5},
	{APPLY_CHA,	1},
	{APPLY_CON,	1},
	{APPLY_DAMROLL,	2},
	{APPLY_DEX,	1},
	{APPLY_HIT,	30},
	{APPLY_HITREG,	55},
	{APPLY_HITROLL,	2},
	{APPLY_INITIATIVE,	10},
	{APPLY_INT,	1},
	{APPLY_MANAREG,	30},
	{APPLY_MORALE,	7},
	{APPLY_MOVE,	55},
	{APPLY_RESIST_AIR,	25},
	{APPLY_RESIST_EARTH,	25},
	{APPLY_RESIST_FIRE,	25},
	{APPLY_RESIST_IMMUNITY,	10},
	{APPLY_RESIST_MIND,	10},
	{APPLY_RESIST_VITALITY,	10},
	{APPLY_RESIST_WATER,	25},
	{APPLY_SAVING_CRITICAL,	-10},
	{APPLY_SAVING_REFLEX,	-10},
	{APPLY_SAVING_STABILITY,	-10},
	{APPLY_SAVING_WILL,	-10},
	{APPLY_SIZE,	15},
	{APPLY_STR,	1},
	{APPLY_WIS,	1},
	{0 , 0},
	{0 , 0}},
	{{0,	23},
	{APPLY_ABSORBE,	15},
	{APPLY_C8,	2},
	{APPLY_C9,	2},
	{APPLY_CAST_SUCCESS,	7},
	{APPLY_CHA,	2},
	{APPLY_CON,	2},
	{APPLY_DAMROLL,	3},
	{APPLY_DEX,	2},
	{APPLY_HIT,	45},
	{APPLY_HITROLL,	3},
	{APPLY_INITIATIVE,	15},
	{APPLY_INT,	2},
	{APPLY_MORALE,	9},
	{APPLY_RESIST_IMMUNITY,	15},
	{APPLY_RESIST_MIND,	15},
	{APPLY_RESIST_VITALITY,	15},
	{APPLY_SAVING_CRITICAL,	-15},
	{APPLY_SAVING_REFLEX,	-15},
	{APPLY_SAVING_STABILITY,	-15},
	{APPLY_SAVING_WILL,	-15},
	{APPLY_SIZE,	20},
	{APPLY_STR,	2},
	{APPLY_WIS,	2},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0}},
	{{0,		21},
	{APPLY_ABSORBE,	20},
	{APPLY_CAST_SUCCESS,	10},
	{APPLY_CHA,	2},
	{APPLY_CON,	2},
	{APPLY_DAMROLL,	4},
	{APPLY_DEX,	2},
	{APPLY_HIT,	60},
	{APPLY_HITROLL,	4},
	{APPLY_INITIATIVE,	20},
	{APPLY_INT,	2},
	{APPLY_MORALE,	12},
	{APPLY_MR,	3},
	{APPLY_RESIST_IMMUNITY,	20},
	{APPLY_RESIST_MIND,	20},
	{APPLY_RESIST_VITALITY,	20},
	{APPLY_SAVING_CRITICAL,	-20},
	{APPLY_SAVING_REFLEX,	-20},
	{APPLY_SAVING_STABILITY,	-20},
	{APPLY_SAVING_WILL,	-20},
	{APPLY_STR,	2},
	{APPLY_WIS,	2},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0},
	{0 , 0}}

};

OBJ_DATA *create_skin(CHAR_DATA *mob, CHAR_DATA *ch)
{
	int vnum, i, k = 0, num, effect;
	bool concidence;
	const int vnum_skin_prototype = 1660;

	vnum = vnum_skin_prototype + MIN((int)(GET_LEVEL(mob) / 5), 9);
	const auto skin = world_objects.create_from_prototype_by_vnum(vnum);
	if (!skin)
	{
		mudlog("������� ����� ����� ��������� ��� ����������� � act.item.cpp::create_skin!",
			   NRM, LVL_GRGOD, ERRLOG, TRUE);
		return NULL;
	}

	skin->set_val(3, int(GET_LEVEL(mob) / 11)); // ��������� ������� �����, ������� 44+
	skin->set_parent(GET_MOB_VNUM(mob));
	trans_obj_name(skin.get(), mob); // ��������� ������
	for (i = 1; i <= GET_OBJ_VAL(skin, 3); i++) // ������� ����� �� 4� �������
	{
		if ((k == 1) && (number(1, 100) >= 35))
		{
			continue;
		}
		if ((k == 2) && (number(1, 100) >= 20))
		{
			continue;
		}
		if ((k == 3) && (number(1, 100) >= 10))
		{
			continue;
		}

		{
			concidence = true;
			while (concidence)
			{
				num = number(1, effects_l[GET_OBJ_VAL(skin, 3)][0][1]);
				concidence = false;
				for (int n = 0; n <= k && i > 1; n++)
				{
					if (effects_l[GET_OBJ_VAL(skin, 3)][num][0] == (skin)->get_affected(n).location)
					{
						concidence = true;
					}
				}
			}
			auto location = effects_l[GET_OBJ_VAL(skin, 3)][num][0];
			effect = effects_l[GET_OBJ_VAL(skin, 3)][num][1];
			if (number(0, 1000) <= (250 / (GET_OBJ_VAL(skin, 3) + 1))) //  ��� ����� ����� ��� ����  ������������� ������
			{
				effect *= -1;
			}
			skin->set_affected(k, static_cast<EApplyLocation>(location), effect);
			k++;
		}
	}

	skin->set_cost(GET_LEVEL(mob) * number(2, MAX(3, 3 * k)));
	skin->set_val(2, 95); //������� 5% ����� �������� �������� �� ����������� ������

	act("$n ����� ������$g $o3.", FALSE, ch, skin.get(), 0, TO_ROOM | TO_ARENA_LISTEN);
	act("�� ����� ������� $o3.", FALSE, ch, skin.get(), 0, TO_CHAR);
	
	//������ ������ "�� ������� �� ���������"
	skin->set_extra_flag(EExtraFlag::ITEM_NOT_DEPEND_RPOTO);
	return skin.get();
}

/* The following put modes are supported by the code below:

   1) put <object> <container>
   2) put all.<object> <container>
   3) put all <container>

   <container> must be in inventory or on ground.
   all objects to be put into container must be in inventory.
*/

void do_put(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char arg4[MAX_INPUT_LENGTH];
	OBJ_DATA *next_obj, *cont;
	CHAR_DATA *tmp_char;
	int obj_dotmode, cont_dotmode, found = 0, howmany = 1, money_mode = FALSE;
	char *theobj, *thecont, *theplace;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;


	argument = two_arguments(argument, arg1, arg2);
	argument = two_arguments(argument, arg3, arg4);

	if (is_number(arg1))
	{
		howmany = atoi(arg1);
		theobj = arg2;
		thecont = arg3;
		theplace = arg4;
	}
	else
	{
		theobj = arg1;
		thecont = arg2;
		theplace = arg3;
	}

	if (isname(theplace, "����� ������� room ground"))
		where_bits = FIND_OBJ_ROOM;
	else if (isname(theplace, "��������� inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(theplace, "���������� equipment"))
		where_bits = FIND_OBJ_EQUIP;


	if (theobj && (!strn_cmp("coin", theobj, 4) || !strn_cmp("���", theobj, 3)))
	{
		money_mode = TRUE;
		if (howmany <= 0)
		{
			send_to_char("������� ������� ����� ���������� �����.\r\n", ch);
			return;
		}
		if (ch->get_gold() < howmany)
		{
			send_to_char("��� � ��� ����� �����.\r\n", ch);
			return;
		}
		obj_dotmode = FIND_INDIV;
	}
	else
		obj_dotmode = find_all_dots(theobj);

	cont_dotmode = find_all_dots(thecont);

	if (!*theobj)
		send_to_char("�������� ��� � ����?\r\n", ch);
	else if (cont_dotmode != FIND_INDIV)
		send_to_char("�� ������ �������� ���� ������ � ���� ���������.\r\n", ch);
	else if (!*thecont)
	{
		sprintf(buf, "���� �� ������ �������� '%s'?\r\n", theobj);
		send_to_char(buf, ch);
	}
	else
	{
		generic_find(thecont, where_bits, ch, &tmp_char, &cont);
		if (!cont)
		{
			sprintf(buf, "�� �� ������ ����� '%s'.\r\n", thecont);
			send_to_char(buf, ch);
		}
		else if (GET_OBJ_TYPE(cont) != OBJ_DATA::ITEM_CONTAINER)
		{
			act("� $o3 ������ ������ ��������.", FALSE, ch, cont, 0, TO_CHAR);
		}
		else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
		{
			act("$o0 ������$A!", FALSE, ch, cont, 0, TO_CHAR);
		}
		else
		{
			if (obj_dotmode == FIND_INDIV)  	// put <obj> <container>
			{
				if (money_mode)
				{
					if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM))
					{
						act("��������� ���� �������� ��� ������� ���!!", FALSE,
							ch, 0, 0, TO_CHAR);
						return;
					}

					const auto obj = create_money(howmany);

					if (!obj)
					{
						return;
					}

					obj_to_char(obj.get(), ch);
					ch->remove_gold(howmany);

					// ���� �������� �� ������� - ���������� ��� ����
					if (perform_put(ch, obj, cont))
					{
						obj_from_char(obj.get());
						extract_obj(obj.get());
						ch->add_gold(howmany);
						return;
					}
				}
				else
				{
					auto obj = get_obj_in_list_vis(ch, theobj, ch->carrying);
					if (!obj)
					{
						sprintf(buf, "� ��� ��� '%s'.\r\n", theobj);
						send_to_char(buf, ch);
					}
					else if (obj == cont)
					{
						send_to_char("��� ����� ������ ��������� ���� ���� � ����.\r\n", ch);
					}
					else
					{
						OBJ_DATA *next_obj;
						while (obj && howmany--)
						{
							next_obj = obj->get_next_content();
							const auto object_ptr = world_objects.get_by_raw_ptr(obj);
							if (perform_put(ch, object_ptr, cont) == 1)
							{
								return;
							}
							obj = get_obj_in_list_vis(ch, theobj, next_obj);
						}
					}
				}
			}
			else
			{
				for (auto obj = ch->carrying; obj; obj = next_obj)
				{
					next_obj = obj->get_next_content();
					if (obj != cont
						&& CAN_SEE_OBJ(ch, obj)
						&& (obj_dotmode == FIND_ALL
							|| isname(theobj, obj->get_aliases())
							|| CHECK_CUSTOM_LABEL(theobj, obj, ch)))
					{
						found = 1;
						const auto object_ptr = world_objects.get_by_raw_ptr(obj);
						if (perform_put(ch, object_ptr, cont) == 1)
						{
							return;
						}
					}
				}

				if (!found)
				{
					if (obj_dotmode == FIND_ALL)
						send_to_char
						("����� �������� ���-�� �������� ����� ������ ���-�� ��������.\r\n",
						 ch);
					else
					{
						sprintf(buf, "�� �� ������ ������ �������� �� '%s'.\r\n", theobj);
						send_to_char(buf, ch);
					}
				}
			}
		}
	}
}

//���������� ������ �� ����� ����� 
//� ������
void do_refill(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *from_obj = NULL, *to_obj = NULL;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1)  	// No arguments //
	{
		send_to_char("������ ����� ������?\r\n", ch);
		return;
	}
	if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
	{
		send_to_char("� ��� ��� �����!\r\n", ch);
		return;
	}
	if (GET_OBJ_TYPE(from_obj) != OBJ_DATA::ITEM_MAGIC_ARROW)
	{
		send_to_char("� ��� �� ���� ��� �������������?\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0)
	{
		act("�����.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
	}
	if (!*arg2)
	{
		send_to_char("���� �� ������ �� ��������?\r\n", ch);
		return;
	}
	if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
	{
		send_to_char("�� �� ������ ����� �����!\r\n", ch);
		return;
	}
	if (!((GET_OBJ_TYPE(to_obj) == OBJ_DATA::ITEM_MAGIC_CONTAINER) || GET_OBJ_TYPE(to_obj) == OBJ_DATA::ITEM_MAGIC_ARROW))
	{
		send_to_char("�� �� ������� � ��� ������� ������.\r\n", ch);
		return;
	}

	if (to_obj == from_obj)
	{
		send_to_char("����� ��������? �� ���� ������ ��� �� ���������?\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 2) >= GET_OBJ_VAL(to_obj, 1))
	{
		send_to_char("��� ��� �����.\r\n", ch);
		return;
	}
	else //����� ������ ��� ��������. �������� �������������
	{
		if (GET_OBJ_VAL(from_obj, 0) != GET_OBJ_VAL(to_obj, 0))
		{
			send_to_char("������ ������� ��� �� �������� �� ����.\r\n", ch);
			return;
		}
		const int t1 = GET_OBJ_VAL(from_obj, 3);  // ���������� �������
		const int t2 = GET_OBJ_VAL(to_obj, 3);
		const int delta = (GET_OBJ_VAL(to_obj, 2) - GET_OBJ_VAL(to_obj, 3));
		if (delta >= t1) //����� ������� ������ �����
		{
			to_obj->add_val(2, t1);
			send_to_char("�� ��������� ������� ������ � ������.\r\n", ch);
			extract_obj(from_obj);
			return;
		}
		else
		{
			to_obj->add_val(2, (t2 - GET_OBJ_VAL(to_obj, 2)));
			send_to_char("�� ��������� ���������� ��������� ����� � ������.\r\n", ch);
			from_obj->add_val(2, (GET_OBJ_VAL(to_obj, 2) - t2));
			return;
		}
	}
}


int can_take_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (!IS_NPC(ch) && CLAN(ch))
		sprintf(buf, "clan%d!", CLAN(ch)->GetRent());

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MONEY)
	{
		act("$p: �� �� ������ ����� ������� �����.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MONEY)
	{
		act("$p: �� �� � ��������� ����� ��� � $S.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	else if (!(CAN_WEAR(obj, EWearFlag::ITEM_WEAR_TAKE)))
	{
		act("$p: �� �� ������ ����� $S.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	else if (invalid_anti_class(ch, obj))
	{
		act("$p: ��� ���� �� ������������� ��� ���!", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	else if (NamedStuff::check_named(ch, obj, false))
	{
		if(!NamedStuff::wear_msg(ch, obj))
			act("$p: ��� ���� �� ������������� ��� ���!", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	else if (invalid_unique(ch, obj) || (strstr(obj->get_aliases().c_str(), "clan") && (IS_NPC(ch) || !CLAN(ch) || !strstr(obj->get_aliases().c_str(), buf))))
	{
		act("��� ������� ��� ������� ����� $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n �������$u ����� $o3 - � ����� �� ������$g.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		return (0);
	}
	return (1);
}

/// ������� ������� � ch � ������ ��� ������� (�� �����)
int other_pc_in_group(CHAR_DATA *ch)
{
	int num = 0;
	CHAR_DATA *k = ch->has_master() ? ch->get_master() : ch;
	for (follow_type *f = k->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
			&& !IS_NPC(f->follower)
			&& IN_ROOM(f->follower) == ch->in_room)
		{
			++num;
		}
	}
	return num;
}

void split_or_clan_tax(CHAR_DATA *ch, long amount)
{
	if (IS_AFFECTED(ch, AFF_GROUP)
		&& other_pc_in_group(ch) > 0
		&& PRF_FLAGGED(ch, PRF_AUTOSPLIT))
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%ld", amount);
		do_split(ch, buf_, 0, 0);
	}
	else
	{
		const long tax = ClanSystem::do_gold_tax(ch, amount);
		ch->remove_gold(tax);
	}
}


void get_check_money(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *cont)
{

	if (system_obj::is_purse(obj) && GET_OBJ_VAL(obj, 3) == ch->get_uid())
	{
		system_obj::process_open_purse(ch, obj);
		return;
	}

	const int value = GET_OBJ_VAL(obj, 0);
	const int curr_type = GET_OBJ_VAL(obj, 1);

	if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MONEY
		|| value <= 0)
	{
		return;
	}
	
	if (curr_type == currency::ICE) {
		sprintf(buf, "��� ��������� %d %s.\r\n", value, desc_count(value, WHAT_ICEu));
		send_to_char(buf, ch);
		ch->add_ice_currency(value);
		//������ ��� ������!
		if (IS_AFFECTED(ch, AFF_GROUP) && other_pc_in_group(ch) > 0) {
			char local_buf[256];
			sprintf(local_buf, "%d", value);
			do_split(ch, local_buf, 0, 0,curr_type);
		}
		extract_obj(obj);
		return;
	}

// ��� ��� ���������� - ���� (��� �������������)
/*	if (curr_type != currency::GOLD) {
		//��� ��� ������������ ������
		return;
	}
*/
	sprintf(buf, "��� ��������� %d %s.\r\n", value, desc_count(value, WHAT_MONEYu));
	send_to_char(buf, ch);

	// ���, ��� ������� �� ������ - ���� ����� ����� (�� ��������� �� �������)
	if (IS_AFFECTED(ch, AFF_GROUP) && other_pc_in_group(ch) > 0
		&& PRF_FLAGGED(ch, PRF_AUTOSPLIT)
		&& (!cont || !system_obj::is_purse(cont)))
	{
		// ��������� �����, ����� � ���, ����-����� �������
		// ������ �� ����� ������� �� ������ � do_split()
		ch->add_gold(value);
		sprintf(buf, "<%s> {%d} ��������� %d %s � ������.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), value,desc_count(value, WHAT_MONEYu));
		mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
		char local_buf[256];
		sprintf(local_buf, "%d", value);
		do_split(ch, local_buf, 0, 0);
	}
	else if ((cont && IS_MOB_CORPSE(cont)) || GET_OBJ_VNUM(obj) != -1)
	{
		// ��� �� ����� ���� ��� �� ���������-����� � ������
		// (��������-������� � �����) - ������� ����-�����
/*		int mob_num = GET_OBJ_VAL(cont, 2);
		CHAR_DATA *mob;
		if (mob_num  <= 0)
		{
			sprintf(buf, "<%s> {%d} ��������� %d %s.", ch->get_name(), GET_ROOM_VNUM(ch->in_room), value,desc_count(value, WHAT_MONEYu));
			mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
		}
		else
		{
			mob = read_mobile(mob_num, VIRTUAL);
			sprintf(buf, "<%s> {%d} ������� %d %s ��� �� ����� ����. [���: %s, Vnum: %d]", ch->get_name(), GET_ROOM_VNUM(ch->in_room), value,desc_count(value, WHAT_MONEYu),  GET_NAME(mob), mob_num);
			mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
			extract_char(mob, FALSE);
		}
*/
		sprintf(buf, "%s ��������� %d  %s.", ch->get_name().c_str(),  value,desc_count(value, WHAT_MONEYu));
		mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
		ch->add_gold(value, true, true);
	}
	else
	{
		sprintf(buf, "<%s> {%d} ���-�� ������� %d  %s.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), value,desc_count(value, WHAT_MONEYu));
		mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
		ch->add_gold(value);
	}

	obj_from_char(obj);
	extract_obj(obj);
}


// return 0 - ����� ������� ������������� ����� �� ����-�������,
// ����� ��� �� ��� ��� ����� ���� �� ������ �������, ��� ����
bool perform_get_from_container(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * cont, int mode)
{
	if (!bloody::handle_transfer(NULL, ch, obj))
		return false;
	if ((mode == FIND_OBJ_INV || mode == FIND_OBJ_ROOM || mode == FIND_OBJ_EQUIP) && can_take_obj(ch, obj) && get_otrigger(obj, ch))
	{
		// ���� ����� �� ����-�������
		if (Clan::is_clan_chest(cont))
		{
			if (!Clan::TakeChest(ch, obj, cont))
			{
				return false;
			}
			return true;
		}
		// ����-��������� ������
		if (ClanSystem::is_ingr_chest(cont))
		{
			if (!Clan::take_ingr_chest(ch, obj, cont))
			{
				return false;
			}
			return true;
		}
		obj_from_obj(obj);
		obj_to_char(obj, ch);
		if (obj->get_carried_by() == ch)
		{
			if (bloody::is_bloody(obj))
			{
				act("�� ����� $o3 �� $O1, �������� ���� ���� ������!", FALSE, ch, obj, cont, TO_CHAR);
				act("$n ����$g $o3 �� $O1, �������� ���� ������.", TRUE, ch, obj, cont, TO_ROOM | TO_ARENA_LISTEN);
			} else
			{
				act("�� ����� $o3 �� $O1.", FALSE, ch, obj, cont, TO_CHAR);
				act("$n ����$g $o3 �� $O1.", TRUE, ch, obj, cont, TO_ROOM | TO_ARENA_LISTEN);
			}
			get_check_money(ch, obj, cont);
		}
	}
	return true;
}

// *\param autoloot - true ������ ��� ������ ������ �� ����� � ������ �����������
void get_from_container(CHAR_DATA * ch, OBJ_DATA * cont, char *arg, int mode, int howmany, bool autoloot)
{
	if (Depot::is_depot(cont))
	{
		Depot::take_depot(ch, arg, howmany);
		return;
	}

	OBJ_DATA *obj, *next_obj;
	int obj_dotmode, found = 0;

	obj_dotmode = find_all_dots(arg);
	if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
		act("$o ������$A.", FALSE, ch, cont, 0, TO_CHAR);
	else if (obj_dotmode == FIND_INDIV)
	{
		if (!(obj = get_obj_in_list_vis(ch, arg, cont->get_contains())))
		{
			sprintf(buf, "�� �� ������ '%s' � $o5.", arg);
			act(buf, FALSE, ch, cont, 0, TO_CHAR);
		}
		else
		{
			OBJ_DATA *obj_next;
			while (obj && howmany--)
			{
				obj_next = obj->get_next_content();
				if (!perform_get_from_container(ch, obj, cont, mode))
					return;
				obj = get_obj_in_list_vis(ch, arg, obj_next);
			}
		}
	}
	else
	{
		if (obj_dotmode == FIND_ALLDOT && !*arg)
		{
			send_to_char("����� ��� \"���\"?\r\n", ch);
			return;
		}
		for (obj = cont->get_contains(); obj; obj = next_obj)
		{
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (obj_dotmode == FIND_ALL
					|| isname(arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(arg, obj, ch)))
			{
				if (autoloot
					&& (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_INGREDIENT
						|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_MING)
					&& PRF_FLAGGED(ch, PRF_NOINGR_LOOT))
				{
					continue;
				}
				found = 1;
				if (!perform_get_from_container(ch, obj, cont, mode))
				{
					return;
				}
			}
		}
		if (!found)
		{
			if (obj_dotmode == FIND_ALL)
				act("$o ����$A.", FALSE, ch, cont, 0, TO_CHAR);
			else
			{
				sprintf(buf, "�� �� ������ ������ �������� �� '%s' � $o5.", arg);
				act(buf, FALSE, ch, cont, 0, TO_CHAR);
			}
		}
	}
}


int perform_get_from_room(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (can_take_obj(ch, obj) && get_otrigger(obj, ch) && bloody::handle_transfer(NULL, ch, obj))
	{
		obj_from_room(obj);
		obj_to_char(obj, ch);
		if (obj->get_carried_by() == ch)
		{
			if (bloody::is_bloody(obj))
			{
				act("�� ������� $o3, �������� ���� ���� ������!", FALSE, ch, obj, 0, TO_CHAR);
				act("$n ������$g $o3, �������� ���� ������.", TRUE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
			} else {
				act("�� ������� $o3.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n ������$g $o3.", TRUE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
			}
			get_check_money(ch, obj, 0);
			return (1);
		}
	}
	return (0);
}


void get_from_room(CHAR_DATA * ch, char *arg, int howmany)
{
	OBJ_DATA *obj, *next_obj;
	int dotmode, found = 0;

	// Are they trying to take something in a room extra description?
	if (find_exdesc(arg, world[ch->in_room]->ex_description) != NULL)
	{
		send_to_char("�� �� ������ ��� �����.\r\n", ch);
		return;
	}

	dotmode = find_all_dots(arg);

	if (dotmode == FIND_INDIV)
	{
		if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents)))
		{
			sprintf(buf, "�� �� ������ ����� '%s'.\r\n", arg);
			send_to_char(buf, ch);
		}
		else
		{
			OBJ_DATA *obj_next;
			while (obj && howmany--)
			{
				obj_next = obj->get_next_content();
				perform_get_from_room(ch, obj);
				obj = get_obj_in_list_vis(ch, arg, obj_next);
			}
		}
	}
	else
	{
		if (dotmode == FIND_ALLDOT && !*arg)
		{
			send_to_char("����� ��� \"���\"?\r\n", ch);
			return;
		}
		for (obj = world[ch->in_room]->contents; obj; obj = next_obj)
		{
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
				&& (dotmode == FIND_ALL
					|| isname(arg, obj->get_aliases())
					|| CHECK_CUSTOM_LABEL(arg, obj, ch)))
			{
				found = 1;
				perform_get_from_room(ch, obj);
			}
		}
		if (!found)
		{
			if (dotmode == FIND_ALL)
			{
				send_to_char("������, ����� ������ ���.\r\n", ch);
			}
			else
			{
				sprintf(buf, "�� �� ����� ����� '%s'.\r\n", arg);
				send_to_char(buf, ch);
			}
		}
	}
}

void do_mark(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	int cont_dotmode, found = 0;
	OBJ_DATA *cont;
	CHAR_DATA *tmp_char;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1)
	{
		send_to_char("��� �� ������ �����������?\r\n", ch);
	}
	else if (!*arg2 || !is_number(arg2))
	{
		send_to_char("�� ������ ��� �������� ������.\r\n", ch);
	}
	else
	{
		cont_dotmode = find_all_dots(arg1);
		if (cont_dotmode == FIND_INDIV)
		{
			generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
			if (!cont)
			{
				sprintf(buf, "� ��� ��� '%s'.\r\n", arg1);
				send_to_char(buf, ch);
				return;
			}
			cont->set_owner(atoi(arg2));
			act("�� �������� $o3.", FALSE, ch, cont, 0, TO_CHAR);
		}
		else
		{
			if (cont_dotmode == FIND_ALLDOT && !*arg1)
			{
				send_to_char("�������� ��� \"���\"?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont; cont = cont->get_next_content())
			{
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(arg1, cont->get_aliases())))
				{
					cont->set_owner(atoi(arg2));
					act("�� �������� $o3.", FALSE, ch, cont, 0, TO_CHAR);
					found = TRUE;
				}
			}
			for (cont = world[ch->in_room]->contents; cont; cont = cont->get_next_content())
			{
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(arg2, cont->get_aliases())))
				{
					cont->set_owner(atoi(arg2));
					act("�� �������� $o3.", FALSE, ch, cont, 0, TO_CHAR);
					found = TRUE;
				}
			}
			if (!found)
			{
				if (cont_dotmode == FIND_ALL)
				{
					send_to_char("�� �� ������ ����� ������ ��� ����������.\r\n", ch);
				}
				else
				{
					sprintf(buf, "�� ���-�� �� ������ ����� '%s'.\r\n", arg1);
					send_to_char(buf, ch);
				}
			}
		}
	}
}

void do_get(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char arg4[MAX_INPUT_LENGTH];
	char *theobj, *thecont, *theplace;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;

	int cont_dotmode, found = 0, mode, amount = 1;
	OBJ_DATA *cont;
	CHAR_DATA *tmp_char;

	argument = two_arguments(argument, arg1, arg2);
	argument = two_arguments(argument, arg3, arg4);

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		send_to_char("� ��� ������ ����!\r\n", ch);
	else if (!*arg1)
		send_to_char("��� �� ������ �����?\r\n", ch);
	else if (!*arg2 || isname(arg2, "����� ������� ground room"))
		get_from_room(ch, arg1, 1);
	else if (is_number(arg1) && (!*arg3 || isname(arg3, "����� ������� ground room")))
		get_from_room(ch, arg2, atoi(arg1));
	else if ((!*arg3 && isname(arg2, "��������� ���������� inventory equipment")) ||
			 (is_number(arg1) && !*arg4 && isname(arg3, "��������� ���������� inventory equipment")))
		send_to_char("�� ��� ��������� ���� �������!\r\n", ch);
	else
	{
		if (is_number(arg1))
		{
			amount = atoi(arg1);
			theobj = arg2;
			thecont = arg3;
			theplace = arg4;
		}
		else
		{
			theobj = arg1;
			thecont = arg2;
			theplace = arg3;
		}


		if (isname(theplace, "����� ������� room ground"))
			where_bits = FIND_OBJ_ROOM;
		else if (isname(theplace, "��������� inventory"))
			where_bits = FIND_OBJ_INV;
		else if (isname(theplace, "���������� equipment"))
			where_bits = FIND_OBJ_EQUIP;

		cont_dotmode = find_all_dots(thecont);
		if (cont_dotmode == FIND_INDIV)
		{
			mode = generic_find(thecont, where_bits, ch, &tmp_char, &cont);
			if (!cont)
			{
				sprintf(buf, "�� �� ������ '%s'.\r\n", arg2);
				send_to_char(buf, ch);
			}
			else if (GET_OBJ_TYPE(cont) != OBJ_DATA::ITEM_CONTAINER)
			{
				act("$o - �� ���������.", FALSE, ch, cont, 0, TO_CHAR);
			}
			else
			{
				get_from_container(ch, cont, theobj, mode, amount, false);
			}
		}
		else
		{
			if (cont_dotmode == FIND_ALLDOT
				&& !*thecont)
			{
				send_to_char("����� �� ���� \"�����\"?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont && IS_SET(where_bits, FIND_OBJ_INV); cont = cont->get_next_content())
			{
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch)))
				{
					if (GET_OBJ_TYPE(cont) == OBJ_DATA::ITEM_CONTAINER)
					{
						found = 1;
						get_from_container(ch, cont, theobj, FIND_OBJ_INV, amount, false);
					}
					else if (cont_dotmode == FIND_ALLDOT)
					{
						found = 1;
						act("$o - �� ���������.", FALSE, ch, cont, 0, TO_CHAR);
					}
				}
			}
			for (cont = world[ch->in_room]->contents; cont && IS_SET(where_bits, FIND_OBJ_ROOM); cont = cont->get_next_content())
			{
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == FIND_ALL
						|| isname(thecont, cont->get_aliases())
						|| CHECK_CUSTOM_LABEL(thecont, cont, ch)))
				{
					if (GET_OBJ_TYPE(cont) == OBJ_DATA::ITEM_CONTAINER)
					{
						get_from_container(ch, cont, theobj, FIND_OBJ_ROOM, amount, false);
						found = 1;
					}
					else if (cont_dotmode == FIND_ALLDOT)
					{
						act("$o - �� ���������.", FALSE, ch, cont, 0, TO_CHAR);
						found = 1;
					}
				}
			}
			if (!found)
			{
				if (cont_dotmode == FIND_ALL)
				{
					send_to_char("�� �� ������ ����� �� ������ ����������.\r\n", ch);
				}
				else
				{
					sprintf(buf, "�� ���-�� �� ������ ����� '%s'.\r\n", thecont);
					send_to_char(buf, ch);
				}
			}
		}
	}
}


void perform_drop_gold(CHAR_DATA * ch, int amount, byte mode, room_rnum RDR)
{
	if (amount <= 0)
	{
		send_to_char("��, ������ �� ������� ���������� �������.\r\n", ch);
	}
	else if (ch->get_gold() < amount)
	{
		send_to_char("� ��� ��� ����� �����!\r\n", ch);
	}
	else
	{
		if (mode != SCMD_JUNK)
		{
			WAIT_STATE(ch, PULSE_VIOLENCE);	// to prevent coin-bombing
			if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM))
			{
				act("��������� ���� �������� ��� ������� ���!", FALSE, ch, 0, 0, TO_CHAR);
				return;
			}
			//������� ������� ����� � �������
			int additional_amount = 0;
			OBJ_DATA* next_obj;
			if (mode != SCMD_DONATE)
			{
				for (OBJ_DATA* existing_obj = world[ch->in_room]->contents; existing_obj; existing_obj = next_obj)
				{
					next_obj = existing_obj->get_next_content();
					if (GET_OBJ_TYPE(existing_obj) == OBJ_DATA::ITEM_MONEY && GET_OBJ_VAL(existing_obj, 1) == currency::GOLD)
					{
						//���������� ��������� ������������ ����� � ������� ��
						additional_amount = GET_OBJ_VAL(existing_obj, 0);
						obj_from_room(existing_obj);
						extract_obj(existing_obj);
					}
				}
			}

			const auto obj = create_money(amount+additional_amount);

			if (mode == SCMD_DONATE)
			{
				sprintf(buf, "�� ��������� %d %s �� �����.\r\n", amount,
						desc_count(amount, WHAT_MONEYu));
				send_to_char(buf, ch);
				act("$n ��������$g ������... �� ����� :(", FALSE, ch, 0, 0, TO_ROOM);
				obj_to_room(obj.get(), RDR);
				act("$o �����$Q � ������ ����!", 0, 0, obj.get(), 0, TO_ROOM);
			}
			else
			{
				const int result = drop_wtrigger(obj.get(), ch);

				if (!result)
				{
					extract_obj(obj.get());
					return;
				}

				// ���� ���� ��� ����� �� �������, �� �� �������� ��������� ����� ������ ������ ��������� � ��� � � ������
				if (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_CORPSE))
				{
					send_to_char(ch, "�� ������� %d %s �� �����.\r\n",
						amount, desc_count(amount, WHAT_MONEYu));
					sprintf(buf, "<%s> {%d} �������� %d %s �� �����.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), amount, desc_count(amount, WHAT_MONEYu));
					mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
					sprintf(buf, "$n ������$g %s �� �����.", money_desc(amount, 3));
					act(buf, TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				}
				obj_to_room(obj.get(), ch->in_room);
			}
		}
		else
		{
			sprintf(buf, "$n �����������$g %s... � ������� �����!", money_desc(amount, 3));
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			sprintf(buf, "�� ������������ ����� %d %s.\r\n", amount, desc_count(amount, WHAT_MONEYu));
			send_to_char(buf, ch);
		}
		ch->remove_gold(amount);
	}
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
            "  It vanishes in a puff of smoke!" : "")

const char *drop_op[3][3] =
{
	{"���������", "���������", "��������"},
	{"������������", "������������", "�����������"},
	{"�������", "�������", "������"}
};

int perform_drop(CHAR_DATA * ch, OBJ_DATA * obj, byte mode, const int sname, room_rnum RDR)
{
	int value;
	if (!drop_otrigger(obj, ch))
		return 0;
	if (!bloody::handle_transfer(ch, NULL, obj))
		return 0;
	if ((mode == SCMD_DROP && !drop_wtrigger(obj, ch)))
		return 0;
	if (obj->get_extra_flag(EExtraFlag::ITEM_NODROP))
	{
		sprintf(buf, "�� �� ������ %s $o3!", drop_op[sname][0]);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	sprintf(buf, "�� %s $o3.%s", drop_op[sname][1], VANISH(mode));
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	sprintf(buf, "$n %s$g $o3.%s", drop_op[sname][2], VANISH(mode));
	act(buf, TRUE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
	obj_from_char(obj);

	if ((mode == SCMD_DONATE) && obj->get_extra_flag(EExtraFlag::ITEM_NODONATE))
		mode = SCMD_JUNK;

	switch (mode)
	{
	case SCMD_DROP:
		obj_to_room(obj, ch->in_room);
		obj_decay(obj);
		return (0);
	case SCMD_DONATE:
		obj_to_room(obj, RDR);
		obj_decay(obj);
		act("$o ���������$U � ������ ����!", FALSE, 0, obj, 0, TO_ROOM);
		return (0);
	case SCMD_JUNK:
		value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
		extract_obj(obj);
		return (value);
	default:
		log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
		break;
	}

	return (0);
}

void do_drop(CHAR_DATA *ch, char* argument, int/* cmd*/, int subcmd)
{
	OBJ_DATA *obj, *next_obj;
	const room_rnum RDR = 0;
	byte mode = SCMD_DROP;
	int dotmode, amount = 0, multi;
	int sname;

	switch (subcmd)
	{
	case SCMD_JUNK:
		sname = 0;
		mode = SCMD_JUNK;
		break;
	case SCMD_DONATE:
		sname = 1;
		mode = SCMD_DONATE;
		switch (number(0, 2))
		{
		case 0:
			mode = SCMD_JUNK;
			break;
		case 1:
		case 2:
			// ��� ���� donation_room_1
			break;
		}
		if (RDR == NOWHERE)
		{
			send_to_char("�� �� ������ ����� ����� �������.\r\n", ch);
			return;
		}
		break;
	default:
		sname = 2;
		break;
	}

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		sprintf(buf, "��� �� ������ %s?\r\n", drop_op[sname][0]);
		send_to_char(buf, ch);
		return;
	}
	else if (is_number(arg))
	{
		multi = atoi(arg);
		one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg) || !str_cmp("���", arg) || !str_cmp("�����", arg))
			perform_drop_gold(ch, multi, mode, RDR);
		else if (multi <= 0)
			send_to_char("�� ����� ������.\r\n", ch);
		else if (!*arg)
		{
			sprintf(buf, "%s %d ����?\r\n", drop_op[sname][0], multi);
			send_to_char(buf, ch);
		}
		else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
		{
			sprintf(buf, "� ��� ��� ������ �������� �� %s.\r\n", arg);
			send_to_char(buf, ch);
		}
		else
		{
			do
			{
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				amount += perform_drop(ch, obj, mode, sname, RDR);
				obj = next_obj;
			}
			while (obj && --multi);
		}
	}
	else
	{
		dotmode = find_all_dots(arg);
		// Can't junk or donate all
		if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE))
		{
			if (subcmd == SCMD_JUNK)
				send_to_char("��� � ����������� ����. � ��������� :)\r\n", ch);
			else
				send_to_char("����� ������ � ������� �� ����!\r\n", ch);
			return;
		}
		if (dotmode == FIND_ALL)
		{
			if (!ch->carrying)
				send_to_char("� � ��� ������ � ���.\r\n", ch);
			else
				for (obj = ch->carrying; obj; obj = next_obj)
				{
					next_obj = obj->get_next_content();
					amount += perform_drop(ch, obj, mode, sname, RDR);
				}
		}
		else if (dotmode == FIND_ALLDOT)
		{
			if (!*arg)
			{
				sprintf(buf, "%s \"���\" ������ ���� ���������?\r\n", drop_op[sname][0]);
				send_to_char(buf, ch);
				return;
			}
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
			{
				sprintf(buf, "� ��� ��� ������ �������� �� '%s'.\r\n", arg);
				send_to_char(buf, ch);
			}
			while (obj)
			{
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				amount += perform_drop(ch, obj, mode, sname, RDR);
				obj = next_obj;
			}
		}
		else
		{
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
			{
				sprintf(buf, "� ��� ��� '%s'.\r\n", arg);
				send_to_char(buf, ch);
			}
			else
				amount += perform_drop(ch, obj, mode, sname, RDR);
		}
	}

	if (amount && (subcmd == SCMD_JUNK))
	{
		send_to_char("���� �� �������� �������� �� ���� ����.\r\n", ch);
		act("$n ������$q ������. �� ���� ���� ����� � �$m!", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
}

void perform_give(CHAR_DATA * ch, CHAR_DATA * vict, OBJ_DATA * obj)
{
	if (!bloody::handle_transfer(ch, vict, obj))
		return;
	if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM) && !IS_GOD(ch))
	{
		act("��������� ���� �������� ��� ������� ���!", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (obj->get_extra_flag(EExtraFlag::ITEM_NODROP))
	{
		act("�� �� ������ �������� $o3!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict))
	{
		act("� $N1 ������ ����.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
	{
		act("$E �� ����� ����� ����� ���.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (!give_otrigger(obj, ch, vict))
	{
		act("$E �� ����� ����� ���� � ���� �����.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (!receive_mtrigger(vict, ch, obj))
	{
		act("$E �� ����� ����� ���� � ���� �����.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	act("�� ���� $o3 $N2.", FALSE, ch, obj, vict, TO_CHAR);
	act("$n ���$g ��� $o3.", FALSE, ch, obj, vict, TO_VICT);
	act("$n ���$g $o3 $N2.", TRUE, ch, obj, vict, TO_NOTVICT | TO_ARENA_LISTEN);

	if (!world_objects.get_by_raw_ptr(obj))
	{
		return;	// object has been removed from world during script execution.
	}

	obj_from_char(obj);
	obj_to_char(obj, vict);

	// �������� ��������-����� � ���������
	get_check_money(vict, obj, 0);

	if (!IS_NPC(ch) && !IS_NPC(vict))
	{
		ObjSaveSync::add(ch->get_uid(), vict->get_uid(), ObjSaveSync::CHAR_SAVE);
	}
}

// utility function for give
CHAR_DATA *give_find_vict(CHAR_DATA * ch, char *arg)
{
	CHAR_DATA *vict;

	if (!*arg)
	{
		send_to_char("����?\r\n", ch);
		return (NULL);
	}
	else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
	{
		send_to_char(NOPERSON, ch);
		return (NULL);
	}
	else if (vict == ch)
	{
		send_to_char("�� ���������� ��� �� ������ ������� � ������.\r\n", ch);
		return (NULL);
	}
	else
		return (vict);
}


void perform_give_gold(CHAR_DATA * ch, CHAR_DATA * vict, int amount)
{
	if (amount <= 0)
	{
		send_to_char("��-��-�� (3 ����)...\r\n", ch);
		return;
	}
	if (ch->get_gold() < amount && (IS_NPC(ch) || !IS_IMPL(ch)))
	{
		send_to_char("� ������ �� �� ����� �����������?\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ROOM_NOITEM) && !IS_GOD(ch))
	{
		act("��������� ���� �������� ��� ������� ���!", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	send_to_char(OK, ch);
	sprintf(buf, "$n ���$g ��� %d %s.", amount, desc_count(amount, WHAT_MONEYu));
	act(buf, FALSE, ch, 0, vict, TO_VICT);
	sprintf(buf, "$n ���$g %s $N2.", money_desc(amount, 3));
	act(buf, TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
	if (!(IS_NPC(ch) || IS_NPC(vict)))
	{
		sprintf(buf, "<%s> {%d} ������� %d ��� ��� ������ ������� c %s.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), amount, GET_PAD(vict, 4));
		mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
	}
	if (IS_NPC(ch) || !IS_IMPL(ch))
	{
		ch->remove_gold(amount);
	}
	// ���� ����� ���� ��� - ������� ����-�����
	if (IS_NPC(ch) && !IS_CHARMICE(ch))
	{
		vict->add_gold(amount);
		split_or_clan_tax(vict, amount);
	}
	else
	{
		vict->add_gold(amount);
	}
	bribe_mtrigger(vict, ch, amount);
}

void do_give(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int amount, dotmode;
	CHAR_DATA *vict;
	OBJ_DATA *obj, *next_obj;

	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("���� ��� � ����?\r\n", ch);
	else if (is_number(arg))
	{
		amount = atoi(arg);
		argument = one_argument(argument, arg);
		if (!strn_cmp("coin", arg, 4) || !strn_cmp("���", arg, 5) || !str_cmp("�����", arg))
		{
			one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)) != NULL)
				perform_give_gold(ch, vict, amount);
			return;
		}
		else if (!*arg)  	// Give multiple code.
		{
			sprintf(buf, "���� %d �� ������ ����?\r\n", amount);
			send_to_char(buf, ch);
		}
		else if (!(vict = give_find_vict(ch, argument)))
		{
			return;
		}
		else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
		{
			sprintf(buf, "� ��� ��� '%s'.\r\n", arg);
			send_to_char(buf, ch);
		}
		else
		{
			while (obj && amount--)
			{
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				perform_give(ch, vict, obj);
				obj = next_obj;
			}
		}
	}
	else
	{
		one_argument(argument, buf1);
		if (!(vict = give_find_vict(ch, buf1)))
			return;
		dotmode = find_all_dots(arg);
		if (dotmode == FIND_INDIV)
		{
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
			{
				sprintf(buf, "� ��� ��� '%s'.\r\n", arg);
				send_to_char(buf, ch);
			}
			else
				perform_give(ch, vict, obj);
		}
		else
		{
			if (dotmode == FIND_ALLDOT && !*arg)
			{
				send_to_char("���� \"���\" ������ ���� ���������?\r\n", ch);
				return;
			}
			if (!ch->carrying)
				send_to_char("� ��� ���� ������ ���.\r\n", ch);
			else
			{
				bool has_items = false;
				for (obj = ch->carrying; obj; obj = next_obj)
				{
					next_obj = obj->get_next_content();
					if (CAN_SEE_OBJ(ch, obj)
						&& (dotmode == FIND_ALL
							|| isname(arg, obj->get_aliases())
							|| CHECK_CUSTOM_LABEL(arg, obj, ch)))
					{
						perform_give(ch, vict, obj);
						has_items = true;
					}
				}
				if (!has_items)
				{
					send_to_char(ch, "� ��� ��� '%s'.\r\n", arg);
				}
			}
		}
	}
}



void weight_change_object(OBJ_DATA * obj, int weight)
{
	OBJ_DATA *tmp_obj;
	CHAR_DATA *tmp_ch;

	if (obj->get_in_room() != NOWHERE)
	{
		obj->set_weight(MAX(1, GET_OBJ_WEIGHT(obj) + weight));
	}
	else if ((tmp_ch = obj->get_carried_by()))
	{
		obj_from_char(obj);
		obj->set_weight(MAX(1, GET_OBJ_WEIGHT(obj) + weight));
		obj_to_char(obj, tmp_ch);
	}
	else if ((tmp_obj = obj->get_in_obj()))
	{
		obj_from_obj(obj);
		obj->set_weight(MAX(1, GET_OBJ_WEIGHT(obj) + weight));
		obj_to_obj(obj, tmp_obj);
	}
	else
	{
		log("SYSERR: Unknown attempt to subtract weight from an object.");
	}
}

void do_fry(CHAR_DATA *ch, char *argument, int/* cmd*/, int /*subcmd*/)
{
	OBJ_DATA *meet;
	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("��� �� ��������� ���������?\r\n", ch);
		return;
	}
	if (ch->get_fighting())
	{
		send_to_char("�� ����� ����������� � ���.\r\n", ch);
		return;
	}
	if (!(meet = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		sprintf(buf, "� ��� ��� '%s'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	}
	if (!world[ch->in_room]->fires)
	{
	        send_to_char(ch, "�� ��� �� ��������� ������, ���� �� ���!\r\n");
		return;
	}
	if (world[ch->in_room]->fires > 2)
	{
	        send_to_char(ch, "������ ������� �����, ������!\r\n");
		return;
	}

	const auto meet_vnum = GET_OBJ_VNUM(meet);
	if (!meat_mapping.has(meet_vnum)) // �� ������� � �������
	{
		send_to_char(ch, "%s �� �������� ��� �����.\r\n", GET_OBJ_PNAME(meet,0).c_str());
		return;
	}

	act("�� �������� �� ������� � ��������� $o3.", FALSE, ch, meet, 0, TO_CHAR);
	act("$n �������$g �� ������� � ��������$g $o3.", TRUE, ch, meet, 0, TO_ROOM | TO_ARENA_LISTEN);
	const auto tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.get(meet_vnum));
	if (tobj)
	{
		can_carry_obj(ch, tobj.get());
		extract_obj(meet);
		WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
	}
	else	
	{
		mudlog("�� �������� ��������� �������� ���� � act.item.cpp::do_fry!", NRM, LVL_GRGOD, ERRLOG, TRUE);
	}
}

void do_eat(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	OBJ_DATA *food;
	int amount;

	one_argument(argument, arg);


	if (subcmd == SCMD_DEVOUR)
	{
		if (MOB_FLAGGED(ch, MOB_RESURRECTED)
			&& can_use_feat(ch->get_master(), ZOMBIE_DROVER_FEAT))
		{
			feed_charmice(ch, arg);
			return;
		}
	}
	if (!IS_NPC(ch)
		&& subcmd == SCMD_DEVOUR)
	{
		send_to_char("�� �� �� ����� �����, �������� �����!\r\n", ch);
		return;
	}

	if (IS_NPC(ch))		// Cannot use GET_COND() on mobs.
		return;

	if (!*arg)
	{
		send_to_char("��� �� ��������� ��������?\r\n", ch);
		return;
	}
	if (ch->get_fighting())
	{
		send_to_char("�� ����� ����������� � ���.\r\n", ch);
		return;
	}

	if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		sprintf(buf, "� ��� ��� '%s'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	}
	if (subcmd == SCMD_TASTE
		&& ((GET_OBJ_TYPE(food) == OBJ_DATA::ITEM_DRINKCON)
			|| (GET_OBJ_TYPE(food) == OBJ_DATA::ITEM_FOUNTAIN)))
	{
		do_drink(ch, argument, 0, SCMD_SIP);
		return;
	}

	if (!IS_GOD(ch))
	{
		if (GET_OBJ_TYPE(food) == OBJ_DATA::ITEM_MING) //��������� �� ������ ������� ���������� �����
		{
			send_to_char("�� ������ ����������� - ������� �������!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(food) != OBJ_DATA::ITEM_FOOD
			&& GET_OBJ_TYPE(food) != OBJ_DATA::ITEM_NOTE)
		{
			send_to_char("��� ����������!\r\n", ch);
			return;
		}
	}
	if (GET_COND(ch, FULL) == 0
		&& GET_OBJ_TYPE(food) != OBJ_DATA::ITEM_NOTE)  	// Stomach full
	{
		send_to_char("�� ������� ���� ��� �����!\r\n", ch);
		return;
	}
	if (subcmd == SCMD_EAT
		|| (subcmd == SCMD_TASTE
			&& GET_OBJ_TYPE(food) == OBJ_DATA::ITEM_NOTE))
	{
		act("�� ����� $o3.", FALSE, ch, food, 0, TO_CHAR);
		act("$n ����$g $o3.", TRUE, ch, food, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
	else
	{
		act("�� �������� ��������� ������� �� $o1.", FALSE, ch, food, 0, TO_CHAR);
		act("$n ����������$g $o3 �� ����.", TRUE, ch, food, 0, TO_ROOM | TO_ARENA_LISTEN);
	}

	amount = ((subcmd == SCMD_EAT && GET_OBJ_TYPE(food) != OBJ_DATA::ITEM_NOTE)
		? GET_OBJ_VAL(food, 0)
		: 1);

	gain_condition(ch, FULL, -2*amount);

	if (GET_COND(ch, FULL)==0)
	{
		send_to_char("�� �������.\r\n", ch);
	}

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (food->get_affected(i).modifier)
		{
			AFFECT_DATA<EApplyLocation> af;
			af.location = food->get_affected(i).location;
			af.modifier = food->get_affected(i).modifier;
			af.bitvector = 0;
			af.type = 103;
//			af.battleflag = 0;
			af.duration = pc_duration(ch, 10 * 2, 0, 0, 0, 0);
			affect_join_fspell(ch, af);
		}

	}

	if ((GET_OBJ_VAL(food, 3) == 1) && !IS_IMMORTAL(ch))  	// The shit was poisoned !
	{
		send_to_char("������, ����� �������� ����!\r\n", ch);
		act("$n ��������$u � �����$g �������������.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

		AFFECT_DATA<EApplyLocation> af;
		af.type = SPELL_POISON;
		af.duration = pc_duration(ch, amount == 1 ? amount : amount * 2, 0, 0, 0, 0);
		af.modifier = 0;
		af.location = APPLY_STR;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = AF_SAME_TIME;
		affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
		af.type = SPELL_POISON;
		af.duration = pc_duration(ch, amount == 1 ? amount : amount * 2, 0, 0, 0, 0);
		af.modifier = amount * 3;
		af.location = APPLY_POISON;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = AF_SAME_TIME;
		affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
		ch->Poisoner = 0;
	}
	if (subcmd == SCMD_EAT
		|| (subcmd == SCMD_TASTE
			&& GET_OBJ_TYPE(food) == OBJ_DATA::ITEM_NOTE))
	{
		extract_obj(food);
	}
	else
	{
		food->set_val(0, food->get_val(0) - 1);
		if (!food->get_val(0))
		{
			send_to_char("�� ����� ���!\r\n", ch);
			extract_obj(food);
		}
	}
}

void perform_wear(CHAR_DATA * ch, OBJ_DATA * obj, int where)
{
	/*
	 * ITEM_WEAR_TAKE is used for objects that do not require special bits
	 * to be put into that position (e.g. you can hold any object, not just
	 * an object with a HOLD bit.)
	 */

	const EWearFlag wear_bitvectors[] =
	{
		EWearFlag::ITEM_WEAR_TAKE,
		EWearFlag::ITEM_WEAR_FINGER,
		EWearFlag::ITEM_WEAR_FINGER,
		EWearFlag::ITEM_WEAR_NECK,
		EWearFlag::ITEM_WEAR_NECK,
		EWearFlag::ITEM_WEAR_BODY,
		EWearFlag::ITEM_WEAR_HEAD,
		EWearFlag::ITEM_WEAR_LEGS,
		EWearFlag::ITEM_WEAR_FEET,
		EWearFlag::ITEM_WEAR_HANDS,
		EWearFlag::ITEM_WEAR_ARMS,
		EWearFlag::ITEM_WEAR_SHIELD,
		EWearFlag::ITEM_WEAR_ABOUT,
		EWearFlag::ITEM_WEAR_WAIST,
		EWearFlag::ITEM_WEAR_WRIST,
		EWearFlag::ITEM_WEAR_WRIST,
		EWearFlag::ITEM_WEAR_WIELD,
		EWearFlag::ITEM_WEAR_TAKE,
		EWearFlag::ITEM_WEAR_BOTHS,
		EWearFlag::ITEM_WEAR_QUIVER
	};

	const std::array<const char *, sizeof(wear_bitvectors)> already_wearing =
	{
		"�� ��� ����������� ����.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"� ��� ��� ���-�� ������ �� �������.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"� ��� ��� ���-�� ������ �� ���.\r\n",
		"� ��� ��� ���-�� ������ �� ��������.\r\n",
		"� ��� ��� ���-�� ������ �� ������.\r\n",
		"� ��� ��� ���-�� ������ �� ����.\r\n",
		"� ��� ��� ���-�� ������ �� ������.\r\n",
		"� ��� ��� ���-�� ������ �� �����.\r\n",
		"� ��� ��� ���-�� ������ �� ����.\r\n",
		"�� ��� ����������� ���.\r\n",
		"�� ��� �������� �� ���-��.\r\n",
		"� ��� ��� ���-�� ������ �� ����.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"� ��� ��� ���-�� ������ �� ��������.\r\n",
		"�� ��� ���-�� ������� � ������ ����.\r\n",
		"�� ��� ���-�� ������� � ����� ����.\r\n",
		"�� ��� ������� ������ � ����� �����.\r\n",
		"�� ��� ����������� ������.\r\n"
	};

	// first, make sure that the wear position is valid.
	if (!CAN_WEAR(obj, wear_bitvectors[where]))
	{
		act("�� �� ������ ������ $o3 �� ��� ����� ����.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (unique_stuff(ch, obj) && OBJ_FLAGGED(obj, EExtraFlag::ITEM_UNIQUE))
	{
		send_to_char("�� �� ������ ������������ ����� ����� ����� ����.\r\n", ch);
		return;
	}
    
	// for neck, finger, and wrist, try pos 2 if pos 1 is already full
	if (   // �� ����� ������� ���� ���� ���� ��� ���������
		(where == WEAR_HOLD && (GET_EQ(ch, WEAR_BOTHS) || GET_EQ(ch, WEAR_LIGHT)
								|| GET_EQ(ch, WEAR_SHIELD))) ||
		// �� ����� ����������� ���� ���� ���������
		(where == WEAR_WIELD && GET_EQ(ch, WEAR_BOTHS)) ||
		// �� ����� ������� ��� ���� ���-�� ������ ��� ���������
		(where == WEAR_SHIELD && (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_BOTHS))) ||
		// �� ����� ��������� ���� ���� ���, ����, �������� ��� ������
		(where == WEAR_BOTHS && (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_LIGHT)
								 || GET_EQ(ch, WEAR_SHIELD) || GET_EQ(ch, WEAR_WIELD))) ||
		// �� ����� ������� ���� ���� ��������� ��� ������
		(where == WEAR_LIGHT && (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_BOTHS))))
	{
		send_to_char("� ��� ������ ����.\r\n", ch);
		return;
	}
	if (   // �� ����� ����� ������ ���� ���� �� ���
		(where == WEAR_QUIVER && 
                !(GET_EQ(ch, WEAR_BOTHS) &&
                (((GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS))) == OBJ_DATA::ITEM_WEAPON) 
                    && (GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS )))))
        {
		send_to_char("� �������� ��� ������?\r\n", ch);
		return;
        }
	// ������ ������ ���, ���� ������������ ����
	if (!IS_IMMORTAL(ch) && (where == WEAR_SHIELD) && !OK_SHIELD(ch, obj))
	{
	}

	if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R))
		if (GET_EQ(ch, where))
			where++;

	if (GET_EQ(ch, where))
	{
		send_to_char(already_wearing[where], ch);
		return;
	}
	if (!wear_otrigger(obj, ch, where))
		return;

	//obj_from_char(obj);
	equip_char(ch, obj, where | 0x100);
}

int find_eq_pos(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
	int where = -1;

	// \r to prevent explicit wearing. Don't use \n, it's end-of-array marker.
	const char *keywords[] =
	{
		"\r!RESERVED!",
		"�����������",
		"����������",
		"���",
		"�����",
		"����",
		"������",
		"����",
		"������",
		"�����",
		"����",
		"���",
		"�����",
		"����",
		"��������",
		"\r!RESERVED!",
		"\r!RESERVED!",
		"\r!RESERVED!",
		"\n"
	};

	if (!arg || !*arg)
	{
		int tmp_where = -1;
		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER))
		{
			if (!GET_EQ(ch, WEAR_FINGER_R))
			{
				where = WEAR_FINGER_R;
			}
			else if (!GET_EQ(ch, WEAR_FINGER_L))
			{
				where = WEAR_FINGER_L;
			}
			else
			{
				tmp_where = WEAR_FINGER_R;
			}
		}
		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK))
		{
			if (!GET_EQ(ch, WEAR_NECK_1))
			{
				where = WEAR_NECK_1;
			}
			else if (!GET_EQ(ch, WEAR_NECK_2))
			{
				where = WEAR_NECK_2;
			}
			else
			{
				tmp_where = WEAR_NECK_1;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY))
		{
			if (!GET_EQ(ch, WEAR_BODY))
			{
				where = WEAR_BODY;
			}
			else
			{
				tmp_where = WEAR_BODY;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD))
		{
			if (!GET_EQ(ch, WEAR_HEAD))
			{
				where = WEAR_HEAD;
			}
			else
			{
				tmp_where = WEAR_HEAD;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS))
		{
			if (!GET_EQ(ch, WEAR_LEGS))
			{
				where = WEAR_LEGS;
			}
			else
			{
				tmp_where = WEAR_LEGS;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET))
		{
			if (!GET_EQ(ch, WEAR_FEET))
			{
				where = WEAR_FEET;
			}
			else
			{
				tmp_where = WEAR_FEET;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))
		{
			if (!GET_EQ(ch, WEAR_HANDS))
			{
				where = WEAR_HANDS;
			}
			else
			{
				tmp_where = WEAR_HANDS;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS))
		{
			if (!GET_EQ(ch, WEAR_ARMS))
			{
				where = WEAR_ARMS;
			}
			else
			{
				tmp_where = WEAR_ARMS;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
		{
			if (!GET_EQ(ch, WEAR_SHIELD))
			{
				where = WEAR_SHIELD;
			}
			else
			{
				tmp_where = WEAR_SHIELD;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))
		{
			if (!GET_EQ(ch, WEAR_ABOUT))
			{
				where = WEAR_ABOUT;
			}
			else
			{
				tmp_where = WEAR_ABOUT;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST))
		{
			if (!GET_EQ(ch, WEAR_WAIST))
			{
				where = WEAR_WAIST;
			}
			else
			{
				tmp_where = WEAR_WAIST;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_QUIVER))
		{
			if (!GET_EQ(ch, WEAR_QUIVER))
			{
				where = WEAR_QUIVER;
			}
			else
			{
				tmp_where = WEAR_QUIVER;
			}
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST))
		{
			if (!GET_EQ(ch, WEAR_WRIST_R))
			{
				where = WEAR_WRIST_R;
			}
			else if (!GET_EQ(ch, WEAR_WRIST_L))
			{
				where = WEAR_WRIST_L;
			}
			else
			{
				tmp_where = WEAR_WRIST_R;
			}
		}

		if (where == -1)
		{
			where = tmp_where;
		}
	}
	else
	{
		where = search_block(arg, keywords, FALSE);
		if (where < 0
			|| *arg == '!')
		{
			sprintf(buf, "'%s'? �������� �������� � ���� �������!\r\n", arg);
			send_to_char(buf, ch);
			return -1;
		}
	}

	return where;
}

void do_wear(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *obj, *next_obj;
	int where, dotmode, items_worn = 0;

	two_arguments(argument, arg1, arg2);

	if (IS_NPC(ch)
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& (!NPC_FLAGGED(ch, NPC_ARMORING)
			|| MOB_FLAGGED(ch, MOB_RESURRECTED)))
	{
		return;
	}

	if (!*arg1)
	{
		send_to_char("��� �� ��������� ������?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg1);

	if (*arg2 && (dotmode != FIND_INDIV))
	{
		send_to_char("� �� ����� ����� ���� �� ������� ��� ������?!\r\n", ch);
		return;
	}
	if (dotmode == FIND_ALL)
	{
		for (obj = ch->carrying; obj && !GET_MOB_HOLD(ch) && GET_POS(ch) > POS_SLEEPING; obj = next_obj)
		{
			next_obj = obj->get_next_content();
			if (CAN_SEE_OBJ(ch, obj)
					&& (where = find_eq_pos(ch, obj, 0)) >= 0)
			{
				items_worn++;
				perform_wear(ch, obj, where);
			}
		}
		if (!items_worn)
		{
			send_to_char("���, �� ������ ��� ������.\r\n", ch);
		}
	}
	else if (dotmode == FIND_ALLDOT)
	{
		if (!*arg1)
		{
			send_to_char("������ \"���\" ����?\r\n", ch);
			return;
		}
		if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		{
			sprintf(buf, "� ��� ��� ������ �������� �� '%s'.\r\n", arg1);
			send_to_char(buf, ch);
		}
		else
			while (obj && !GET_MOB_HOLD(ch) && GET_POS(ch) > POS_SLEEPING)
			{
				next_obj = get_obj_in_list_vis(ch, arg1, obj->get_next_content());
				if ((where = find_eq_pos(ch, obj, 0)) >= 0)
				{
					perform_wear(ch, obj, where);
				}
				else
				{
					act("�� �� ������ ������ $o3.", FALSE, ch, obj, 0, TO_CHAR);
				}
				obj = next_obj;
			}
	}
	else
	{
		if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		{
			sprintf(buf, "� ��� ��� ������ �������� �� '%s'.\r\n", arg1);
			send_to_char(buf, ch);
		}
		else
		{
			if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
				perform_wear(ch, obj, where);
			else if (!*arg2)
				act("�� �� ������ ������ $o3.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
}

void do_wield(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	OBJ_DATA *obj;
	int wear;

	if (IS_NPC(ch) && (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)&&(!NPC_FLAGGED(ch, NPC_WIELDING) || MOB_FLAGGED(ch, MOB_RESURRECTED))))
		return;

	if (ch->is_morphed())
	{
		send_to_char("������ �������� ������� ������.\r\n", ch);
		return;
	}
	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("����������� ���?\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		sprintf(buf, "�� �� ������ ������ �������� �� \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
	}
	else
	{
		if (!CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD)
			&& !CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
		{
			send_to_char("�� �� ������ ����������� ����.\r\n", ch);
		}
		else if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_WEAPON)
		{
			send_to_char("��� �� ������.\r\n", ch);
		}
		else if (IS_NPC(ch)
			&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& MOB_FLAGGED(ch, MOB_CORPSE))
		{
			send_to_char("������� ����� �� ����� �����������.\r\n", ch);
		}
		else
		{
			one_argument(argument, arg);
			if (!str_cmp(arg, "���")
				&& CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
			{
				// ������ ������ ����
				if (!IS_IMMORTAL(ch) && !OK_BOTH(ch, obj))
				{
					act("��� ������� ������ ������� $o3 ����� ������.", FALSE, ch, obj, 0, TO_CHAR);
					message_str_need(ch, obj, STR_BOTH_W);
					return;
				};
				perform_wear(ch, obj, WEAR_BOTHS);
				return;
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
			{
				wear = WEAR_WIELD;
			}
			else
			{
				wear = WEAR_BOTHS;
			}

			if (wear == WEAR_WIELD && !IS_IMMORTAL(ch) && !OK_WIELD(ch, obj))
			{
				act("��� ������� ������ ������� $o3 � ������ ����.", FALSE, ch, obj, 0, TO_CHAR);
				message_str_need(ch, obj, STR_WIELD_W);

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
				{
					wear = WEAR_BOTHS;
				}
				else
				{
					return;
				}
			}

			if (wear == WEAR_BOTHS && !IS_IMMORTAL(ch) && !OK_BOTH(ch, obj))
			{
				act("��� ������� ������ ������� $o3 ����� ������.", FALSE, ch, obj, 0, TO_CHAR);
				message_str_need(ch, obj, STR_BOTH_W);
				return;
			};
			perform_wear(ch, obj, wear);
		}
	}
}

void do_grab(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int where = WEAR_HOLD;
	OBJ_DATA *obj;
	one_argument(argument, arg);

	if (IS_NPC(ch) && !NPC_FLAGGED(ch, NPC_WIELDING))
		return;

	if (ch->is_morphed())
	{
		send_to_char("������ �������� ��� �������.\r\n", ch);
		return;
	}

	if (!*arg)
		send_to_char("�� ������� : '����� ���!!! ������ ���!!!'\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		sprintf(buf, "� ��� ��� ������ �������� �� '%s'.\r\n", arg);
		send_to_char(buf, ch);
	}
	else
	{
		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_LIGHT)
		{
			perform_wear(ch, obj, WEAR_LIGHT);
		}
		else
		{
			if (!CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD)
				&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_WAND
				&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_STAFF
				&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_SCROLL
				&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_POTION)
			{
				send_to_char("�� �� ������ ��� �������.\r\n", ch);
				return;
			}

			if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
			{
				if (GET_OBJ_SKILL(obj) == SKILL_BOTHHANDS
					|| GET_OBJ_SKILL(obj) == SKILL_BOWS)
				{
					send_to_char("������ ��� ������ ������� ����������.", ch);
					return;
				}
			}

			if (IS_NPC(ch)
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
				&& MOB_FLAGGED(ch, MOB_CORPSE))
			{
				send_to_char("������� ����� �� ����� �����������.\r\n", ch);
				return;
			}
			if (!IS_IMMORTAL(ch)
				&& !OK_HELD(ch, obj))
			{
				act("��� ������� ������ ������� $o3 � ����� ����.", FALSE, ch, obj, 0, TO_CHAR);
				message_str_need(ch, obj, STR_HOLD_W);

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
				{
					if (!OK_BOTH(ch, obj))
					{
						act("��� ������� ������ ������� $o3 ����� ������.", FALSE, ch, obj, 0, TO_CHAR);
						message_str_need(ch, obj, STR_BOTH_W);
						return;
					}
					else
					{
						where = WEAR_BOTHS;
					}
				}
				else
				{
					return;
				}
			}
			perform_wear(ch, obj, where);
		}
	}
}



void perform_remove(CHAR_DATA * ch, int pos)
{
	OBJ_DATA *obj;

	if (!(obj = GET_EQ(ch, pos)))
	{
		log("SYSERR: perform_remove: bad pos %d passed.", pos);
	}
	else
	{
		/*
		   if (IS_OBJ_STAT(obj, ITEM_NODROP))
		   act("�� �� ������ ����� $o3!", FALSE, ch, obj, 0, TO_CHAR);
		   else
		 */
		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		{
			act("$p: �� �� ������ ����� ������� �����!", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			if (!remove_otrigger(obj, ch))
			{
				return;
			}

			act("�� ���������� ������������ $o3.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n ���������$g ������������ $o3.", TRUE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
			obj_to_char(unequip_char(ch, pos | 0x40), ch);
		}
	}
}

void do_remove(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int i, dotmode, found;
	OBJ_DATA *obj;

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("����� ���?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg);

	if (dotmode == FIND_ALL)
	{
		found = 0;
		for (i = 0; i < NUM_WEARS; i++)
		{
			if (GET_EQ(ch, i))
			{
				perform_remove(ch, i);
				found = 1;
			}
		}
		if (!found)
		{
			send_to_char("�� ��� �� ������ ��������� ����� ����.\r\n", ch);
        		return;
		}
	}
	else if (dotmode == FIND_ALLDOT)
	{
		if (!*arg)
		{
			send_to_char("����� ��� ���� ������ ����?\r\n", ch);
        		return;
		}
		else
		{
			found = 0;
			for (i = 0; i < NUM_WEARS; i++)
			{
				if (GET_EQ(ch, i)
					&& CAN_SEE_OBJ(ch, GET_EQ(ch, i))
					&& (isname(arg, GET_EQ(ch, i)->get_aliases())
						|| CHECK_CUSTOM_LABEL(arg, GET_EQ(ch, i), ch)))
				{
					perform_remove(ch, i);
					found = 1;
				}
			}
			if (!found)
			{
				sprintf(buf, "�� �� ����������� �� ������ '%s'.\r\n", arg);
				send_to_char(buf, ch);
                                return;
			}
		}
	}
	else  		// Returns object pointer but we don't need it, just true/false.
	{
		if (!get_object_in_equip_vis(ch, arg, ch->equipment, &i))
		{
			// ���� ������� �� ������, �� �������� ����� ���� "�����" ��� "������"
			if (!str_cmp("������", arg))
			{
				if (!GET_EQ(ch, WEAR_WIELD))
				{
					send_to_char("� ������ ���� ������ ���.\r\n", ch);
				}
				else
				{
					perform_remove(ch, WEAR_WIELD);
				}
			}
			else if (!str_cmp("�����", arg))
			{
				if (!GET_EQ(ch, WEAR_HOLD))
					send_to_char("� ����� ���� ������ ���.\r\n", ch);
				else
					perform_remove(ch, WEAR_HOLD);
			}
			else
			{
				sprintf(buf, "�� �� ����������� '%s'.\r\n", arg);
				send_to_char(buf, ch);
                                return;
			}
		}
		else
			perform_remove(ch, i);
	}
        //�� ���-�� �� �������. ������ ������� � ��� ����
        if ((obj = GET_EQ(ch, WEAR_QUIVER)) && !GET_EQ(ch, WEAR_BOTHS))
                {
                    send_to_char("���� ����, ��� � �����.\r\n", ch);
                    act("$n ���������$g ������������ $o3.", FALSE, ch, obj, 0, TO_ROOM);
                    obj_to_char(unequip_char(ch, WEAR_QUIVER), ch);
                    return;
                }
}

void do_upgrade(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	OBJ_DATA *obj;
	int weight, add_hr, add_dr, prob, percent, min_mod, max_mod, i;
	bool oldstate;
	if (!ch->get_skill(SKILL_UPGRADE))
	{
		send_to_char("�� �� ������ �����.", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("��� �� ������ ��������?\r\n", ch);
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		sprintf(buf, "� ��� ��� \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	};

	if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_WEAPON)
	{
		send_to_char("�� ������ �������� ������ ������.\r\n", ch);
		return;
	}

	if (GET_OBJ_SKILL(obj) == SKILL_BOWS)
	{
		send_to_char("���������� �������� ���� ��� ������.\r\n", ch);
		return;
	}

	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_MAGIC))
	{
		send_to_char("�� �� ������ �������� ������������� �������.\r\n", ch);
		return;
	}

	// Make sure no other (than hitroll & damroll) affections.
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if ((obj->get_affected(i).location != APPLY_NONE)
			&& (obj->get_affected(i).location != APPLY_HITROLL)
			&& (obj->get_affected(i).location != APPLY_DAMROLL))
		{
			send_to_char("���� ������� �� ����� ���� �������.\r\n", ch);
			return;
		}
	}

	switch (obj->get_material())
	{
	case OBJ_DATA::MAT_BRONZE:
	case OBJ_DATA::MAT_BULAT:
	case OBJ_DATA::MAT_IRON:
	case OBJ_DATA::MAT_STEEL:
	case OBJ_DATA::MAT_SWORDSSTEEL:
	case OBJ_DATA::MAT_COLOR:
	case OBJ_DATA::MAT_BONE:
		act("�� ������� ������ $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n ����$u ������ $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		weight = -1;
		break;

	case OBJ_DATA::MAT_WOOD:
	case OBJ_DATA::MAT_SUPERWOOD:
		act("�� ������� �������� $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n ����$u �������� $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		weight = -1;
		break;

	case OBJ_DATA::MAT_SKIN:
		act("�� ������� ������������ $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n ����$u ������������ $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		weight = + 1;
		break;

	default:
		sprintf(buf, "� ���������, %s ������ �� ������������� ���������.\r\n", OBJN(obj, ch, 0));
		send_to_char(buf, ch);
		return;
	}
	bool change_weight = true;
	//�������� �������� �����, �� ��� ��������� ������ ������ �� 16%
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SHARPEN))
	{
		const int timer = obj->get_timer() - MAX(1000, obj->get_timer() / 6); // ����, ������ ������ 6 ���������� 0 ����������� �������� ������
		obj->set_timer(timer);
		change_weight = false;
	}
	else
	{
		obj->set_extra_flag(EExtraFlag::ITEM_SHARPEN);
		obj->set_extra_flag(EExtraFlag::ITEM_TRANSFORMED); // ���������� ������ ������������� �����
	}

	percent = number(1, skill_info[SKILL_UPGRADE].max_percent);
	prob = train_skill(ch, SKILL_UPGRADE, skill_info[SKILL_UPGRADE].max_percent, 0);
	if (obj->get_timer() == 0) // �� ���� ���������� �� ����
	{
		act("$o �� ��������$G ������������� � ��������$U � ������ ����...", FALSE, ch, obj, 0, TO_CHAR);
		extract_obj(obj);
		return;
	}
	//��� 200% ������� ������ ����� �������� �� 4-5 �������� � 4-5 ��������
	min_mod = ch->get_trained_skill(SKILL_UPGRADE) / 50;
	//� ������� ��� ������� ������� ��������� ��� ����. �������
	max_mod = MAX(1,MIN(5,(GET_LEVEL(ch) + 5 + GET_REMORT(ch) / 4) / 6));
	oldstate = check_unlimited_timer(obj); // �������� ����� ������ ���� �� �������
	if (IS_IMMORTAL(ch))
	{
		add_dr = add_hr = 10;
	}
	else
	{
		add_dr = add_hr = (max_mod <= min_mod) ? min_mod : number(min_mod, max_mod);
	}
	if (percent > prob || GET_GOD_FLAG(ch, GF_GODSCURSE))
	{
		act("�� ������ �������� $S.", FALSE, ch, obj, 0, TO_CHAR);
		add_hr = -add_hr;
		add_dr = -add_dr;
	}
	else
	{
		act("� ����� �� ������� � ����� ����������.", FALSE, ch, obj, 0, TO_CHAR);
	}

	obj->set_affected(0, APPLY_HITROLL, add_hr);
	obj->set_affected(1, APPLY_DAMROLL, add_dr);

	// ���� ������ ��������� ���� ��������� ������ ������ �� ���������
	if (oldstate && !check_unlimited_timer(obj))
	{
		obj->set_timer(obj_proto.at(GET_OBJ_RNUM(obj))->get_timer());
	}
	//��� �������� ������ ���� ������ ��� �� ���� ��������
	//����� ��� �� �������� ���� �� ��� ������� � ������ ���������
	const auto curent_weight = obj->get_weight();
	if (change_weight && !(curent_weight == 0 && weight < 0))
	{
		obj->set_weight(curent_weight + weight);
		IS_CARRYING_W(ch) += weight;
	}
}

void do_armored(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	OBJ_DATA *obj;
	int add_ac, add_armor, prob, percent, i, k_mul = 1, k_div = 1;

	if (!ch->get_skill(SKILL_ARMORED))
	{
		send_to_char("�� �� ������ �����.", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg)
		send_to_char("��� �� ������ ��������?\r\n", ch);

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		sprintf(buf, "� ��� ��� \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	};

	if (!ObjSystem::is_armor_type(obj))
	{
		send_to_char("�� ������ �������� ������ ������.\r\n", ch);
		return;
	}

	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_MAGIC)
		|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_ARMORED))
	{
		send_to_char("�� �� ������ �������� ���� �������.\r\n", ch);
		return;
	}

	// Make sure no other affections.
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (obj->get_affected(i).location != APPLY_NONE)
		{
			send_to_char("���� ������� �� ����� ���� ��������.\r\n", ch);
			return;
		}

	if (OBJWEAR_FLAGGED(obj, (to_underlying(EWearFlag::ITEM_WEAR_BODY)
		| to_underlying(EWearFlag::ITEM_WEAR_ABOUT))))
	{
		k_mul = 1;
		k_div = 1;
	}
	else if (OBJWEAR_FLAGGED(obj, (to_underlying(EWearFlag::ITEM_WEAR_SHIELD)
		| to_underlying(EWearFlag::ITEM_WEAR_HEAD)
		| to_underlying(EWearFlag::ITEM_WEAR_ARMS)
		| to_underlying(EWearFlag::ITEM_WEAR_LEGS))))
	{
		k_mul = 2;
		k_div = 3;
	}
	else if (OBJWEAR_FLAGGED(obj, (to_underlying(EWearFlag::ITEM_WEAR_HANDS)
		| to_underlying(EWearFlag::ITEM_WEAR_FEET))))
	{
		k_mul = 1;
		k_div = 2;
	}
	else
	{
		act("$o3 ���������� ��������.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	switch (obj->get_material())
	{
	case OBJ_DATA::MAT_IRON:
	case OBJ_DATA::MAT_STEEL:
		act("�� ��������� �������� $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n ������$u �������� $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		break;

	case OBJ_DATA::MAT_WOOD:
	case OBJ_DATA::MAT_SUPERWOOD:
		act("�� ��������� �������� $o3 �������.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n ������$u �������� $o3 �������.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		break;

	case OBJ_DATA::MAT_SKIN:
		act("�� ��������� ������������ $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n ������$u ������������ $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		break;

	default:
		sprintf(buf, "� ���������, %s ������ �� ������������� ���������.\r\n", OBJN(obj, ch, 0));
		send_to_char(buf, ch);
		return;
	}
	obj->set_extra_flag(EExtraFlag::ITEM_ARMORED);
	obj->set_extra_flag(EExtraFlag::ITEM_TRANSFORMED); // ���������� ������ ������������� �����
	
	percent = number(1, skill_info[SKILL_ARMORED].max_percent);
	prob = train_skill(ch, SKILL_ARMORED, skill_info[SKILL_ARMORED].max_percent, 0);

	add_ac = IS_IMMORTAL(ch) ? -20 : -number(1, (GET_LEVEL(ch) + 4) / 5);
	add_armor = IS_IMMORTAL(ch) ? 5 : number(1, (GET_LEVEL(ch) + 4) / 5);

	if (percent > prob
		|| GET_GOD_FLAG(ch, GF_GODSCURSE))
	{
		act("�� ������ ��������� $S.", FALSE, ch, obj, 0, TO_CHAR);
		add_ac = -add_ac;
		add_armor = -add_armor;
	}
	else
	{
		add_ac = MIN(-1, add_ac * k_mul / k_div);
		add_armor = MAX(1, add_armor * k_mul / k_div);
	};

	obj->set_affected(0, APPLY_AC, add_ac);
	obj->set_affected(1, APPLY_ARMOUR, add_armor);
}

void do_fire(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int percent, prob;
	if (!ch->get_skill(SKILL_FIRE))
	{
		send_to_char("�� �� �� ������ ���.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		send_to_char("������ ��� ����� ��������������.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("�� ������ �� ������!\r\n", ch);
		return;
	}


	if (world[ch->in_room]->fires)
	{
		send_to_char("����� ��� ����� �����.\r\n", ch);
		return;
	}

	if (SECT(ch->in_room) == SECT_INSIDE ||
			SECT(ch->in_room) == SECT_CITY ||
			SECT(ch->in_room) == SECT_WATER_SWIM ||
			SECT(ch->in_room) == SECT_WATER_NOSWIM ||
			SECT(ch->in_room) == SECT_FLYING ||
			SECT(ch->in_room) == SECT_UNDERWATER || SECT(ch->in_room) == SECT_SECRET)
	{
		send_to_char("� ���� ������� ������ ������� ������.\r\n", ch);
		return;
	}

	if (!check_moves(ch, FIRE_MOVES))
		return;

	percent = number(1, skill_info[SKILL_FIRE].max_percent);
	prob = calculate_skill(ch, SKILL_FIRE, 0);
	if (percent > prob)
	{
		send_to_char("�� ���������� ������� ������, �� � ��� ������ �� �����.\r\n", ch);
		return;
	}
	else
	{
		world[ch->in_room]->fires = MAX(0, (prob - percent) / 5) + 1;
		send_to_char("�� ������� �������� � �������� �����.\n\r", ch);
		act("$n ������$g �����.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		improove_skill(ch, SKILL_FIRE, TRUE, 0);
	}
}

void do_extinguish(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *caster;
	int tp, lag = 0;
	const char *targets[] =
	{
		"������",
		"�����",
		"�����",
		"fire",
		"�����",
		"�������",
		"����",
		"label",
		"\n"
	};

	if (IS_NPC(ch))
	{
		return;
	}

	one_argument(argument, arg);

	if ((!*arg) || ((tp = search_block(arg, targets, FALSE)) == -1))
	{
		send_to_char("��� �� ������ ���������?\r\n", ch);
		return;
	}
	tp >>= 2;

	switch (tp)
	{
	case 0:
		if (world[ch->in_room]->fires)
		{
			if (world[ch->in_room]->fires < 5)
				--world[ch->in_room]->fires;
			else
				world[ch->in_room]->fires = 4;
			send_to_char("�� ��������� ������.\r\n", ch);
			act("$n ��������$g ������.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			if (world[ch->in_room]->fires == 0)
			{
				send_to_char("������ �����.\r\n", ch);
				act("������ �����.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			}
			lag = 1;
		}
		else
		{
			send_to_char("� ��� ������� � ������ :)\r\n", ch);
		}
		break;

	case 1:
		const auto& room = world[ch->in_room];
		auto aff_i = room->affected.end();
		auto aff_first = room->affected.end();
		
		//Find own rune label or first run label in room
		for (auto affect_it = room->affected.begin(); affect_it != room->affected.end(); ++affect_it)
		{
			if (affect_it->get()->type == SPELL_RUNE_LABEL)
			{
				if (affect_it->get()->caster_id == GET_ID(ch))
				{
					aff_i = affect_it;
					break;
				}

				if (aff_first == room->affected.end())
				{
					aff_first = affect_it;
				}
			}
		}

		if (aff_i == room->affected.end())
		{
			//Own rune label not found. Use first in room
			aff_i = aff_first;
		}

		if (aff_i != room->affected.end()
			&& (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
				|| IS_IMMORTAL(ch)
				|| PRF_FLAGGED(ch, PRF_CODERINFO)))
		{
			send_to_char("������� ��������� ��� �� �����, �� ������ ���������� �������.\r\n", ch);
			act("$n �������$g ��������� ��� �� ���������� �����, ��������� �� ���������.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

			const auto& aff = *aff_i;
			if (GET_ID(ch) != aff->caster_id) //��� ������� �� ���� ����� - ���, ��������
			{
				//���� ������� �� ����
				caster = find_char(aff->caster_id);
				//���� ������ ������ - ������ ������� �� ��� �� ���������
				if (caster)
				{
					pk_thiefs_action(ch, caster);
					sprintf(buf, "���������� ������� ���� ��������� ������, � ����� ���� ���������� ���������� ����� %s.\r\n", GET_PAD(ch, 1));
					send_to_char(buf, caster);
				}
			}
			affect_room_remove(world[ch->in_room], aff_i);
			lag = 3;
		}
		else
		{
			send_to_char("� ��� ������� � ������ :)\r\n", ch);
		}
		break;
	}

	//�������-�� ��� �� ��� ����.
	if (!WAITLESS(ch))
	{
		WAIT_STATE(ch, lag * PULSE_VIOLENCE);
	}
}

#define MAX_REMOVE  13
const int RemoveSpell[MAX_REMOVE] = { SPELL_SLEEP, SPELL_POISON, SPELL_WEAKNESS, SPELL_CURSE, SPELL_PLAQUE,
			SPELL_SILENCE, SPELL_BLINDNESS, SPELL_HAEMORRAGIA, SPELL_HOLD, SPELL_PEACEFUL, SPELL_CONE_OF_COLD,
			SPELL_DEAFNESS, SPELL_BATTLE };

void do_firstaid(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int percent, prob, success = FALSE, need = FALSE, spellnum = 0;
	struct timed_type timed;

	if (!ch->get_skill(SKILL_AID))
	{
		send_to_char("��� ������� ����� ���������.\r\n", ch);
		return;
	}
	if (!IS_GOD(ch) && timed_by_skill(ch, SKILL_AID))
	{
		send_to_char("��� ����� ������ ������ - ������� �� ���������.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	CHAR_DATA *vict;
	if (!*arg)
	{
		vict = ch;
	}
	else
	{
		vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);
		if (!vict)
		{
			send_to_char("���� �� ������ ���������?\r\n", ch);
			return;
		}
	}

	if (vict->get_fighting())
	{
		act("$N ���������, $M �� �� ����� �������� ���������.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	percent = number(1, skill_info[SKILL_AID].max_percent);
	prob = calculate_skill(ch, SKILL_AID, vict);

	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) || GET_GOD_FLAG(vict, GF_GODSLIKE))
	{
		percent = prob;
	}
	if (GET_GOD_FLAG(ch, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSCURSE))
	{
		prob = 0;
	}
	success = (prob >= percent);
	need = FALSE;

	if ((GET_REAL_MAX_HIT(vict) > 0
		&& (GET_HIT(vict) * 100 / GET_REAL_MAX_HIT(vict)) < 31)
		|| (GET_REAL_MAX_HIT(vict) <= 0
			&& GET_HIT(vict) < GET_REAL_MAX_HIT(vict))
		|| (GET_HIT(vict) < GET_REAL_MAX_HIT(vict)
			&& can_use_feat(ch, HEALER_FEAT)))
	{
		need = TRUE;
		if (success)
		{
			if (!PRF_FLAGGED(ch, PRF_TESTER))
			{
				const int dif = GET_REAL_MAX_HIT(vict) - GET_HIT(vict);
				const int add = MIN(dif, (dif * (prob - percent) / 100) + 1);
				GET_HIT(vict) += add;
			}
			else
			{
				percent = calculate_skill(ch, SKILL_AID, vict);
				prob = GET_LEVEL(ch) * percent * 0.5;
				send_to_char(ch, "&R������� ���� %d �������� %d �����, ����� %d\r\n", GET_LEVEL(vict), prob, percent);
				GET_HIT(vict) += prob;
				GET_HIT(vict) = MIN(GET_HIT(vict), GET_REAL_MAX_HIT(vict));
				update_pos(vict);
			}
		}
	}

	int count = 0;
	if (PRF_FLAGGED(ch, PRF_TESTER))
	{
		count = (GET_SKILL(ch, SKILL_AID) - 20) / 30;
		send_to_char(ch, "������ %d ��������\r\n", count);

		const auto remove_count = vict->remove_random_affects(count);
		send_to_char(ch, "����� %d ��������\r\n", remove_count);

		//
		need = TRUE;
		prob = TRUE;
	}
	else
	{
		count = MIN(MAX_REMOVE, MAX_REMOVE * prob / 100);

		for (percent = 0, prob = need; !need && percent < MAX_REMOVE && RemoveSpell[percent]; percent++)
		{
			if (affected_by_spell(vict, RemoveSpell[percent]))
			{
				need = TRUE;
				if (percent < count)
				{
					spellnum = RemoveSpell[percent];
					prob = TRUE;
				}
			}
		}
	}

	if (!need)
	{
		act("$N � ������� �� ���������.", FALSE, ch, 0, vict, TO_CHAR);
	}
	else if (!prob)
	{
		act("� ��� �� ������ ������ �������� $N3.", FALSE, ch, 0, vict, TO_CHAR);
	}
	else  			//improove_skill(ch, SKILL_AID, TRUE, 0);
	{
		timed.skill = SKILL_AID;
		timed.time = IS_IMMORTAL(ch) ? 2 : IS_PALADINE(ch) ? 4 : IS_CLERIC(ch) ? 2 : 6;
		timed_to_char(ch, &timed);
		if (vict != ch)
		{
			improove_skill(ch, SKILL_AID, success, 0);
			if (success)
			{
				act("�� ������� ������ ������ $N2.", FALSE, ch, 0, vict, TO_CHAR);
				act("$N ������$G ��� ������ ������.", FALSE, vict, 0, ch, TO_CHAR);
				act("$n ������$g ������ ������ $N2.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
				if (spellnum)
					affect_from_char(vict, spellnum);
			}
			else
			{
				act("�� �������������� ���������� ������� ������ ������ $N2.",
					FALSE, ch, 0, vict, TO_CHAR);
				act("$N �������������� �������$U ������� ��� ������ ������.",
					FALSE, vict, 0, ch, TO_CHAR);
				act("$n �������������� �������$u ������� ������ ������ $N2.",
					TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
			}
		}
		else
		{
			if (success)
			{
				act("�� ������� ���� ������ ������.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n ������$g ���� ������ ������.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				if (spellnum)
					affect_from_char(vict, spellnum);
			}
			else
			{
				act("�� �������������� ���������� ������� ���� ������ ������.",
					FALSE, ch, 0, vict, TO_CHAR);
				act("$n �������������� �������$u ������� ���� ������ ������.",
					FALSE, ch, 0, vict, TO_ROOM | TO_ARENA_LISTEN);
			}
		}
	}
}

void do_poisoned(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (!ch->get_skill(SKILL_POISONED))
	{
		send_to_char("�� �� ������ �����.", ch);
		return;
	}

	argument = one_argument(argument, arg);
	skip_spaces(&argument);

	if (!*arg)
	{
		send_to_char("��� �� ������ ��������?\r\n", ch);
		return;
	}
	else if (!*argument)
	{
		send_to_char("�� ���� �� ��������� ����� ��?\r\n", ch);
		return;
	}

	OBJ_DATA *weapon = 0;
	CHAR_DATA *dummy = 0;
	const int result = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &weapon);

	if (!weapon || !result)
	{
		send_to_char(ch, "� ��� ��� \'%s\'.\r\n", arg);
		return;
	}
	else if (GET_OBJ_TYPE(weapon) != OBJ_DATA::ITEM_WEAPON)
	{
		send_to_char("�� ������ ������� �� ������ �� ������.\r\n", ch);
		return;
	}

	OBJ_DATA *cont = get_obj_in_list_vis(ch, argument, ch->carrying);
	if (!cont)
	{
		send_to_char(ch, "� ��� ��� \'%s\'.\r\n", argument);
		return;
	}
	else if (GET_OBJ_TYPE(cont) != OBJ_DATA::ITEM_DRINKCON)
	{
		send_to_char(ch, "%s �� �������� ��������.\r\n", cont->get_PName(0).c_str());
		return;
	}
	else if (GET_OBJ_VAL(cont, 1) <= 0)
	{
		send_to_char(ch, "� %s ��� ������� ��������.\r\n", cont->get_PName(5).c_str());
		return;
	}
	else if (!poison_in_vessel(GET_OBJ_VAL(cont, 2)))
	{
		send_to_char(ch, "� %s ��� ����������� ���.\r\n", cont->get_PName(5).c_str());
		return;
	}

	const int cost = MIN(GET_OBJ_VAL(cont, 1), GET_LEVEL(ch) <= 10 ? 1 : GET_LEVEL(ch) <= 20 ? 2 : 3);
	cont->set_val(1, cont->get_val(1) - cost);
	weight_change_object(cont, -cost);
	if (!GET_OBJ_VAL(cont, 1))
	{
		name_from_drinkcon(cont);
	}

	set_weap_poison(weapon, cont->get_val(2));

	snprintf(buf, sizeof(buf), "�� ��������� ������� ������� %s �� $o3.", drinks[cont->get_val(2)]);
	act(buf, FALSE, ch, weapon, 0, TO_CHAR);
	act("$n ��������� �����$q �� �� $o3.", FALSE, ch, weapon, 0, TO_ROOM | TO_ARENA_LISTEN);
}

void do_repair(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	OBJ_DATA *obj;
	int prob, percent = 0, decay;

	if (!ch->get_skill(SKILL_REPAIR))
	{
		send_to_char("�� �� ������ �����.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (ch->get_fighting())
	{
		send_to_char("�� �� ������ ������� ��� � ���!\r\n", ch);
		return;
	}

	if (!*arg)
	{
		send_to_char("��� �� ������ �������������?\r\n", ch);
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		sprintf(buf, "� ��� ��� \'%s\'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	};

	if (GET_OBJ_MAX(obj) <= GET_OBJ_CUR(obj))
	{
		act("$o � ������� �� ���������.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_WEAPON
		&& !ObjSystem::is_armor_type(obj))
	{
		send_to_char("�� ������ ��������������� ������ ������ ��� �����.\r\n", ch);
		return;
	}

	prob = number(1, skill_info[SKILL_REPAIR].max_percent);
	percent = train_skill(ch, SKILL_REPAIR, skill_info[SKILL_REPAIR].max_percent, 0);
	if (prob > percent)
	{
//Polos.repair_bug
//������ ��� 0 ���������� ������ ��������� ���� ��� ����� 100+ �
//��������� ������ <����� ������>
		if (!percent)
		{
			percent = ch->get_skill(SKILL_REPAIR) / 10;
		}
//-Polos.repair_bug
		obj->set_current_durability(MAX(0, obj->get_current_durability() * percent / prob));
		if (obj->get_current_durability())
		{
			act("�� ���������� �������� $o3, �� ������� $S ��� ������.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n �������$u �������� $o3, �� ������$g $S ��� ������.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
			decay = (GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 10;
			decay = MAX(1, MIN(decay, GET_OBJ_MAX(obj) / 20));
			if (GET_OBJ_MAX(obj) > decay)
			{
				obj->set_maximum_durability(obj->get_maximum_durability() - decay);
			}
			else
			{
				obj->set_maximum_durability(1);
			}
		}
		else
		{
			act("�� ������������ �������� $o3.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n ������������ �������$g $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
			extract_obj(obj);
		}
	}
	else
	{
		// �������. � ����� ����������� ��� ���������
		if (!IS_IMMORTAL(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SMITH))
		{
			obj->set_maximum_durability(obj->get_maximum_durability() - MAX(1, (GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 40));
		}
		obj->set_current_durability(MIN(GET_OBJ_MAX(obj), GET_OBJ_CUR(obj) * percent / prob + 1));
		send_to_char(ch, "������ %s ������%s �����.\r\n", obj->get_PName(0).c_str(), GET_OBJ_POLY_1(ch, obj));
		act("$n ����� �������$g $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
}

bool skill_to_skin(CHAR_DATA *mob, CHAR_DATA *ch)
{
	int num;
	switch (GET_LEVEL(mob)/11)
	{
	case 0:
			num = 15 * animals_levels[0] / 2201; // �������� ���������� � ���������� ������ �� 15.11.2015 � ����
			if (number(1, 100) <= num)
			return true;
	break;
	case 1:
		if (ch->get_skill(SKILL_MAKEFOOD) >= 40)
		{
			num = 20 * animals_levels[1] / 701;
			if (number(1, 100) <= num)
				return true;
		}
		else
		{
			sprintf(buf, "���� ������ ������� ������, ����� ������� ����� %s.\r\n", GET_PAD(mob, 1));
			send_to_char(buf, ch);
			return false;
		}
		
	break;
	case 2:
		if (ch->get_skill(SKILL_MAKEFOOD) >= 80)
		{
			num = 10 * animals_levels[2] / 594;
			if (number(1, 100) <= num)
				return true;
		}
		else
		{
			sprintf(buf, "���� ������ ������� ������, ����� ������� ����� %s.\r\n", GET_PAD(mob, 1));
			send_to_char(buf, ch);
			return false;
		}
		break;

	case 3:
		if (ch->get_skill(SKILL_MAKEFOOD) >= 120)
		{
			num = 8 * animals_levels[3] / 209;
			if (number(1, 100) <= num)
				return true;
		}
		else
		{
			sprintf(buf, "���� ������ ������� ������, ����� ������� ����� %s.\r\n", GET_PAD(mob, 1));
			send_to_char(buf, ch);
			return false;
		}
		break;

	case 4:
		if (ch->get_skill(SKILL_MAKEFOOD) >= 160)
		{
			num = 25 * animals_levels[4] / 20;
			if (number(1, 100) <= num)
				return true;
		}
		else
		{
			sprintf(buf, "���� ������ ������� ������, ����� ������� ����� %s.\r\n", GET_PAD(mob, 1));
			send_to_char(buf, ch);
			return false;
		}
		break;
	//TODO: �������� ��� ����� ���� 54 ������
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	default:
		return false;
	}
	return false;
}
void do_makefood(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	OBJ_DATA *obj;
	CHAR_DATA *mob;
	int prob, percent = 0, mobn;

	if (!ch->get_skill(SKILL_MAKEFOOD))
	{
		send_to_char("�� �� ������ �����.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("��� �� ������ ����������?\r\n", ch);
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))
		&& !(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents)))
	{
		sprintf(buf, "�� �� ������ ����� '%s'.\r\n", arg);
		send_to_char(buf, ch);
		return;
	}

	if (!IS_CORPSE(obj) || (mobn = GET_OBJ_VAL(obj, 2)) < 0)
	{
		act("�� �� ������� ���������� $o3.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	mob = (mob_proto + real_mobile(mobn));
	mob->set_normal_morph();

	if (!IS_IMMORTAL(ch) && ((GET_RACE(mob) != NPC_RACE_ANIMAL) && (GET_RACE(mob) != NPC_RACE_REPTILE) && (GET_RACE(mob) != NPC_RACE_FISH) && (GET_RACE(mob) != NPC_RACE_BIRD) && 
		(GET_RACE(mob) != NPC_RACE_HUMAN_ANIMAL)))
	{
		send_to_char("���� ���� ���������� ����������.\r\n", ch);
		return;
	}

	if (GET_WEIGHT(mob) < 11)
	{
		send_to_char("���� ���� ������� ���������, ������ �� ���������.\r\n", ch);
		return;
	}

	prob = number(1, skill_info[SKILL_MAKEFOOD].max_percent);
	percent = train_skill(ch, SKILL_MAKEFOOD, skill_info[SKILL_MAKEFOOD].max_percent, mob)
		+ number(1, GET_REAL_DEX(ch)) + number(1, GET_REAL_STR(ch));
	OBJ_DATA::shared_ptr tobj;
	if (prob > percent
		|| !(tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.random_key()))) // ��������� � ������ ���������� ���� ��������, ��������� ��������� ����
	{
		act("�� �� ������ ���������� $o3.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n �������$u ���������� $o3, �� ��������.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
	else
	{
		act("$n ����� ���������$g $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		act("�� ����� ���������� $o3.", FALSE, ch, obj, 0, TO_CHAR);
		
		dl_load_obj(obj, mob, ch, DL_SKIN);

		std::vector<OBJ_DATA*> entrails;
		if ((GET_SKILL(ch, SKILL_MAKEFOOD) > 150) && (number(1,200) == 1)) // ��������
		{
			tobj = world_objects.create_from_prototype_by_vnum(meat_mapping.get_artefact_key());
		}
		entrails.push_back(tobj.get());
		if (GET_RACE(mob) == NPC_RACE_ANIMAL) // ����� ������ � ��������
		{
			if (skill_to_skin(mob, ch))
			{
				entrails.push_back(create_skin(mob, ch));
			}
		}
		entrails.push_back(try_make_ingr(mob, 1000 - ch->get_skill(SKILL_MAKEFOOD) * 2, 100));  // ����� �� ����
		for (const auto& it : entrails)
		{
			if (it)
			{
				if (obj->get_carried_by() == ch)
				{
					can_carry_obj(ch, it);
				}
				else
				{
					obj_to_room(it, ch->in_room);
				}
			}
		}
	}

	extract_obj(obj);
}

void feed_charmice(CHAR_DATA * ch, char *arg)
{
	OBJ_DATA *obj;
	int max_charm_duration = 1;
	int chance_to_eat = 0;
	struct follow_type *k;
	int reformed_hp_summ = 0;

	obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents);

	if (!obj || !IS_CORPSE(obj) || !ch->has_master())
	{
		return;
	}

	for (k = ch->get_master()->followers; k; k = k->next)
	{
		if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM)
			&& k->follower->get_master() == ch->get_master())
		{
			reformed_hp_summ += get_reformed_charmice_hp(ch->get_master(), k->follower, SPELL_ANIMATE_DEAD);
		}
	}

	if (reformed_hp_summ >= get_player_charms(ch->get_master(), SPELL_ANIMATE_DEAD))
	{
		send_to_char("�� �� ������ ��������� ��������� ���������������.\r\n", ch->get_master());
		extract_char(ch, FALSE);
		return;
	}

	int mob_level = 1;
	// ���� �� ������
	if (GET_OBJ_VAL(obj, 2) != -1)
	{
		mob_level = GET_LEVEL(mob_proto + real_mobile(GET_OBJ_VAL(obj, 2)));
	}
	const int max_heal_hp = 3 * mob_level;
	chance_to_eat = (100 - 2 * mob_level) / 2;
	//Added by Ann
	if (affected_by_spell(ch->get_master(), SPELL_FASCINATION))
	{
		chance_to_eat -= 30;
	}
	//end Ann
	if (number(1, 100) < chance_to_eat)
	{
		act("$N �������$U � �����$G ������ �������.", TRUE, ch, NULL, ch, TO_ROOM | TO_ARENA_LISTEN);
		GET_HIT(ch) -= 3 * mob_level;
		update_pos(ch);
		// ��������� ��������.
		if (GET_POS(ch) == POS_DEAD)
		{
			die(ch, NULL);
		}
		extract_obj(obj);
		return;
	}
	if (weather_info.moon_day < 14)
	{
		max_charm_duration = pc_duration(ch, GET_REAL_WIS(ch->get_master()) - 6 + number(0, weather_info.moon_day % 14), 0, 0, 0, 0);
	}
	else
	{
		max_charm_duration = pc_duration(ch, GET_REAL_WIS(ch->get_master()) - 6 + number(0, 14 - weather_info.moon_day % 14), 0, 0, 0, 0);
	}

	AFFECT_DATA<EApplyLocation> af;
	af.type = SPELL_CHARM;
	af.duration = MIN(max_charm_duration, (int)(mob_level * max_charm_duration / 30));
	af.modifier = 0;
	af.location = EApplyLocation::APPLY_NONE;
	af.bitvector = to_underlying(EAffectFlag::AFF_CHARM);
	af.battleflag = 0;

	affect_join_fspell(ch, af);

	act("������ ������, $N ������$G ����.", TRUE, ch, obj, ch, TO_ROOM | TO_ARENA_LISTEN);
	act("������, ��������� �������� �� �����.", TRUE, ch, NULL, ch->get_master(), TO_VICT);
	act("�� �������������� ������� ��� ���� �� ���������.", TRUE, ch, NULL, ch->get_master(), TO_NOTVICT | TO_ARENA_LISTEN);

	if (GET_HIT(ch) < GET_MAX_HIT(ch))
	{
		GET_HIT(ch) = MIN(GET_HIT(ch) + MIN(max_heal_hp, GET_MAX_HIT(ch)), GET_MAX_HIT(ch));
	}

	if (GET_HIT(ch) >= GET_MAX_HIT(ch))
	{
		act("$n ���� ������$g � ���������� ���������$g �� ���.", TRUE, ch, NULL, ch->get_master(), TO_VICT);
		act("$n ���� ������$g � ���������� ���������$g �� $N3.", TRUE, ch, NULL, ch->get_master(), TO_NOTVICT | TO_ARENA_LISTEN);
	}

	extract_obj(obj);
}

// ���� �� ������� �����. ������������ �����, � �������� �� ����.
#define MAX_LABEL_LENGTH 32
void do_custom_label(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	char arg3[MAX_INPUT_LENGTH];
	char arg4[MAX_INPUT_LENGTH];

	OBJ_DATA *target = NULL;
	int erase_only = 0; // 0 -- ������� ����� �����, 1 -- ������� ������
	int clan = 0; // ���� �����, ���� �������. ������������, ���� ��
	int no_target = 0; // ������� ������� �� ������, ����� ����� �����

	char *objname = NULL;
	char *labels = NULL;

	if (IS_NPC(ch))
		return;

	half_chop(argument, arg1, arg2);

	if (!strlen(arg1))
		no_target = 1;
	else
	{
		if (!strcmp(arg1, "����")) { // ���� � arg1 "����", �� � arg2 ���� �������� ������� � �����
			clan = 1;
			if (strlen(arg2)) {
				half_chop(arg2, arg3, arg4); // � arg3 �������� �������� �������
				objname = str_dup(arg3);
				if (strlen(arg4))
					labels = str_dup(arg4);
				else
					erase_only = 1;
			}
			else {
				no_target = 1;
			}
		}
		else { // ����� "����" �� �����, ������, ������� � arg1 ����� ��� ������� � ����� � arg2
			if (strlen(arg1)) {
				objname = str_dup(arg1);
				if (strlen(arg2))
					labels = str_dup(arg2);
				else
					erase_only = 1;
			}
			else {
				no_target = 1;
			}
		}
	}

	if (no_target)
	{
		send_to_char("�� ��� ��������?\r\n", ch);
	}
	else
	{
		if (!(target = get_obj_in_list_vis(ch, objname, ch->carrying)))
		{
			sprintf(buf, "� ��� ��� \'%s\'.\r\n", objname);
			send_to_char(buf, ch);
		}
		else
		{
			if (erase_only)
			{
				target->remove_custom_label();
				act("�� ������� ������� �� $o5.", FALSE, ch, target, 0, TO_CHAR);
			}
			else if (labels)
			{
				if (strlen(labels) > MAX_LABEL_LENGTH)
					labels[MAX_LABEL_LENGTH] = '\0';

				// ������� ������
				for (int i = 0; labels[i] != '\0'; i++)
					if (labels[i] == '~')
						labels[i] = '-';

				const std::shared_ptr<custom_label> label(new custom_label());
				label->label_text = str_dup(labels);
				label->author = ch->get_idnum();
				label->author_mail = str_dup(GET_EMAIL(ch));

				const char* msg = "�� ������� $o3 ����������, ������� ����� ����� ��� �� ��������.";
				if (clan && ch->player_specials->clan)
				{
					label->clan = str_dup(ch->player_specials->clan->GetAbbrev());
					msg = "�� ������� $o3 ����������, ��������� ����� ��� ����� ����������.";
				}
				target->set_custom_label(label);
				act(msg, FALSE, ch, target, 0, TO_CHAR);
			}
		}
	}

	if (objname)
	{
		free(objname);
	}

	if (labels)
	{
		free(labels);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
