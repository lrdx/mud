/* ************************************************************************
*   File: act.informative.cpp                           Part of Bylins    *
*  Usage: Player-level commands of an informative nature                  *
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
#include "world.characters.hpp"
#include "object.prototypes.hpp"
#include "logger.hpp"
#include "shutdown.parameters.hpp"
#include "obj.hpp"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "skills.h"
#include "fight.h"
#include "screen.h"
#include "constants.h"
#include "pk.h"
#include "dg_scripts.h"
#include "mail.h"
#include "parcel.hpp"
#include "features.hpp"
#include "im.h"
#include "house.h"
#include "description.h"
#include "privilege.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "liquid.hpp"
#include "modify.h"
#include "room.hpp"
#include "glory_const.hpp"
#include "player_races.hpp"
#include "map.hpp"
#include "mob_stat.hpp"
#include "char_obj_utils.inl"
#include "class.hpp"
#include "structs.h"
#include "sysdep.h"
#include "bonus.h"
#include "conf.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <string>
#include <sstream>
#include <vector>

#define EXIT_SHOW_WALL    (1 << 0)
#define EXIT_SHOW_LOOKING (1 << 1)

void do_quest(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{	
	send_to_char("� ��� ��� ������� ���������� ���������.\r\n����� ����� �����, �������� &W��������� ��������&n.\r\n", ch);
}

const char *Dirs[NUM_OF_DIRS + 1] = { "�����",
									  "������",
									  "��",
									  "�����",
									  "�����",
									  "����",
									  "\n"
									};

const char *ObjState[8][2] = { {"�����������", "�����������"},
	{"��������", "� ��������� ���������"},
	{"�����", "� ������ ���������"},
	{"�������", "� �������� ���������"},
	{"������", "� ������� ���������"},
	{"������", "� ������� ���������"},
	{"����� ������", "� ����� ������� ���������"},
	{"�����������", "� ������������ ���������"}
};

const char *Locks[4][2] =
{
	{"%s �� � ����� �� ������ ��������� �����.%s\r\n", KIRED},
	{"%s ����� ����� �������.%s\r\n", KIYEL},
	{"%s ������� �����. ��� �� �� �������.%s\r\n", KIGRN},
	{"%s ������� �����. ��� ��������.%s\r\n", KGRN}
};

void do_check(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
    login_change_invoice(ch);
}

char *diag_obj_to_char(CHAR_DATA* i, OBJ_DATA* obj, int mode)
{
	static char out_str[80] = "\0";
	const char *color;
	int percent;

	if (GET_OBJ_MAX(obj) > 0)
		percent = 100 * GET_OBJ_CUR(obj) / GET_OBJ_MAX(obj);
	else
		percent = -1;

	if (percent >= 100)
	{
		percent = 7;
		color = CCWHT(i, C_NRM);
	}
	else if (percent >= 90)
	{
		percent = 6;
		color = CCIGRN(i, C_NRM);
	}
	else if (percent >= 75)
	{
		percent = 5;
		color = CCGRN(i, C_NRM);
	}
	else if (percent >= 50)
	{
		percent = 4;
		color = CCIYEL(i, C_NRM);
	}
	else if (percent >= 30)
	{
		percent = 3;
		color = CCIRED(i, C_NRM);
	}
	else if (percent >= 15)
	{
		percent = 2;
		color = CCRED(i, C_NRM);
	}
	else if (percent > 0)
	{
		percent = 1;
		color = CCNRM(i, C_NRM);
	}
	else
	{
		percent = 0;
		color = CCINRM(i, C_NRM);
	}

	if (mode == 1)
		sprintf(out_str, " %s<%s>%s", color, ObjState[percent][0], CCNRM(i, C_NRM));
	else if (mode == 2)
		strcpy(out_str, ObjState[percent][1]);

	return out_str;
}


const char *weapon_class[] = { "����",
							   "�������� ������",
							   "������� ������",
							   "������",
							   "������ � ������",
							   "���� ������",
							   "����������",
							   "����������� ������",
							   "����� � ��������"
							 };

char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear)
{
	static char out_str[MAX_STRING_LENGTH];
	int skill = 0;
	int need_str = 0;

	*out_str = '\0';
	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_WEAPON:
		switch (GET_OBJ_SKILL(obj))
		{
		case SKILL_BOWS:
			skill = 1;
			break;
		case SKILL_SHORTS:
			skill = 2;
			break;
		case SKILL_LONGS:
			skill = 3;
			break;
		case SKILL_AXES:
			skill = 4;
			break;
		case SKILL_CLUBS:
			skill = 5;
			break;
		case SKILL_NONSTANDART:
			skill = 6;
			break;
		case SKILL_BOTHHANDS:
			skill = 7;
			break;
		case SKILL_PICK:
			skill = 8;
			break;
		case SKILL_SPADES:
			skill = 9;
			break;
		default:
			sprintf(out_str, "!! �� ����������� � ��������� ����� ������ - �������� ����� !!\r\n");
		}
		if (skill)
		{
			sprintf(out_str, "����������� � ������ \"%s\".\r\n", weapon_class[skill - 1]);
		}

	default:
		if (show_wear)
		{
			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� �����.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� ���.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� ��������.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� ������.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� ����.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET))
			{
				sprintf(out_str + strlen(out_str), "����� �����.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� �����.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� ����.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� �����.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� ����.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_QUIVER))
			{
				sprintf(out_str + strlen(out_str), "����� ������������ ��� ������.\r\n");
			}

			if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST))
			{
				sprintf(out_str + strlen(out_str), "����� ������ �� ��������.\r\n");
			}

			if (show_wear > 1)
			{
				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
				{
					need_str = MAX(0, calc_str_req((GET_OBJ_WEIGHT(obj)+1)/2, STR_HOLD_W));
					sprintf(out_str + strlen(out_str), "����� ������������ ��� ��� (��������� %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
				{
					need_str = MAX(0, calc_str_req(GET_OBJ_WEIGHT(obj), STR_WIELD_W));
					sprintf(out_str + strlen(out_str), "����� ����� � ������ ���� (��������� %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
				{
					need_str = MAX(0, calc_str_req(GET_OBJ_WEIGHT(obj), STR_HOLD_W));
					sprintf(out_str + strlen(out_str), "����� ����� � ����� ���� (��������� %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
				{
					need_str = MAX(0, calc_str_req(GET_OBJ_WEIGHT(obj), STR_BOTH_W));
					sprintf(out_str + strlen(out_str), "����� ����� � ��� ���� (��������� %d %s).\r\n", need_str, desc_count(need_str, WHAT_STR));
				}
			}
			else
			{
				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
				{
					sprintf(out_str + strlen(out_str), "����� ������������ ��� ���.\r\n");
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
				{
					sprintf(out_str + strlen(out_str), "����� ����� � ������ ����.\r\n");
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
				{
					sprintf(out_str + strlen(out_str), "����� ����� � ����� ����.\r\n");
				}

				if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
				{
					sprintf(out_str + strlen(out_str), "����� ����� � ��� ����.\r\n");
				}
			}
		}
	}
	return (out_str);
}

// ����� ����� ���� �������� ������ ������ ��������
const char *diag_obj_timer(const OBJ_DATA* obj)
{
	int prot_timer;
	if (GET_OBJ_RNUM(obj) != NOTHING)
	{
		if (check_unlimited_timer(obj))
		{
			return "��������";
		}

		if (GET_OBJ_CRAFTIMER(obj) > 0)
		{
			prot_timer = GET_OBJ_CRAFTIMER(obj);// ���� ���� ���������, ������� �� ������ � �� � ���������
		}
		else 
		{
			prot_timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		}

		if (!prot_timer)
		{
			return "�������� �������� ����� ������� ������!\r\n";
		}

		const int tm = (obj->get_timer() * 100 / prot_timer); // ���� ���� ���������, ������� �� ������ � �� � ���������
		return print_obj_state(tm);
	}

	return "";
}

char *diag_timer_to_char(const OBJ_DATA* obj)
{
	static char out_str[MAX_STRING_LENGTH];
	*out_str = 0;
	sprintf(out_str, "���������: %s.\r\n", diag_obj_timer(obj));
	return (out_str);
}

char *diag_uses_to_char(OBJ_DATA * obj, CHAR_DATA * ch)
{
	static char out_str[MAX_STRING_LENGTH];

	*out_str = 0;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_INGREDIENT
		&& IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES)
		&& GET_CLASS(ch) == CLASS_DRUID)
	{
		int i = -1;
		if ((i = real_object(GET_OBJ_VAL(obj, 1))) >= 0)
		{
			sprintf(out_str, "��������: %s%s%s.\r\n",
				CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
		}
		sprintf(out_str + strlen(out_str), "�������� ����������: %s%d&n.\r\n",
			GET_OBJ_VAL(obj, 2) > 100 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
	}
	return (out_str);
}

char *diag_shot_to_char(OBJ_DATA * obj, CHAR_DATA * ch)
{
	static char out_str[MAX_STRING_LENGTH];

	*out_str = 0;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_MAGIC_CONTAINER
		&& (GET_CLASS(ch) == CLASS_RANGER||GET_CLASS(ch) == CLASS_CHARMMAGE||GET_CLASS(ch) == CLASS_DRUID))
	{
		sprintf(out_str + strlen(out_str), "�������� �����: %s%d&n.\r\n",
			GET_OBJ_VAL(obj, 2) > 3 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
	}
	return (out_str);
}

/**
* ��� ������ ����� � ������� ���� � ��� �������� ����������� � ������ ������ ������ ������
* (��� ������ ������), ��������� ������� ������� ������ �����!
*/
std::string space_before_string(const char* text)
{
	if (text)
	{
		std::string tmp(" ");
		tmp += text;
		boost::replace_all(tmp, "\n", "\n ");
		boost::trim_right_if(tmp, boost::is_any_of(std::string(" ")));
		return tmp;
	}

	return std::string();
}

std::string space_before_string(const std::string& text)
{
	if (!text.empty())
	{
		std::string tmp(" ");
		tmp += text;
		boost::replace_all(tmp, "\n", "\n ");
		boost::trim_right_if(tmp, boost::is_any_of(std::string(" ")));
		return tmp;
	}

	return std::string();
}

namespace
{

std::string diag_armor_type_to_char(const OBJ_DATA *obj)
{
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_LIGHT)
	{
		return "������ ��� ��������.\r\n";
	}
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_MEDIAN)
	{
		return "������� ��� ��������.\r\n";
	}
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR_HEAVY)
	{
		return "������� ��� ��������.\r\n";
	}
	return "";
}

} // namespace

// ��� ������������� � ������:
// ���������� ����� ��������, ���� ��� ���� � ��������� �������� �� ������� ��� �������� ������ �����. �����
std::string char_get_custom_label(OBJ_DATA *obj, CHAR_DATA *ch)
{
	const char *delim_l = NULL;
	const char *delim_r = NULL;

	// ������ ������ ��� �������� � ������
	if (obj->get_custom_label() && (ch->player_specials->clan && obj->get_custom_label()->clan != NULL &&
	    !strcmp(obj->get_custom_label()->clan, ch->player_specials->clan->GetAbbrev())))
	{
		delim_l = " *";
		delim_r = "*";
	} else {
		delim_l = " (";
		delim_r = ")";
	}

	if (AUTH_CUSTOM_LABEL(obj, ch))
	{
		return boost::str(boost::format("%s%s%s") % delim_l % obj->get_custom_label()->label_text % delim_r);
	}

	return "";
}

// mode 1 show_state 3 ��� ��������� (4 - ��������� ������)
const char *show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how)
{
	*buf = '\0';
	if ((mode < 5) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
		sprintf(buf, "[%5d] ", GET_OBJ_VNUM(object));

	if (mode == 0
		&& !object->get_description().empty())
	{
		strcat(buf, object->get_description().c_str());
		strcat(buf, char_get_custom_label(object, ch).c_str());
	}
	else if (!object->get_short_description().empty() && ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4)))
	{
		strcat(buf, object->get_short_description().c_str());
		strcat(buf, char_get_custom_label(object, ch).c_str());
	}
	else if (mode == 5)
	{
		if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_NOTE)
		{
			if (!object->get_action_description().empty())
			{
				strcpy(buf, "�� ��������� ��������� :\r\n\r\n");
				strcat(buf, space_before_string(object->get_action_description()).c_str());
				page_string(ch->desc, buf, 1);
			}
			else
			{
				send_to_char("�����.\r\n", ch);
			}
			return 0;
		}
		else if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_BANDAGE)
		{
			strcpy(buf, "����� ��� ��������� ��� ('����������').\r\n");
			snprintf(buf2, MAX_STRING_LENGTH, "�������� ����������: %d, ��������������: %d",
				GET_OBJ_WEIGHT(object), GET_OBJ_VAL(object, 0) * 10);
			strcat(buf, buf2);
		}
		else if (GET_OBJ_TYPE(object) != OBJ_DATA::ITEM_DRINKCON)
		{
			strcpy(buf, "�� �� ������ ������ ����������.");
		}
		else		// ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN
		{
			strcpy(buf, "��� ������� ��� ��������.");
		}
	}

	if (show_state && show_state != 3 && show_state != 4)
	{
		*buf2 = '\0';
		if (mode == 1 && how <= 1)
		{
			if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_LIGHT)
			{
				if (GET_OBJ_VAL(object, 2) == -1)
					strcpy(buf2, " (������ ����)");
				else if (GET_OBJ_VAL(object, 2) == 0)
					sprintf(buf2, " (�����%s)", GET_OBJ_SUF_4(object));
				else
					sprintf(buf2, " (%d %s)",
							GET_OBJ_VAL(object, 2), desc_count(GET_OBJ_VAL(object, 2), WHAT_HOUR));
			}
			else
			{
				if (object->timed_spell().is_spell_poisoned() != -1)
				{
					sprintf(buf2, " %s*%s%s", CCGRN(ch, C_NRM),
						CCNRM(ch, C_NRM), diag_obj_to_char(ch, object, 1));
				}
				else
				{
					sprintf(buf2, " %s", diag_obj_to_char(ch, object, 1));
				}
			}
			if ((GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_CONTAINER) && !OBJVAL_FLAGGED(object, CONT_CLOSED)) // ���� �������, ���������� �� ����������
			{
				if (object->get_contains())
				{
					strcat(buf2, " (���� ����������)");
				}
				else
				{
					if (GET_OBJ_VAL(object, 3) < 1) // ���� ���� ��� ��������, ������� �� ����������2
						sprintf(buf2 + strlen(buf2), " (����%s)", GET_OBJ_SUF_6(object));
				}
			}
		}
		else if (mode >= 2 && how <= 1)
		{
			std::string obj_name = OBJN(object, ch, 0);
			obj_name[0] = UPPER(obj_name[0]);
			if (GET_OBJ_TYPE(object) == OBJ_DATA::ITEM_LIGHT)
			{
				if (GET_OBJ_VAL(object, 2) == -1)
				{
					sprintf(buf2, "\r\n%s ���� ������ ����.", obj_name.c_str());
				}
				else if (GET_OBJ_VAL(object, 2) == 0)
				{
					sprintf(buf2, "\r\n%s �����%s.", obj_name.c_str(), GET_OBJ_SUF_4(object));
				}
				else
				{
					sprintf(buf2, "\r\n%s ����� ������� %d %s.", obj_name.c_str(), GET_OBJ_VAL(object, 2),
						desc_count(GET_OBJ_VAL(object, 2), WHAT_HOUR));
				}
			}
			else if (GET_OBJ_CUR(object) < GET_OBJ_MAX(object))
			{
				sprintf(buf2, "\r\n%s %s.", obj_name.c_str(), diag_obj_to_char(ch, object, 2));
			}
		}
		strcat(buf, buf2);
	}
	if (how > 1)
	{
		sprintf(buf + strlen(buf), " [%d]", how);
	}
	if (mode != 3 && how <= 1)
	{
		if (object->get_extra_flag(EExtraFlag::ITEM_INVISIBLE))
		{
			sprintf(buf2, " (�������%s)", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (object->get_extra_flag(EExtraFlag::ITEM_BLESS)
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
			strcat(buf, " ..������� ����!");
		if (object->get_extra_flag(EExtraFlag::ITEM_MAGIC)
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
			strcat(buf, " ..������ ����!");
		if (object->get_extra_flag(EExtraFlag::ITEM_POISONED)
				&& AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON))
		{
			sprintf(buf2, "..��������%s!", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (object->get_extra_flag(EExtraFlag::ITEM_GLOW))
			strcat(buf, " ..�������!");
		if (object->get_extra_flag(EExtraFlag::ITEM_HUM) && !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
			strcat(buf, " ..�����!");
		if (object->get_extra_flag(EExtraFlag::ITEM_FIRE))
			strcat(buf, " ..�����!");
		if (object->get_extra_flag(EExtraFlag::ITEM_BLOODY))
		{
			sprintf(buf2, " %s..������%s ������!%s", CCIRED(ch, C_NRM), GET_OBJ_SUF_6(object), CCNRM(ch, C_NRM));
			strcat(buf, buf2);
		}
	}

	if (mode == 1)
	{
		// ����-������, ������� ������ ����� �����������
		if (show_state == 3)
		{
			sprintf(buf + strlen(buf), " [%d %s]\r\n",
					GET_OBJ_RENTEQ(object) * CLAN_STOREHOUSE_COEFF / 100,
					desc_count(GET_OBJ_RENTEQ(object) * CLAN_STOREHOUSE_COEFF / 100, WHAT_MONEYa));
			return buf;
		}
		// �����
		else if (show_state == 4)
		{
			sprintf(buf + strlen(buf), " [%d %s]\r\n", GET_OBJ_RENT(object),
					desc_count(GET_OBJ_RENT(object), WHAT_MONEYa));
			return buf;
		}
	}

	strcat(buf, "\r\n");
	if (mode >= 5)
	{
		strcat(buf, diag_weapon_to_char(object, TRUE));
		strcat(buf, diag_armor_type_to_char(object).c_str());
		strcat(buf, diag_timer_to_char(object));
		//strcat(buf, diag_uses_to_char(object, ch)); // commented by WorM ������� � obj_info ����� ������ ��� ���� ����� �� ������/����
		strcat(buf, object->diag_ts_to_char(ch).c_str());
	}
	page_string(ch->desc, buf, TRUE);
	return 0;
}

void do_cities(CHAR_DATA *ch, char*, int, int)
{
	send_to_char("������ �� ����:\r\n", ch);
	for (unsigned int i = 0; i < cities.size(); i++)
	{
		sprintf(buf, "%3d.", i + 1);
		if (IS_IMMORTAL(ch))
		{
			sprintf(buf1, " [VNUM: %d]", cities[i].rent_vnum);
			strcat(buf, buf1);
		}
		sprintf(buf1, " %s: %s\r\n", cities[i].name.c_str(), (ch->check_city(i) ? "&g�� ���� ���.&n" : "&r�� ��� �� ���� ���.&n"));
		strcat(buf, buf1);
		send_to_char(buf, ch);
	}
}

bool quest_item(OBJ_DATA *obj)
{
	if ((OBJ_FLAGGED(obj, EExtraFlag::ITEM_NODECAY)) && (!(CAN_WEAR(obj, EWearFlag::ITEM_WEAR_TAKE))))
	{
		return true;
	}
	return false;
}

void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show)
{
	OBJ_DATA *i, *push = NULL;
	bool found = FALSE;
	int push_count = 0;
	std::ostringstream buffer;
	long count = 0, cost = 0;

	bool clan_chest = false;
	if (mode == 1 && (show == 3 || show == 4))
	{
		clan_chest = true;
	}

	for (i = list; i; i = i->get_next_content())
	{
		if (CAN_SEE_OBJ(ch, i))
		{
			if (!push)
			{
				push = i;
				push_count = 1;
			}
			else if ((!equal_obj(i, push)) 
				|| (quest_item(i)))
			{
				if (clan_chest)
				{
					buffer << show_obj_to_char(push, ch, mode, show, push_count);
					count += push_count;
					cost += GET_OBJ_RENTEQ(push) * push_count;
				}
				else
					show_obj_to_char(push, ch, mode, show, push_count);
				push = i;
				push_count = 1;
			}
			else
				push_count++;
			found = TRUE;
		}
	}
	if (push && push_count)
	{
		if (clan_chest)
		{
			buffer << show_obj_to_char(push, ch, mode, show, push_count);
			count += push_count;
			cost += GET_OBJ_RENTEQ(push) * push_count;
		}
		else
			show_obj_to_char(push, ch, mode, show, push_count);
	}
	if (!found && show)
	{
		if (show == 1)
			send_to_char(" ������ ������ ���.\r\n", ch);
		else if (show == 2)
			send_to_char(" �� ������ �� ������.\r\n", ch);
		else if (show == 3)
		{
			send_to_char(" �����...\r\n", ch);
			return;
		}
	}
	if (clan_chest)
		page_string(ch->desc, buffer.str());
}

void diag_char_to_char(CHAR_DATA * i, CHAR_DATA * ch)
{
	int percent;

	if (GET_REAL_MAX_HIT(i) > 0)
		percent = (100 * GET_HIT(i)) / GET_REAL_MAX_HIT(i);
	else
		percent = -1;	// How could MAX_HIT be < 1??

	strcpy(buf, PERS(i, ch, 0));
	CAP(buf);

	if (percent >= 100)
	{
		sprintf(buf2, " ��������%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 90)
	{
		sprintf(buf2, " ������ ���������%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 75)
	{
		sprintf(buf2, " ����� �����%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 50)
	{
		sprintf(buf2, " �����%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 30)
	{
		sprintf(buf2, " ������ �����%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 15)
	{
		sprintf(buf2, " ���������� �����%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	}
	else if (percent >= 0)
		strcat(buf, " � ������� ���������");
	else
		strcat(buf, " �������");

	if (!on_horse(i))
		switch (GET_POS(i))
			{
			case POS_MORTALLYW:
				strcat(buf, ".");
			break;
			case POS_INCAP:
				strcat(buf, IS_POLY(i) ? ", ����� ��� ��������." : ", ����� ��� ��������.");
			break;
			case POS_STUNNED:
				strcat(buf, IS_POLY(i) ? ", ����� � ��������." : ", ����� � ��������.");
			break;
			case POS_SLEEPING:
				strcat(buf, IS_POLY(i) ? ", ����." : ", ����.");
			break;
			case POS_RESTING:
				strcat(buf, IS_POLY(i) ? ", ��������." : ", ��������.");
			break;
			case POS_SITTING:
				strcat(buf, IS_POLY(i) ? ", �����." : ", �����.");
			break;
			case POS_STANDING:
				strcat(buf, IS_POLY(i) ? ", �����." : ", �����.");
			break;
			case POS_FIGHTING:
				if (i->get_fighting())
					strcat(buf, IS_POLY(i) ? ", ���������." : ", ���������.");
				else
					strcat(buf, IS_POLY(i) ? ", ������ ��������." : ", ������ ��������.");
			break;
			default:
				return;
			break;
		}
	else
		strcat(buf, IS_POLY(i) ? ", ����� ������." : ", ����� ������.");

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON))
		if (AFF_FLAGGED(i, EAffectFlag::AFF_POISON))
		{
			sprintf(buf2, " (��������%s)", GET_CH_SUF_6(i));
			strcat(buf, buf2);
		}

	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}

void look_at_char(CHAR_DATA * i, CHAR_DATA * ch)
{
	int j, found, push_count = 0;
	OBJ_DATA *tmp_obj, *push = NULL;

	if (!ch->desc)
		return;

	if (!i->player_data.description.empty())
	{
		if (IS_NPC(i))
			send_to_char(ch, " * %s", i->player_data.description.c_str());
		else
			send_to_char(ch, "*\r\n%s*\r\n", space_before_string(i->player_data.description).c_str());
	}
	else if (!IS_NPC(i))
	{
		strcpy(buf, "\r\n���");
		if (i->is_morphed())
			strcat(buf, std::string(" " + i->get_morph_desc() + ".\r\n").c_str());
		else
			if (IS_FEMALE(i))
			{
				if (GET_HEIGHT(i) <= 151)
				{
					if (GET_WEIGHT(i) >= 140)
						strcat(buf, " ��������� ������� �������.\r\n");
					else if (GET_WEIGHT(i) >= 125)
						strcat(buf, " ��������� �������.\r\n");
					else
						strcat(buf, " ����������� �������.\r\n");
				}
				else if (GET_HEIGHT(i) <= 159)
				{
					if (GET_WEIGHT(i) >= 145)
						strcat(buf, " ��������� ������� �����.\r\n");
					else if (GET_WEIGHT(i) >= 130)
						strcat(buf, " ��������� �������.\r\n");
					else
						strcat(buf, " ������� ����.\r\n");
				}
				else if (GET_HEIGHT(i) <= 165)
				{
					if (GET_WEIGHT(i) >= 145)
						strcat(buf, " �������� ����� �������.\r\n");
					else
						strcat(buf, " �������� ����� ������� ���������.\r\n");
				}
				else if (GET_HEIGHT(i) <= 175)
				{
					if (GET_WEIGHT(i) >= 150)
						strcat(buf, " ������� �������� ����.\r\n");
					else if (GET_WEIGHT(i) >= 135)
						strcat(buf, " ������� �������� �������.\r\n");
					else
						strcat(buf, " ������� ������� �������.\r\n");
				}
				else
				{
					if (GET_WEIGHT(i) >= 155)
						strcat(buf, " ����� ������� ������� ����.\r\n");
					else if (GET_WEIGHT(i) >= 140)
						strcat(buf, " ����� ������� �������� �������.\r\n");
					else
						strcat(buf, " ����� ������� ��������� �������.\r\n");
				}
			}
			else
			{
				if (GET_HEIGHT(i) <= 165)
				{
					if (GET_WEIGHT(i) >= 170)
						strcat(buf, " ���������, ������� �� �������, �������.\r\n");
					else if (GET_WEIGHT(i) >= 150)
						strcat(buf, " ��������� ������� �������.\r\n");
					else
						strcat(buf, " ��������� ������������ ���������.\r\n");
				}
				else if (GET_HEIGHT(i) <= 175)
				{
					if (GET_WEIGHT(i) >= 175)
						strcat(buf, " ��������� ���������� ������.\r\n");
					else if (GET_WEIGHT(i) >= 160)
						strcat(buf, " ��������� ������� �������.\r\n");
					else
						strcat(buf, " ��������� ��������� �������.\r\n");
				}
				else if (GET_HEIGHT(i) <= 185)
				{
					if (GET_WEIGHT(i) >= 180)
						strcat(buf, " �������� ����� ���������� �������.\r\n");
					else if (GET_WEIGHT(i) >= 165)
						strcat(buf, " �������� ����� ������� �������.\r\n");
					else
						strcat(buf, " �������� ����� ��������� �������.\r\n");
				}
				else if (GET_HEIGHT(i) <= 195)
				{
					if (GET_WEIGHT(i) >= 185)
						strcat(buf, " ������� ������� �������.\r\n");
					else if (GET_WEIGHT(i) >= 170)
						strcat(buf, " ������� �������� �������.\r\n");
					else
						strcat(buf, " �������, ��������� �������.\r\n");
				}
				else
				{
					if (GET_WEIGHT(i) >= 190)
						strcat(buf, " �������� �����.\r\n");
					else if (GET_WEIGHT(i) >= 180)
						strcat(buf, " ����� �������, ������� �����.\r\n");
					else
						strcat(buf, " ���������, ������� �� ����� �������.\r\n");
				}
			}
		send_to_char(buf, ch);
	}
	else
		act("\r\n������ ���������� � $n5 �� �� ��������.", FALSE, i, 0, ch, TO_VICT);

	if (AFF_FLAGGED(i, EAffectFlag::AFF_CHARM)
		&& i->get_master() == ch)
	{
		if (i->low_charm())
		{
			act("$n ����� ���������� ��������� �� ����.", FALSE, i, 0, ch, TO_VICT);
		}
		else
		{
			for (const auto& aff : i->affected)
			{
				if (aff->type == SPELL_CHARM)
				{
					sprintf(buf, IS_POLY(i) ? "$n ����� ��������� ��� ��� %d %s." : "$n ����� ��������� ��� ��� %d %s.", aff->duration / 2, desc_count(aff->duration / 2, 1));
					act(buf, FALSE, i, 0, ch, TO_VICT);
					break;
				}
			}
		}
	}

	if (IS_HORSE(i)
		&& i->get_master() == ch)
	{
		strcpy(buf, "\r\n��� ��� ������. �� ");
		if (GET_HORSESTATE(i) <= 0)
			strcat(buf, "������.\r\n");
		else if (GET_HORSESTATE(i) <= 20)
			strcat(buf, "���� � ����.\r\n");
		else if (GET_HORSESTATE(i) <= 80)
			strcat(buf, "� ������� ���������.\r\n");
		else
			strcat(buf, "�������� ������ ������.\r\n");
		send_to_char(buf, ch);
	};

	diag_char_to_char(i, ch);

	if (i->is_morphed())
	{
		send_to_char("\r\n", ch);
		std::string coverDesc = "$n ������$a " + i->get_cover_desc() + ".";
		act(coverDesc.c_str(), FALSE, i, 0, ch, TO_VICT);
		send_to_char("\r\n", ch);
	}
	else
	{
		found = FALSE;
		for (j = 0; !found && j < NUM_WEARS; j++)
			if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
				found = TRUE;

		if (found)
		{
			send_to_char("\r\n", ch);
			act("$n ����$a :", FALSE, i, 0, ch, TO_VICT);
			for (j = 0; j < NUM_WEARS; j++)
			{
				if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
				{
					send_to_char(where[j], ch);
					if (i->has_master()
						&& IS_NPC(i))
					{
						show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i->get_master(), 1);
					}
					else
					{
						show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i, 1);
					}
				}
			}
		}
	}

	if (ch != i && (ch->get_skill(SKILL_LOOK_HIDE) || IS_IMMORTAL(ch)))
	{
		found = FALSE;
		act("\r\n�� ���������� ��������� � $s ����:", FALSE, i, 0, ch, TO_VICT);
		for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->get_next_content())
		{
			if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 30) < GET_LEVEL(ch)))
			{
				if (!push)
				{
					push = tmp_obj;
					push_count = 1;
				}
				else if (GET_OBJ_VNUM(push) != GET_OBJ_VNUM(tmp_obj)
					|| GET_OBJ_VNUM(push) == -1)
				{
					show_obj_to_char(push, ch, 1, ch == i, push_count);
					push = tmp_obj;
					push_count = 1;
				}
				else
					push_count++;
				found = TRUE;
			}
		}
		if (push && push_count)
			show_obj_to_char(push, ch, 1, ch == i, push_count);
		if (!found)
			send_to_char("...� ������ �� ����������.\r\n", ch);
	}
}


void list_one_char(CHAR_DATA * i, CHAR_DATA * ch, int skill_mode)
{
	int sector = SECT_CITY;
	int n;
	char aura_txt[200];
	const char *positions[] =
	{
		"����� �����, �������. ",
		"����� �����, ��� ������. ",
		"����� �����, ��� ��������. ",
		"����� �����, � ��������. ",
		"���� �����. ",
		"�������� �����. ",
		"����� �����. ",
		"���������! ",
		"����� �����. "
	};

	// ����� � ����� ��� ������������� IS_POLY() - ���� ��� ����������� ������� ����� ���� "���" -- ��������
	const char *poly_positions[] =
	{
		"����� �����, �������. ",
		"����� �����, ��� ������. ",
		"����� �����, ��� ��������. ",
		"����� �����, � ��������. ",
		"���� �����. ",
		"�������� �����. ",
		"����� �����. ",
		"���������! ",
		"����� �����. "
	};

	if (IS_HORSE(i) && on_horse(i->get_master()))
	{
		if (ch == i->get_master())
		{
			if (!IS_POLY(i))
			{
				act("$N ����� ��� �� ����� �����.", FALSE, ch, 0, i, TO_CHAR);
			}
			else
			{
				act("$N ����� ��� �� ����� �����.", FALSE, ch, 0, i, TO_CHAR);
			}
		}

		return;
	}

	if (skill_mode == SKILL_LOOKING)
	{
		if (HERE(i) && INVIS_OK(ch, i) && GET_REAL_LEVEL(ch) >= (IS_NPC(i) ? 0 : GET_INVIS_LEV(i)))
		{
			if (GET_RACE(i)==NPC_RACE_THING && IS_IMMORTAL(ch)) {
				sprintf(buf, "�� ���������� %s.(�������)\r\n", GET_PAD(i, 3));
			} else {
				sprintf(buf, "�� ���������� %s.\r\n", GET_PAD(i, 3));
			}
			send_to_char(buf, ch);
		}
		return;
	}

	if (!CAN_SEE(ch, i))
	{
		skill_mode =
			check_awake(i, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING | ACHECK_WEIGHT);
		*buf = 0;
		if (IS_SET(skill_mode, ACHECK_AFFECTS))
		{
			REMOVE_BIT(skill_mode, ACHECK_AFFECTS);
			sprintf(buf + strlen(buf), "���������� �����%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_LIGHT))
		{
			REMOVE_BIT(skill_mode, ACHECK_LIGHT);
			sprintf(buf + strlen(buf), "����� ����%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_GLOWING)
				&& IS_SET(skill_mode, ACHECK_HUMMING)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
		{
			REMOVE_BIT(skill_mode, ACHECK_GLOWING);
			REMOVE_BIT(skill_mode, ACHECK_HUMMING);
			sprintf(buf + strlen(buf), "��� � ����� ����������%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_GLOWING))
		{
			REMOVE_BIT(skill_mode, ACHECK_GLOWING);
			sprintf(buf + strlen(buf), "����� ����������%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_HUMMING)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
		{
			REMOVE_BIT(skill_mode, ACHECK_HUMMING);
			sprintf(buf + strlen(buf), "��� ����������%s", skill_mode ? ", " : " ");
		}
		if (IS_SET(skill_mode, ACHECK_WEIGHT)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE))
		{
			REMOVE_BIT(skill_mode, ACHECK_WEIGHT);
			sprintf(buf + strlen(buf), "�������� �������%s", skill_mode ? ", " : " ");
		}
		strcat(buf, "������ ���-�� �����������.\r\n");
		send_to_char(CAP(buf), ch);
		return;
	}

	if (IS_NPC(i)
		&& !i->player_data.long_descr.empty()
		&& GET_POS(i) == GET_DEFAULT_POS(i)
		&& ch->in_room == i->in_room
		&& !AFF_FLAGGED(i, EAffectFlag::AFF_CHARM)
		&& !IS_HORSE(i))
	{
		*buf = '\0';
		if (PRF_FLAGGED(ch, PRF_ROOMFLAGS))
		{
			sprintf(buf, "[%5d] ", GET_MOB_VNUM(i));
		}

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
		{
			if (AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
			{
				strcat(buf, "(������ ����) ");
			}
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
		{
			if (IS_NPC(i))
			{
				if (NPC_FLAGGED(i, NPC_AIRCREATURE))
					sprintf(buf + strlen(buf), "%s(���� �������)%s ",
							CCIBLU(ch, C_CMP), CCIRED(ch, C_CMP));
				else if (NPC_FLAGGED(i, NPC_WATERCREATURE))
					sprintf(buf + strlen(buf), "%s(���� ����)%s ",
							CCICYN(ch, C_CMP), CCIRED(ch, C_CMP));
				else if (NPC_FLAGGED(i, NPC_FIRECREATURE))
					sprintf(buf + strlen(buf), "%s(���� ����)%s ",
							CCIMAG(ch, C_CMP), CCIRED(ch, C_CMP));
				else if (NPC_FLAGGED(i, NPC_EARTHCREATURE))
					sprintf(buf + strlen(buf), "%s(���� �����)%s ",
							CCIGRN(ch, C_CMP), CCIRED(ch, C_CMP));
			}
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_INVISIBLE))
			sprintf(buf + strlen(buf), "(�������%s) ", GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HIDE))
			sprintf(buf + strlen(buf), "(�������%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_CAMOUFLAGE))
			sprintf(buf + strlen(buf), "(������������%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_FLY))
			strcat(buf, IS_POLY(i) ? "(�����) " : "(�����) ");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HORSE))
			strcat(buf, "(��� ������) ");

		strcat(buf, i->player_data.long_descr.c_str());
		send_to_char(buf, ch);

		*aura_txt = '\0';
		if (AFF_FLAGGED(i, EAffectFlag::AFF_SHIELD))
		{
			strcat(aura_txt, "...������");
			strcat(aura_txt, GET_CH_SUF_6(i));
			strcat(aura_txt, " ���������� ������� ");
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_SANCTUARY))
			strcat(aura_txt, IS_POLY(i) ? "...�������� ����� ������� " : "...�������� ����� ������� ");
		else if (AFF_FLAGGED(i, EAffectFlag::AFF_PRISMATICAURA))
			strcat(aura_txt, IS_POLY(i) ? "...������������ ����� ������� " : "...������������ ����� ������� ");
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, "...�������");
		strcat(aura_txt, GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRSHIELD))
		{
			strcat(aura_txt, " ���������");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_FIRESHIELD))
		{
			if (n > 0)
				strcat(aura_txt, ", ��������");
			else
				strcat(aura_txt, " ��������");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_ICESHIELD))
		{
			if (n > 0)
				strcat(aura_txt, ", �������");
			else
				strcat(aura_txt, " �������");
			n++;
		}
		if (n == 1)
			strcat(aura_txt, " ����� ");
		else if (n > 1)
			strcat(aura_txt, " ������ ");
		if (n > 0)
			act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
		{
			*aura_txt = '\0';
			n = 0;
			strcat(aura_txt, "...");
			if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRAURA))
			{
				strcat(aura_txt, "���������");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_FIREAURA))
			{
				if (n > 0)
					strcat(aura_txt, ", ��������");
				else
					strcat(aura_txt, "��������");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_ICEAURA))
			{
				if (n > 0)
					strcat(aura_txt, ", �������");
				else
					strcat(aura_txt, "�������");
				n++;
                        }
			if (AFF_FLAGGED(i, EAffectFlag::AFF_EARTHAURA))
			{
				if (n > 0)
					strcat(aura_txt, ", ����������");
				else
					strcat(aura_txt, "����������");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_MAGICGLASS))
			{
				if (n > 0)
					strcat(aura_txt, ", �����������");
				else
					strcat(aura_txt, "�����������");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_BROKEN_CHAINS))
			{
				if (n > 0)
					strcat(aura_txt, ", ����-�����");
				else
					strcat(aura_txt, "����-�����");
				n++;
			}
			if (AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
			{
				if (n > 0)
					strcat(aura_txt, ", ������");
				else
					strcat(aura_txt, "������");
				n++;
			}
			if (n == 1)
				strcat(aura_txt, " ���� ");
			else if (n > 1)
				strcat(aura_txt, " ���� ");

			if (n > 0)
				act(aura_txt, FALSE, i, 0, ch, TO_VICT);
		}
		*aura_txt = '\0';
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
		{
			if (AFF_FLAGGED(i, EAffectFlag::AFF_HOLD))
				strcat(aura_txt, "...�����������$a");
			if (AFF_FLAGGED(i, EAffectFlag::AFF_SILENCE))
				strcat(aura_txt, "...���$a");
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_BLIND))
			strcat(aura_txt, "...����$a");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_DEAFNESS))
			strcat(aura_txt, "...����$a");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_STRANGLED))
			strcat(aura_txt, "...����������.");

		if (*aura_txt)
			act(aura_txt, FALSE, i, 0, ch, TO_VICT);

		return;
	}

	if (IS_NPC(i))
	{
		strcpy(buf1, i->get_npc_name().c_str());
		strcat(buf1, " ");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HORSE))
			strcat(buf1, "(��� ������) ");
		CAP(buf1);
	}
	else
	{
		sprintf(buf1, "%s%s ", i->get_morphed_title().c_str(), PLR_FLAGGED(i, PLR_KILLER) ? " <�������>" : "");
	}

	sprintf(buf, "%s%s", AFF_FLAGGED(i, EAffectFlag::AFF_CHARM) ? "*" : "", buf1);
	if (AFF_FLAGGED(i, EAffectFlag::AFF_INVISIBLE))
		sprintf(buf + strlen(buf), "(�������%s) ", GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, EAffectFlag::AFF_HIDE))
		sprintf(buf + strlen(buf), "(�������%s) ", GET_CH_SUF_2(i));
	if (AFF_FLAGGED(i, EAffectFlag::AFF_CAMOUFLAGE))
		sprintf(buf + strlen(buf), "(������������%s) ", GET_CH_SUF_2(i));
	if (!IS_NPC(i) && !i->desc)
		sprintf(buf + strlen(buf), "(�������%s �����) ", GET_CH_SUF_1(i));
	if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
		strcat(buf, "(�����) ");

	if (GET_POS(i) != POS_FIGHTING)
	{
		if (on_horse(i))
		{
			CHAR_DATA *horse = get_horse(i);
			if (horse)
			{
				const char *msg =
					AFF_FLAGGED(horse, EAffectFlag::AFF_FLY) ? "������" : "�����";
				sprintf(buf + strlen(buf), "%s ����� ������ �� %s. ",
					msg, PERS(horse, ch, 5));
			}
		}
		else if (IS_HORSE(i) && AFF_FLAGGED(i, EAffectFlag::AFF_TETHERED))
			sprintf(buf + strlen(buf), "��������%s �����. ", GET_CH_SUF_6(i));
		else if ((sector = real_sector(i->in_room)) == SECT_FLYING)
			strcat(buf, IS_POLY(i) ? "������ �����. " : "������ �����. ");
		else if (sector == SECT_UNDERWATER)
			strcat(buf, IS_POLY(i) ? "������� �����. " : "������� �����. ");
		else if (GET_POS(i) > POS_SLEEPING && AFF_FLAGGED(i, EAffectFlag::AFF_FLY))
			strcat(buf, IS_POLY(i) ? "������ �����. " : "������ �����. ");
		else if (sector == SECT_WATER_SWIM || sector == SECT_WATER_NOSWIM)
			strcat(buf, IS_POLY(i) ? "������� �����. " : "������� �����. ");
		else
			strcat(buf, IS_POLY(i) ? poly_positions[(int) GET_POS(i)] : positions[(int) GET_POS(i)]);
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC) && IS_NPC(i) && affected_by_spell(i, SPELL_CAPABLE))
			sprintf(buf + strlen(buf), "(���� �����) ");
	}
	else
	{
		if (i->get_fighting())
		{
			strcat(buf, IS_POLY(i) ? "��������� � " : "��������� � ");
			if (i->in_room != i->get_fighting()->in_room)
				strcat(buf, "����-�� �����");
			else if (i->get_fighting() == ch)
				strcat(buf, "����");
			else
				strcat(buf, GET_PAD(i->get_fighting(), 4));
			if (on_horse(i))
				sprintf(buf + strlen(buf), ", ���� ������ �� %s! ", PERS(get_horse(i), ch, 5));
			else
				strcat(buf, "! ");
		}
		else		// NIL fighting pointer
		{
			strcat(buf, IS_POLY(i) ? "������� �� �������" : "������� �� �������");
			if (on_horse(i))
				sprintf(buf + strlen(buf), ", ���� ������ �� %s. ", PERS(get_horse(i), ch, 5));
			else
				strcat(buf, ". ");
		}
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
	{
		if (AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
			strcat(buf, "(������ ����) ");
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
	{
		if (IS_NPC(i))
		{
			if (IS_EVIL(i))
			{
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
						&& AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
					strcat(buf, "(������-������ ����) ");
				else
					strcat(buf, "(������ ����) ");
			}
			else if (IS_GOOD(i))
			{
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
						&& AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
					strcat(buf, "(����� ����) ");
				else
					strcat(buf, "(������� ����) ");
			}
			else
			{
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC)
						&& AFF_FLAGGED(i, EAffectFlag::AFF_EVILESS))
					strcat(buf, "(������ ����) ");
			}
		}
		else
		{
			aura(ch, C_CMP, i, aura_txt);
			strcat(buf, aura_txt);
			strcat(buf, " ");
		}
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON))
		if (AFF_FLAGGED(i, EAffectFlag::AFF_POISON))
			sprintf(buf + strlen(buf), "(��������%s) ", GET_CH_SUF_6(i));

	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	*aura_txt = '\0';
	if (AFF_FLAGGED(i, EAffectFlag::AFF_SHIELD))
	{
		strcat(aura_txt, "...������");
		strcat(aura_txt, GET_CH_SUF_6(i));
		strcat(aura_txt, " ���������� ������� ");
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_SANCTUARY))
		strcat(aura_txt, IS_POLY(i) ? "...�������� ����� ������� " : "...�������� ����� ������� ");
	else if (AFF_FLAGGED(i, EAffectFlag::AFF_PRISMATICAURA))
		strcat(aura_txt, IS_POLY(i) ? "...������������ ����� ������� " : "...������������ ����� ������� ");
	act(aura_txt, FALSE, i, 0, ch, TO_VICT);

	*aura_txt = '\0';
	n = 0;
	strcat(aura_txt, "...�������");
	strcat(aura_txt, GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRSHIELD))
	{
		strcat(aura_txt, " ���������");
		n++;
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_FIRESHIELD))
	{
		if (n > 0)
			strcat(aura_txt, ", ��������");
		else
			strcat(aura_txt, " ��������");
		n++;
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_ICESHIELD))
	{
		if (n > 0)
			strcat(aura_txt, ", �������");
		else
			strcat(aura_txt, " �������");
		n++;
	}
	if (n == 1)
		strcat(aura_txt, " ����� ");
	else if (n > 1)
		strcat(aura_txt, " ������ ");
	if (n > 0)
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_ALIGN))
	{
	*aura_txt = '\0';
	if (AFF_FLAGGED(i, EAffectFlag::AFF_COMMANDER))
		strcat(aura_txt, "... ���� ���� ��� ������� ");
	if (*aura_txt)
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
	{
		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, " ..");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_AIRAURA))
		{
			strcat(aura_txt, "���������");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_FIREAURA))
		{
			if (n > 0)
				strcat(aura_txt, ", ��������");
			else
				strcat(aura_txt, "��������");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_ICEAURA))
		{
			if (n > 0)
				strcat(aura_txt, ", �������");
			else
				strcat(aura_txt, "�������");
			n++;
                }
		if (AFF_FLAGGED(i, EAffectFlag::AFF_EARTHAURA))
		{
			if (n > 0)
				strcat(aura_txt, ", ����������");
			else
				strcat(aura_txt, "����������");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_MAGICGLASS))
		{
			if (n > 0)
				strcat(aura_txt, ", �����������");
			else
				strcat(aura_txt, "�����������");
			n++;
		}
		if (AFF_FLAGGED(i, EAffectFlag::AFF_BROKEN_CHAINS))
		{
			if (n > 0)
				strcat(aura_txt, ", ����-�����");
			else
				strcat(aura_txt, "����-�����");
			n++;
		}
		if (n == 1)
			strcat(aura_txt, " ���� ");
		else if (n > 1)
			strcat(aura_txt, " ���� ");

		if (n > 0)
			act(aura_txt, FALSE, i, 0, ch, TO_VICT);
	}
	*aura_txt = '\0';
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC))
	{
		if (AFF_FLAGGED(i, EAffectFlag::AFF_HOLD))
			strcat(aura_txt, " ...�����������$a");
		if (AFF_FLAGGED(i, EAffectFlag::AFF_SILENCE))
			strcat(aura_txt, " ...���$a");
	}
	if (AFF_FLAGGED(i, EAffectFlag::AFF_BLIND))
		strcat(aura_txt, " ...����$a");
	if (AFF_FLAGGED(i, EAffectFlag::AFF_DEAFNESS))
		strcat(aura_txt, " ...����$a");
	if (AFF_FLAGGED(i, EAffectFlag::AFF_STRANGLED))
		strcat(aura_txt, " ...����������");
	if (*aura_txt)
		act(aura_txt, FALSE, i, 0, ch, TO_VICT);
}

void list_char_to_char(const ROOM_DATA::people_t& list, CHAR_DATA* ch)
{
	for (const auto i : list)
	{
		if (ch != i)
		{
			if (HERE(i) && (GET_RACE(i) != NPC_RACE_THING)
				&& (CAN_SEE(ch, i)
					|| awaking(i, AW_HIDE | AW_INVIS | AW_CAMOUFLAGE)))
			{
				list_one_char(i, ch, 0);
			}
			else if (IS_DARK(i->in_room)
				&& i->in_room == ch->in_room
				&& !CAN_SEE_IN_DARK(ch)
				&& AFF_FLAGGED(i, EAffectFlag::AFF_INFRAVISION))
			{
				send_to_char("���� ���������� ���� ������� �� ���.\r\n", ch);
			}
		}
	}
}
void list_char_to_char_thing(const ROOM_DATA::people_t& list, CHAR_DATA* ch)   //���� ����� ������� ����� �������� ������ � ���������� ����������
{
	for (const auto i : list)
	{
		if (ch != i)
		{
			if (GET_RACE(i) == NPC_RACE_THING)
			{
				list_one_char(i, ch, 0);
			}
		}
	}
}

void do_auto_exits(CHAR_DATA * ch)
{
	int door, slen = 0;

	*buf = '\0';

	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		// �������-�� ��������� ��������� � ����������� �������� ������
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE)
		{
			if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
			{
				slen += sprintf(buf + slen, "(%c) ", LOWER(*dirs[door]));
			}
			else if (!EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN))
			{
				if (world[EXIT(ch, door)->to_room]->zone == world[ch->in_room]->zone)
				{
					slen += sprintf(buf + slen, "%c ", LOWER(*dirs[door]));
				}
				else
				{
					slen += sprintf(buf + slen, "%c ", UPPER(*dirs[door]));
				}
			}
		}
	}
	sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM), *buf ? buf : "None! ", CCNRM(ch, C_NRM));

	send_to_char(buf2, ch);
}

void do_blind_exits(CHAR_DATA *ch)
{
	int door;

	*buf = '\0';

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("�� �����, ��� �������!\r\n", ch);
		return;
	}
	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
		{
			if (IS_GOD(ch))
				sprintf(buf2, "&W%-5s - [%5d] %s ", Dirs[door],
					GET_ROOM_VNUM(EXIT(ch, door)->to_room), world[EXIT(ch, door)->to_room]->name);
			else
			{
				sprintf(buf2, "&W%-5s - ", Dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "������� �����");
				else
				{
					strcat(buf2, world[EXIT(ch, door)->to_room]->name);
					strcat(buf2, "");
				}
			}
			strcat(buf, CAP(buf2));
		}
	send_to_char("������� ������:\r\n", ch);
	if (*buf)
		send_to_char(ch, "%s&n\r\n", buf);
	else
		send_to_char("&W ����������, ������!&n\r\n", ch);
}

void do_exits(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int door;

	*buf = '\0';

	if (PRF_FLAGGED(ch, PRF_BLIND))
	{
		do_blind_exits(ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("�� �����, ��� �������!\r\n", ch);
		return;
	}
	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
		{
			if (IS_GOD(ch))
				sprintf(buf2, "%-5s - [%5d] %s\r\n", Dirs[door],
						GET_ROOM_VNUM(EXIT(ch, door)->to_room), world[EXIT(ch, door)->to_room]->name);
			else
			{
				sprintf(buf2, "%-5s - ", Dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "������� �����\r\n");
				else
				{
					strcat(buf2, world[EXIT(ch, door)->to_room]->name);
					strcat(buf2, "\r\n");
				}
			}
			strcat(buf, CAP(buf2));
		}
	send_to_char("������� ������:\r\n", ch);
	if (*buf)
		send_to_char(buf, ch);
	else
		send_to_char(" ����������, ������!\r\n", ch);
}

#define MAX_FIRES 6
const char *Fires[MAX_FIRES] = { "����� ��������� ����� ��������",
								 "����� ��������� ����� ��������",
								 "���-��� �������� ������",
								 "�������� ��������� ������",
								 "������ ������ ������",
								 "���� ������ ������"
							   };

#define TAG_NIGHT       "<night>"
#define TAG_DAY         "<day>"
#define TAG_WINTERNIGHT "<winternight>"
#define TAG_WINTERDAY   "<winterday>"
#define TAG_SPRINGNIGHT "<springnight>"
#define TAG_SPRINGDAY   "<springday>"
#define TAG_SUMMERNIGHT "<summernight>"
#define TAG_SUMMERDAY   "<summerday>"
#define TAG_AUTUMNNIGHT "<autumnnight>"
#define TAG_AUTUMNDAY   "<autumnday>"

int paste_description(char *string, const char *tag, int need)
{
	if (!*string || !*tag)
	{
		return (FALSE);
	}

	const char *pos = str_str(string, tag);
	if (!pos)
	{
		return FALSE;
	}

	if (!need)
	{
		const size_t offset = pos - string;
		string[offset] = '\0';
		pos = str_str(pos + 1, tag);
		if (pos)
		{
			auto to_pos = string + offset;
			auto from_pos = pos + strlen(tag);
			while ((*(to_pos++) = *(from_pos++)));
		}
		return FALSE;
	}

	for (; *pos && *pos != '>'; pos++);

	if (*pos)
	{
		pos++;
	}

	if (*pos == 'R')
	{
		pos++;
		buf[0] = '\0';
	}

	strcat(buf, pos);
	pos = str_str(buf, tag);
	if (pos)
	{
		const size_t offset = pos - buf;
		buf[offset] = '\0';
	}

	return (TRUE);
}


void show_extend_room(const char * const description, CHAR_DATA * ch)
{
	int found = FALSE;
	char string[MAX_STRING_LENGTH], *pos;

	if (!description || !*description)
		return;

	strcpy(string, description);
	if ((pos = strchr(string, '<')))
		* pos = '\0';
	strcpy(buf, string);
	if (pos)
		*pos = '<';

	found = found || paste_description(string, TAG_WINTERNIGHT,
									   (weather_info.season == SEASON_WINTER
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_WINTERDAY,
									   (weather_info.season == SEASON_WINTER
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_SPRINGNIGHT,
									   (weather_info.season == SEASON_SPRING
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_SPRINGDAY,
									   (weather_info.season == SEASON_SPRING
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_SUMMERNIGHT,
									   (weather_info.season == SEASON_SUMMER
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_SUMMERDAY,
									   (weather_info.season == SEASON_SUMMER
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_AUTUMNNIGHT,
									   (weather_info.season == SEASON_AUTUMN
										&& (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
	found = found || paste_description(string, TAG_AUTUMNDAY,
									   (weather_info.season == SEASON_AUTUMN
										&& (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
	found = found || paste_description(string, TAG_NIGHT,
									   (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK));
	found = found || paste_description(string, TAG_DAY,
									   (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT));

	for (size_t i = strlen(buf); i > 0 && *(buf + i) == '\n'; i--)
	{
		*(buf + i) = '\0';
		if (i > 0 && *(buf + i) == '\r')
			*(buf + --i) = '\0';
	}

	send_to_char(buf, ch);
	send_to_char("\r\n", ch);
}

bool put_delim(std::stringstream &out, bool delim)
{
	if (!delim)
	{
		out << " (";
	}
	else
	{
		out << ", ";
	}
	return true;
}

void print_zone_info(CHAR_DATA *ch)
{
	zone_data *zone = &zone_table[world[ch->in_room]->zone];
	std::stringstream out;
	out << "\r\n" << zone->name;

	bool delim = false;
	if (!zone->is_town)
	{
		delim = put_delim(out, delim);
		out << "������� �������: " << zone->mob_level;
	}
	if (zone->group > 1)
	{
		delim = put_delim(out, delim);
		out << "��������� �� " << zone->group
			<< " " << desc_count(zone->group, WHAT_PEOPLE);
	}
	if (delim)
	{
		out << ")";
	}
	out << ".\r\n";

	send_to_char(out.str(), ch);
}

void show_glow_objs(CHAR_DATA *ch)
{
	unsigned cnt = 0;
	for (OBJ_DATA *obj = world[ch->in_room]->contents;
		obj; obj = obj->get_next_content())
	{
		if (obj->get_extra_flag(EExtraFlag::ITEM_GLOW))
		{
			++cnt;
			if (cnt > 1)
			{
				break;
			}
		}
	}
	if (!cnt) return;

	const char *str = cnt > 1 ?
		"�� ������ ��������� �����-�� ��������� ���������.\r\n" :
		"�� ������ ��������� ������-�� ���������� ��������.\r\n";
	send_to_char(str, ch);
}

void show_room_affects(CHAR_DATA* ch, const char* name_affects[], const char* name_self_affects[])
{
	bitvector_t bitvector = 0;
	std::ostringstream buffer;

	for (const auto& af : world[ch->in_room]->affected)
	{
		switch (af->bitvector)
		{
		case AFF_ROOM_LIGHT:					// 1 << 0
			if (!IS_SET(bitvector, AFF_ROOM_LIGHT))
			{
				if (af->caster_id == ch->id && *name_self_affects[0] != '\0')
				{
					buffer << name_self_affects[0] << "\r\n";
				}
				else if(*name_affects[0] != '\0')
				{
					buffer << name_affects[0] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_LIGHT);
			}
			break;
		case AFF_ROOM_FOG:						// 1 << 1
			if (!IS_SET(bitvector, AFF_ROOM_FOG))
			{
				if (af->caster_id == ch->id && *name_self_affects[1] != '\0')
				{
					buffer << name_self_affects[1] << "\r\n";
				}
				else if (*name_affects[1] != '\0')
				{
					buffer << name_affects[1] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_FOG);
			}
			break;
		case AFF_ROOM_RUNE_LABEL:				// 1 << 2
			if (af->caster_id == ch->id && *name_self_affects[2] != '\0')
			{
				buffer << name_self_affects[2] << "\r\n";
			}
			else if (*name_affects[2] != '\0')
			{
				buffer << name_affects[2] << "\r\n";
			}
			break;
		case AFF_ROOM_FORBIDDEN:				// 1 << 3
			if (!IS_SET(bitvector, AFF_ROOM_FORBIDDEN))
			{
				if (af->caster_id == ch->id && *name_self_affects[3] != '\0')
				{
					buffer << name_self_affects[3] << "\r\n";
				}
				else if (*name_affects[3] != '\0')
				{
					buffer << name_affects[3] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_FORBIDDEN);
			}
			break;
		case AFF_ROOM_HYPNOTIC_PATTERN:			// 1 << 4
			if (!IS_SET(bitvector, AFF_ROOM_HYPNOTIC_PATTERN))
			{
				if (af->caster_id == ch->id && *name_self_affects[4] != '\0')
				{
					buffer << name_self_affects[4] << "\r\n";
				}
				else if (*name_affects[4] != '\0')
				{
					buffer << name_affects[4] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_HYPNOTIC_PATTERN);
			}
			break;
		case AFF_ROOM_EVARDS_BLACK_TENTACLES:	// 1 << 5
			if (!IS_SET(bitvector, AFF_ROOM_EVARDS_BLACK_TENTACLES))
			{
				if (af->caster_id == ch->id && *name_self_affects[5] != '\0')
				{
					buffer << name_self_affects[5] << "\r\n";
				}
				else if (*name_affects[5] != '\0')
				{
					buffer << name_affects[5] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_EVARDS_BLACK_TENTACLES);
			}
			break;
		case AFF_ROOM_METEORSTORM:				// 1 << 6
			if (!IS_SET(bitvector, AFF_ROOM_METEORSTORM))
			{
				if (af->caster_id == ch->id && *name_self_affects[6] != '\0')
				{
					buffer << name_self_affects[6] << "\r\n";
				}
				else if (*name_affects[6] != '\0')
				{
					buffer << name_affects[6] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_METEORSTORM);
			}
			break;
		case AFF_ROOM_THUNDERSTORM:				// 1 << 7
			if (!IS_SET(bitvector, AFF_ROOM_THUNDERSTORM))
			{
				if (af->caster_id == ch->id && *name_self_affects[7] != '\0')
				{
					buffer << name_self_affects[7] << "\r\n";
				}
				else if (*name_affects[7] != '\0')
				{
					buffer << name_affects[7] << "\r\n";
				}

				SET_BIT(bitvector, AFF_ROOM_THUNDERSTORM);
			}
			break;
		default:
			log("SYSERR: Unknown room affect: %d", af->type);
		}
	}

	auto affects = buffer.str();
	if (!affects.empty())
	{
		affects.append("\r\n");
		send_to_char(affects.c_str(), ch);
	}
}

void look_at_room(CHAR_DATA * ch, int ignore_brief)
{
	if (!ch->desc)
		return;

	if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !can_use_feat(ch, DARK_READING_FEAT))
	{
		send_to_char("������� �����...\r\n", ch);
		show_glow_objs(ch);
		return;
	}
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("�� ��� ��� �����...\r\n", ch);
		return;
	}
	else if (GET_POS(ch) < POS_SLEEPING)
	{
		return;
	}

	if (PRF_FLAGGED(ch, PRF_DRAW_MAP) && !PRF_FLAGGED(ch, PRF_BLIND))
	{
		MapSystem::print_map(ch);
	}
	else if (ch->desc->snoop_by
		&& ch->desc->snoop_by->snoop_with_map
		&& ch->desc->snoop_by->character)
	{
		ch->map_print_to_snooper(ch->desc->snoop_by->character.get());
	}

	send_to_char(CCICYN(ch, C_NRM), ch);

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
	{
		// ����� ��������� * �� ������ ������ ����� ����
		const bool has_flag = ROOM_FLAGGED(ch->in_room, ROOM_BFS_MARK) ? true : false;
		GET_ROOM(ch->in_room)->unset_flag(ROOM_BFS_MARK);

		GET_ROOM(ch->in_room)->flags_sprint(buf, ";");
		sprintf(buf2, "[%5d] %s [%s]", GET_ROOM_VNUM(ch->in_room), world[ch->in_room]->name, buf);
		send_to_char(buf2, ch);

		if (has_flag)
		{
			GET_ROOM(ch->in_room)->set_flag(ROOM_BFS_MARK);
		}
	}
	else
	{
		if (PRF_FLAGGED(ch, PRF_MAPPER))
		{
			sprintf(buf2, "%s [%d]", world[ch->in_room]->name, GET_ROOM_VNUM(ch->in_room));
			send_to_char(buf2, ch);
		}
		else    
			send_to_char(world[ch->in_room]->name, ch);
	}

	send_to_char(CCNRM(ch, C_NRM), ch);
	send_to_char("\r\n", ch);

	if (IS_DARK(ch->in_room) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
	{
		send_to_char("������� �����...\r\n", ch);
	}
	else if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief || ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
	{
		show_extend_room(RoomDescription::show_desc(world[ch->in_room]->description_num).c_str(), ch);
	}

	// autoexits
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
	{
		do_auto_exits(ch);
	}

	// ���������� ������� �������. ����� ����������� ����� �� ������ ���������� ������.
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_MAGIC) || IS_IMMORTAL(ch))
	{
		show_room_affects(ch, room_aff_invis_bits, room_self_aff_invis_bits);
	}
	else
	{
		show_room_affects(ch, room_aff_visib_bits, room_aff_visib_bits);
	}

	// now list characters & objects
	if (world[ch->in_room]->fires)
	{
		sprintf(buf, "%s� ������ %s.%s\r\n",
				CCRED(ch, C_NRM), Fires[MIN(world[ch->in_room]->fires, MAX_FIRES - 1)], CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (world[ch->in_room]->portal_time)
	{
		if (world[ch->in_room]->pkPenterUnique)
		{
			sprintf(buf, "%s�������� ����������� %s� �������� ���������%s ���� �������� �����.%s\r\n",
				CCIBLU(ch, C_NRM), CCIRED(ch, C_NRM), CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		else
		{
			sprintf(buf, "%s�������� ����������� ���� �������� �����.%s\r\n",
				CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		}

		send_to_char(buf, ch);
	}

	if (world[ch->in_room]->holes)
	{
		const int ar = roundup(world[ch->in_room]->holes / HOLES_TIME);
		sprintf(buf, "%s����� �������� ���� �������� �������� � %i �����%s.%s\r\n",
			CCYEL(ch, C_NRM), ar, (ar == 1 ? "" : (ar < 5 ? "�" : "��")), (CCNRM(ch, C_NRM)));
		send_to_char(buf, ch);
	}

	if (ch->in_room != NOWHERE && !ROOM_FLAGGED(ch->in_room, ROOM_NOWEATHER))
	{
		*buf = '\0';
		switch (real_sector(ch->in_room))
		{
		case SECT_FIELD_SNOW:
		case SECT_FOREST_SNOW:
		case SECT_HILLS_SNOW:
		case SECT_MOUNTAIN_SNOW:
			sprintf(buf, "%s������� ����� ����� � ��� ��� ������.%s\r\n",
				CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_FIELD_RAIN:
		case SECT_FOREST_RAIN:
		case SECT_HILLS_RAIN:
			sprintf(buf, "%s�� ������ �������� � �����.%s\r\n", CCIWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_THICK_ICE:
			sprintf(buf, "%s� ��� ��� ������ ������� ���.%s\r\n", CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_NORMAL_ICE:
			sprintf(buf, "%s� ��� ��� ������ ���������� ������� ���.%s\r\n",
				CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		case SECT_THIN_ICE:
			sprintf(buf, "%s��������� ����� ���-��� ���������� ��� ����.%s\r\n",
				CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
			break;
		};
		if (*buf)
		{
			send_to_char(buf, ch);
		}
	}

	send_to_char("&Y&q", ch);
//  if (IS_SET(GET_SPELL_TYPE(ch, SPELL_TOWNPORTAL),SPELL_KNOW))
	if (ch->get_skill(SKILL_TOWNPORTAL))
	{
		if (find_portal_by_vnum(GET_ROOM_VNUM(ch->in_room)))
		{
			send_to_char("������ ������ � ������������ ����������� ������� ��������� �� �����.\r\n", ch);
		}
	}
	list_obj_to_char(world[ch->in_room]->contents, ch, 0, FALSE);
	list_char_to_char_thing(world[ch->in_room]->people, ch);  //������� ��������� ����� ���� ��� ���� ������� ������� ������
	send_to_char("&R&q", ch);
	list_char_to_char(world[ch->in_room]->people, ch);
	send_to_char("&Q&n", ch);

	// ���� � ����� ����
	if (zone_table[world[ch->get_from_room()]->zone].number != zone_table[world[ch->in_room]->zone].number)
	{
		if (PRF_FLAGGED(ch, PRF_ENTER_ZONE))
			print_zone_info(ch);
		++zone_table[world[ch->in_room]->zone].traffic;
		
	}
}

int get_pick_chance(int skill_pick, int lock_complexity)
{
	return (MIN(5, MAX(-5, skill_pick - lock_complexity)) + 5);
}

void look_in_direction(CHAR_DATA * ch, int dir, int info_is)
{
	int count = 0, probe, percent;
	ROOM_DATA::exit_data_ptr rdata;

	if (CAN_GO(ch, dir)
		|| (EXIT(ch, dir)
			&& EXIT(ch, dir)->to_room != NOWHERE))
	{
		rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", CCYEL(ch, C_NRM), Dirs[dir], CCNRM(ch, C_NRM));
		if (EXIT_FLAGGED(rdata, EX_CLOSED))
		{
			if (rdata->keyword)
			{
				count += sprintf(buf + count, " ������� (%s).\r\n", rdata->keyword);
			}
			else
			{
				count += sprintf(buf + count, " ������� (�������� �����).\r\n");
			}

			const int skill_pick = ch->get_skill(SKILL_PICK_LOCK) ;
			if (EXIT_FLAGGED(rdata, EX_LOCKED) && skill_pick)
			{
				if (EXIT_FLAGGED(rdata, EX_PICKPROOF))
				{
					count += sprintf(buf+count-2, "%s �� ������� �� ������� ��� ��������!%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
				}
				else if (EXIT_FLAGGED(rdata, EX_BROKEN))
				{
					count += sprintf(buf+count-2, "%s ����� ������... %s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
				}
				else
				{
					const int chance = get_pick_chance(skill_pick, rdata->lock_complexity);
					const int index = chance ? chance/5 + 1 : 0;

					std::string color = Locks[index][1];
					if (abs(skill_pick - rdata->lock_complexity)>10)
					{
						color = KIDRK;
					}
					if (COLOR_LEV(ch)>C_NRM)
					{
						count += sprintf(buf + count - 2, Locks[index][0], color.c_str(), KNRM);
					}
					else
					{
						count += sprintf(buf + count - 2, Locks[index][0], KNUL, KNUL);
					}
				}
			}

			send_to_char(buf, ch);
			return;
		}

		if (IS_TIMEDARK(rdata->to_room))
		{
			count += sprintf(buf + count, " ������� �����.\r\n");
			send_to_char(buf, ch);
			if (info_is & EXIT_SHOW_LOOKING)
			{
				send_to_char("&R&q", ch);
				count = 0;
				for (const auto tch : world[rdata->to_room]->people)
				{
					percent = number(1, skill_info[SKILL_LOOKING].max_percent);
					probe =
						train_skill(ch, SKILL_LOOKING, skill_info[SKILL_LOOKING].max_percent, tch);
					if (HERE(tch) && INVIS_OK(ch, tch) && probe >= percent
							&& (percent < 100 || IS_IMMORTAL(ch)))
					{
						// ���� ��� �� ���� � ��������� �� ��
						if ( GET_RACE(tch) != NPC_RACE_THING || IS_IMMORTAL(ch) ) {
							list_one_char(tch, ch, SKILL_LOOKING);
							count++;
						}
					}
				}

				if (!count)
				{
					send_to_char("�� ������ �� ������ ����������!\r\n", ch);
				}
				send_to_char("&Q&n", ch);
			}
		}
		else
		{
			if (!rdata->general_description.empty())
			{
				count += sprintf(buf + count, "%s\r\n", rdata->general_description.c_str());
			}
			else
			{
				count += sprintf(buf + count, "%s\r\n", world[rdata->to_room]->name);
			}
			send_to_char(buf, ch);
			send_to_char("&R&q", ch);
			list_char_to_char(world[rdata->to_room]->people, ch);
			send_to_char("&Q&n", ch);
		}
	}
	else if (info_is & EXIT_SHOW_WALL)
		send_to_char("� ��� �� ��� �������� �������?\r\n", ch);
}

void hear_in_direction(CHAR_DATA * ch, int dir, int info_is)
{
	int count = 0, percent = 0, probe = 0;
	int fight_count = 0;
	std::string tmpstr;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
	{
		send_to_char("�� ������, ��� �� �����?\r\n", ch);
		return;
	}
	if (CAN_GO(ch, dir)
		|| (EXIT(ch, dir)
		&& EXIT(ch, dir)->to_room != NOWHERE))
	{
		ROOM_DATA::exit_data_ptr rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", CCYEL(ch, C_NRM), Dirs[dir], CCNRM(ch, C_NRM));
		count += sprintf(buf + count, "\r\n%s", CCGRN(ch, C_NRM));
		send_to_char(buf, ch);
		count = 0;
		for (const auto tch : world[rdata->to_room]->people)
		{
			percent = number(1, skill_info[SKILL_HEARING].max_percent);
			probe = train_skill(ch, SKILL_HEARING, skill_info[SKILL_HEARING].max_percent, tch);
			// ���� ��������� �� ������ ������ ������.
			if (tch->get_fighting())
			{
				if (IS_NPC(tch))
				{
					tmpstr += " �� ������� ��� ����-�� ������.\r\n";
				}
				else
				{
					tmpstr += " �� ������� ����� ����-�� ������.\r\n";
				}
				fight_count++;
				continue;
			}

			if ((probe >= percent || ((!AFF_FLAGGED(tch, EAffectFlag::AFF_SNEAK) || !AFF_FLAGGED(tch, EAffectFlag::AFF_HIDE)) && (probe > percent * 2)))
					&& (percent < 100 || IS_IMMORTAL(ch))
					&& !fight_count)
			{
				if (IS_NPC(tch))
				{
					if (GET_RACE(tch)==NPC_RACE_THING) {
						if (GET_LEVEL(tch) < 5)
							tmpstr += " �� ������� ���-�� ����� �������������.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " �� ������� ���-�� �����.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " �� ������� ���-�� ������� �����.\r\n";
						else
							tmpstr += " �� ������� ���-�� ������� �����.\r\n";
					} 
					else if (real_sector(ch->in_room) != SECT_UNDERWATER)
					{
						if (GET_LEVEL(tch) < 5)
							tmpstr += " �� ������� ���-�� ����� �����.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " �� ������� ���-�� �������.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " �� ������� ���-�� ������� �������.\r\n";
						else
							tmpstr += " �� ������� ���-�� ������� �������.\r\n";
					}
					else
					{
						if (GET_LEVEL(tch) < 5)
							tmpstr += " �� ������� ����� ���������.\r\n";
						else if (GET_LEVEL(tch) < 15)
							tmpstr += " �� ������� ���������.\r\n";
						else if (GET_LEVEL(tch) < 25)
							tmpstr += " �� ������� ������� ���������.\r\n";
						else
							tmpstr += " �� ������� ������� ���������.\r\n";
					}
				}
				else
				{
					tmpstr += " �� ������� ���-�� �����������.\r\n";
				}
				count++;
			}
		}

		if ((!count) && (!fight_count))
		{
			send_to_char(" ������ � �����.\r\n", ch);
		}
		else
		{
			send_to_char(tmpstr.c_str(), ch);
		}

		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	else
	{
		if (info_is & EXIT_SHOW_WALL)
		{
			send_to_char("� ��� �� ��� ������ ��������?\r\n", ch);
		}
	}
}

void look_in_obj(CHAR_DATA * ch, char *arg)
{
	OBJ_DATA *obj = NULL;
	CHAR_DATA *dummy = NULL;
	char whatp[MAX_INPUT_LENGTH], where[MAX_INPUT_LENGTH];
	int amt, bits;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP;

	if (!*arg)
		send_to_char("�������� �� ���?\r\n", ch);
	else
		half_chop(arg, whatp, where);

	if (isname(where, "����� ������� room ground"))
		where_bits = FIND_OBJ_ROOM;
	else if (isname(where, "��������� inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "���������� equipment"))
		where_bits = FIND_OBJ_EQUIP;

	bits = generic_find(arg, where_bits, ch, &dummy, &obj);

	if ((obj == NULL) || !bits)
	{
		sprintf(buf, "�� �� ������ ����� '%s'.\r\n", arg);
		send_to_char(buf, ch);
	}
	else if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_DRINKCON
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_FOUNTAIN
		&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_CONTAINER)
	{
		send_to_char("������ � ��� ���!\r\n", ch);
	}
	else
	{
		if (Clan::ChestShow(obj, ch))
		{
			return;
		}
		if (ClanSystem::show_ingr_chest(obj, ch))
		{
			return;
		}
		if (Depot::is_depot(obj))
		{
			Depot::show_depot(ch);
			return;
		}

		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER)
		{
			if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
			{
				act("������$A.", FALSE, ch, obj, 0, TO_CHAR);
				const int skill_pick = ch->get_skill(SKILL_PICK_LOCK) ;
				int count = sprintf(buf, "������%s.", GET_OBJ_SUF_6(obj));
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED) && skill_pick)
				{
					if (OBJVAL_FLAGGED(obj, CONT_PICKPROOF))
						count += sprintf(buf+count, "%s �� ������� �� ������� ��� ��������!%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
					else if (OBJVAL_FLAGGED(obj, CONT_BROKEN))
						count += sprintf(buf+count, "%s ����� ������... %s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
					else
					{
						const int chance = get_pick_chance(skill_pick, GET_OBJ_VAL(obj, 3));
						const int index = chance ? chance/5 + 1 : 0;
						std::string color = Locks[index][1];
						if (abs(skill_pick - GET_OBJ_VAL(obj, 3))>10)
							color = KIDRK;

						if (COLOR_LEV(ch)>C_NRM)
							count += sprintf(buf + count, Locks[index][0], color.c_str(), KNRM);
						else
							count += sprintf(buf + count, Locks[index][0], KNUL, KNUL);
					}
					send_to_char(buf, ch);
				}
			}
			else
			{
				send_to_char(OBJN(obj, ch, 0), ch);
				switch (bits)
				{
				case FIND_OBJ_INV:
					send_to_char("(� �����)\r\n", ch);
					break;
				case FIND_OBJ_ROOM:
					send_to_char("(�� �����)\r\n", ch);
					break;
				case FIND_OBJ_EQUIP:
					send_to_char("(� ��������)\r\n", ch);
					break;
				}
				if (!obj->get_contains())
					send_to_char(" ������ ������ ���.\r\n", ch);
				else
				{
					if (GET_OBJ_VAL(obj, 0) > 0 && bits != FIND_OBJ_ROOM) {
						/* amt - ������ ������� �� 6 ��������� (0..5) � ��������� �������������
						   � ������� �������� ���. �������������� �� �������� ����������� ���� � ������������� ������ ����������,
						   ���������� ������� �� 0 �� 5. (������ 5 ����� ���� ��� ��������� ������ ����������)
						*/
						amt = MAX(0, MIN(5, (GET_OBJ_WEIGHT(obj) * 100) / (GET_OBJ_VAL(obj, 0) *  20)));
						//sprintf(buf, "DEBUG 1: %d 2: %d 3: %d.\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_VAL(obj, 0), amt);
						//send_to_char(buf, ch);
						sprintf(buf, "��������%s ���������� %s:\r\n", GET_OBJ_SUF_6(obj), fullness[amt]);
						send_to_char(buf, ch);
					}
					list_obj_to_char(obj->get_contains(), ch, 1, bits != FIND_OBJ_ROOM);
				}
			}
		}
		else  	// item must be a fountain or drink container
		{
			if (GET_OBJ_VAL(obj, 1) <= 0)
				send_to_char("�����.\r\n", ch);
			else
			{
				if (GET_OBJ_VAL(obj, 0) <= 0 || GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0))
				{
					sprintf(buf, "��������%s ��������?!\r\n", GET_OBJ_SUF_6(obj));	// BUG
				}
				else
				{
					const char* msg = AFF_FLAGGED(ch, EAffectFlag::AFF_DETECT_POISON)
						&& obj->get_val(3) == 1 ? "(�����������)" : "";
					amt = (GET_OBJ_VAL(obj, 1) * 5) / GET_OBJ_VAL(obj, 0);
					sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
					sprintf(buf, "��������%s %s%s%s ���������.\r\n", GET_OBJ_SUF_6(obj), fullness[amt], buf2, msg);
				}
				send_to_char(buf, ch);
			}
		}
	}
}

char *find_exdesc(char *word, const EXTRA_DESCR_DATA::shared_ptr& list)
{
	for (auto i = list; i; i = i->next)
	{
		if (isname(word, i->keyword))
		{
			return i->description;
		}
	}

	return nullptr;
}
const char *diag_liquid_timer(const OBJ_DATA* obj)
{	int tm;
	if (GET_OBJ_VAL(obj, 3) == 1)
		return "�����������!";
	if (GET_OBJ_VAL(obj, 3) == 0)
		return "���������.";
	tm = (GET_OBJ_VAL(obj, 3));
	if (tm < 1440) // �����
		return "����� ����������!";
	else if (tm < 10080) //������
		return "������������.";
	else if (tm < 20160) // 2 ������
		return "�������� ������.";
	else if (tm < 30240) // 3 ������
		return "������.";
	return "���������.";
}

//�-��� ������ ��� ���� �� �������
//buf ��� ����� � ������� ���������� ����, � ��� ��� ����� ���� ���-�� ����� ���� ����� ������� ��������� *buf='\0'
void obj_info(CHAR_DATA * ch, OBJ_DATA *obj, char buf[MAX_STRING_LENGTH])
{
	int j;
		if (can_use_feat(ch, SKILLED_TRADER_FEAT) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)|| ch->get_skill(SKILL_INSERTGEM))
		{
			sprintf(buf+strlen(buf), "�������� : %s", CCCYN(ch, C_NRM));
			sprinttype(obj->get_material(), material_name, buf+strlen(buf));
			sprintf(buf+strlen(buf), "\r\n%s", CCNRM(ch, C_NRM));
		}

		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_MING
			&& (can_use_feat(ch, BREW_POTION_FEAT)
				|| PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
		{
			for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;)
			{
				j++;
			}
			sprintf(buf+strlen(buf), "��� ���������� ���� '%s'.\r\n", imtypes[j].name);
			const int imquality = GET_OBJ_VAL(obj, IM_POWER_SLOT);
			if (GET_LEVEL(ch) >= imquality)
			{
				sprintf(buf+strlen(buf), "�������� ����������� ");
				if (imquality > 25)
					strcat(buf+strlen(buf), "���������.\r\n");
				else if (imquality > 20)
					strcat(buf+strlen(buf), "��������.\r\n");
				else if (imquality > 15)
					strcat(buf+strlen(buf), "����� �������.\r\n");
				else if (imquality > 10)
					strcat(buf+strlen(buf), "���� ��������.\r\n");
				else if (imquality > 5)
					strcat(buf+strlen(buf), "������ ��������������.\r\n");
				else
					strcat(buf+strlen(buf), "���� �� ������.\r\n");
			}
			else
			{
				strcat(buf+strlen(buf), "�� �� � ��������� ���������� �������� ����� �����������.\r\n");
			}
		}

 		//|| PRF_FLAGGED(ch, PRF_HOLYLIGHT)
		if (can_use_feat(ch, MASTER_JEWELER_FEAT))
		{
			sprintf(buf+strlen(buf), "����� : %s", CCCYN(ch, C_NRM));
			if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WITH3SLOTS))
			{
				strcat(buf, "�������� 3 �����\r\n");
			}
			else if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WITH2SLOTS))
			{
				strcat(buf, "�������� 2 �����\r\n");
			}
			else if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WITH1SLOT))
			{
				strcat(buf, "�������� 1 ����\r\n");
			}
			else
			{
				strcat(buf, "��� ������\r\n");
			}
			sprintf(buf+strlen(buf), "\r\n%s", CCNRM(ch, C_NRM));
		}
		if (AUTH_CUSTOM_LABEL(obj, ch) && obj->get_custom_label()->label_text)
		{
			if (obj->get_custom_label()->clan)
			{
				strcat(buf, "����� �������: ");
			}
			else
			{
				strcat(buf, "���� �����: ");
			}
			sprintf(buf + strlen(buf), "%s\r\n", obj->get_custom_label()->label_text);
		}
		sprintf(buf+strlen(buf), "%s", diag_uses_to_char(obj, ch));
		sprintf(buf+strlen(buf), "%s", diag_shot_to_char(obj, ch));
		if (GET_OBJ_VNUM(obj) >= DUPLICATE_MINI_SET_VNUM)
		{
			sprintf(buf + strlen(buf), "�������� ����� �������.\r\n");
		}

		if (((GET_OBJ_TYPE(obj) == CObjectPrototype::ITEM_DRINKCON)
			&& (GET_OBJ_VAL(obj, 1) > 0))
			|| (GET_OBJ_TYPE(obj) == CObjectPrototype::ITEM_FOOD))
		{
			sprintf(buf1, "��������: %s\r\n", diag_liquid_timer(obj));
			strcat(buf, buf1);
		}
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 * \return ���� ���� ������� � ����-������, ����� ����� ������� �� �������� ������ ��� �� look_in_obj
 */
bool look_at_target(CHAR_DATA * ch, char *arg, int subcmd)
{
	int bits, found = FALSE, fnum, i = 0, cn = 0;
	struct portals_list_type *port;
	CHAR_DATA *found_char = NULL;
	OBJ_DATA *found_obj = NULL;
	struct char_portal_type *tmp;
	char *desc, *what, whatp[MAX_INPUT_LENGTH], where[MAX_INPUT_LENGTH];
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM | FIND_OBJ_EXDESC;

	if (!ch->desc)
	{
		return false;
	}

	if (!*arg)
	{
		send_to_char("�� ��� �� ��� �������� ����������?\r\n", ch);
		return false;
	}

	half_chop(arg, whatp, where);
	what = whatp;

	if (isname(where, "����� ������� room ground"))
		where_bits = FIND_OBJ_ROOM | FIND_CHAR_ROOM;
	else if (isname(where, "��������� inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "���������� equipment"))
		where_bits = FIND_OBJ_EQUIP;

	// ��� townportal
	if (isname(whatp, "������") &&
//       IS_SET(GET_SPELL_TYPE(ch, SPELL_TOWNPORTAL), SPELL_KNOW) &&
			ch->get_skill(SKILL_TOWNPORTAL) &&
			(port = get_portal(GET_ROOM_VNUM(ch->in_room), NULL)) != NULL && IS_SET(where_bits, FIND_OBJ_ROOM))
	{
		if (GET_LEVEL(ch) < MAX(1, port->level - GET_REMORT(ch) / 2))
		{
			send_to_char("�� ����� ���-�� �������� ��������� �������.\r\n", ch);
			send_to_char("�� �� ��� ������������ �������, ����� ��������� �����.\r\n", ch);
			return false;
		}
		else
		{
			for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next)
			{
				cn++;
			}
			if (cn >= MAX_PORTALS(ch))
			{
				send_to_char
				("��� ��������� ��� ����� ��� ���������, ������� � ���������� ���.\r\n", ch);
				return false;
			}
			send_to_char("�� ����� ��������� ������� �������� ����� '&R", ch);
			send_to_char(port->wrd, ch);
			send_to_char("&n'.\r\n", ch);
			// ������ ��������� � ������ ����
			add_portal_to_char(ch, GET_ROOM_VNUM(ch->in_room));
			check_portals(ch);
			return false;
		}
	}

	// ��������� � �����������
	if (isname(whatp, "�����������") && world[ch->in_room]->portal_time && IS_SET(where_bits, FIND_OBJ_ROOM))
	{
		const int r = ch->in_room;
		const auto to_room = world[r]->portal_room;
		send_to_char("������������� � �����������, �� ��������� ��������� � ���.\r\n\r\n", ch);
		act("$n0 ��������� ��������$g � �����������.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		if (world[to_room]->portal_time && (r == world[to_room]->portal_room))
		{
			send_to_char
			("����� ����, ������ � ���������������� ����� �������, ��������� ��� �����.\r\n\r\n", ch);
			return false;
		}
		ch->in_room = world[ch->in_room]->portal_room;
		look_at_room(ch, 1);
		ch->in_room = r;
		return false;
	}

	bits = generic_find(what, where_bits, ch, &found_char, &found_obj);
	// Is the target a character?
	if (found_char != NULL)
	{
		if (subcmd == SCMD_LOOK_HIDE && !check_moves(ch, LOOKHIDE_MOVES))
			return false;
		look_at_char(found_char, ch);
		if (ch != found_char)
		{
			if (subcmd == SCMD_LOOK_HIDE && ch->get_skill(SKILL_LOOK_HIDE) > 0)
			{
				fnum = number(1, skill_info[SKILL_LOOK_HIDE].max_percent);
				found =
					train_skill(ch, SKILL_LOOK_HIDE,
								skill_info[SKILL_LOOK_HIDE].max_percent, found_char);
				if (!WAITLESS(ch))
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				if (found >= fnum && (fnum < 100 || IS_IMMORTAL(ch)) && !IS_IMMORTAL(found_char))
					return false;
			}
			if (CAN_SEE(found_char, ch))
				act("$n �������$g ��� � ������ �� ���.", TRUE, ch, 0, found_char, TO_VICT);
			act("$n ���������$g �� $N3.", TRUE, ch, 0, found_char, TO_NOTVICT);
		}
		return false;
	}

	// Strip off "number." from 2.foo and friends.
	if (!(fnum = get_number(&what)))
	{
		send_to_char("��� �����������?\r\n", ch);
		return false;
	}

	// Does the argument match an extra desc in the room?
	if ((desc = find_exdesc(what, world[ch->in_room]->ex_description)) != NULL && ++i == fnum)
	{
		page_string(ch->desc, desc, FALSE);
		return false;
	}

	// If an object was found back in generic_find
	if (bits && (found_obj != NULL))
	{

		if (Clan::ChestShow(found_obj, ch))
		{
			return true;
		}
		if (ClanSystem::show_ingr_chest(found_obj, ch))
		{
			return true;
		}
		if (Depot::is_depot(found_obj))
		{
			Depot::show_depot(ch);
			return true;
		}

		// ���������� ���������. ������ �������� "if (!found)" ������� ��������
		// ������� �������� � �������, ���������� �������� "generic_find"
		if (!(desc = find_exdesc(what, found_obj->get_ex_description())))
		{
			show_obj_to_char(found_obj, ch, 5, TRUE, 1);	// Show no-description
		}
		else
		{
			send_to_char(desc, ch);
			show_obj_to_char(found_obj, ch, 6, TRUE, 1);	// Find hum, glow etc
		}

		*buf = '\0';
		obj_info(ch, found_obj, buf);
		send_to_char(buf, ch);
	}
	else
		send_to_char("������, ����� ����� ���!\r\n", ch);

	return false;
}


void skip_hide_on_look(CHAR_DATA * ch)
{

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE) &&
			((!ch->get_skill(SKILL_LOOK_HIDE) ||
			  ((number(1, 100) -
				calculate_skill(ch, SKILL_LOOK_HIDE, 0) - 2 * (ch->get_wis() - 9)) > 0))))
	{
		affect_from_char(ch, SPELL_HIDE);
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE))
		{
			send_to_char("�� ���������� ���������.\r\n", ch);
			act("$n ���������$g ���������.", FALSE, ch, 0, 0, TO_ROOM);
		}
	}
	return;
}

void do_look(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	char arg2[MAX_INPUT_LENGTH];
	int look_type;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
	{
		send_to_char("������� ����� ��� �����������...\r\n", ch);
	}
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("�� ���������!\r\n", ch);
	}
	else if (is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch))
	{
		if (GET_LEVEL(ch) > 30)
		{
			sprintf(buf,
				"%s�������=%s%d %s����=%s%d %s�����=%s%d %s������=%s%d %s���=%s%d "
				"%s����=%s%d %s������=%s%d %s����=%s%d %s����=%s%d%s.\r\n",
				CCNRM(ch, C_NRM), CCINRM(ch, C_NRM), ch->in_room,
				CCRED(ch, C_NRM), CCIRED(ch, C_NRM), world[ch->in_room]->light,
				CCGRN(ch, C_NRM), CCIGRN(ch, C_NRM), world[ch->in_room]->glight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[ch->in_room]->fires,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), world[ch->in_room]->ices,
				CCBLU(ch, C_NRM), CCIBLU(ch, C_NRM), world[ch->in_room]->gdark,
				CCMAG(ch, C_NRM), CCICYN(ch, C_NRM), weather_info.sky,
				CCWHT(ch, C_NRM), CCIWHT(ch, C_NRM), weather_info.sunlight,
				CCYEL(ch, C_NRM), CCIYEL(ch, C_NRM), weather_info.moon_day, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		skip_hide_on_look(ch);

		send_to_char("������� �����...\r\n", ch);
		list_char_to_char(world[ch->in_room]->people, ch);	// glowing red eyes
		show_glow_objs(ch);
	}
	else
	{
		half_chop(argument, arg, arg2);

		skip_hide_on_look(ch);

		if (subcmd == SCMD_READ)
		{
			if (!*arg)
				send_to_char("��� �� ������ ���������?\r\n", ch);
			else
				look_at_target(ch, arg, subcmd);
			return;
		}
		if (!*arg)	// "look" alone, without an argument at all
			look_at_room(ch, 1);
		else if (is_abbrev(arg, "in") || is_abbrev(arg, "������"))
			look_in_obj(ch, arg2);
		// did the char type 'look <direction>?'
		else if (((look_type = search_block(arg, dirs, FALSE)) >= 0) ||
				 ((look_type = search_block(arg, Dirs, FALSE)) >= 0))
			look_in_direction(ch, look_type, EXIT_SHOW_WALL);
		else if (is_abbrev(arg, "at") || is_abbrev(arg, "��"))
			look_at_target(ch, arg2, subcmd);
		else
			look_at_target(ch, argument, subcmd);
	}
}

void do_sides(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;

	if (!ch->desc)
		return;

	if (GET_POS(ch) <= POS_SLEEPING)
		send_to_char("������� ����� ��� �����������...\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		send_to_char("�� ���������!\r\n", ch);
	else
	{
		skip_hide_on_look(ch);
		send_to_char("�� ���������� �� ��������.\r\n", ch);
		for (i = 0; i < NUM_OF_DIRS; i++)
		{
			look_in_direction(ch, i, 0);
		}
	}
}

void do_looking(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("����� ����� ������ ����� ����, ������ ��������� ��������.\r\n", ch);
	if (GET_POS(ch) == POS_SLEEPING)
		send_to_char("������� ����� ��� �����������...\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		send_to_char("�� ���������!\r\n", ch);
	else if (ch->get_skill(SKILL_LOOKING))
	{
		if (check_moves(ch, LOOKING_MOVES))
		{
			send_to_char("�� �������� ������ � ������ ��������������� �� ��������.\r\n", ch);
			for (i = 0; i < NUM_OF_DIRS; i++)
				look_in_direction(ch, i, EXIT_SHOW_LOOKING);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
	}
	else
		send_to_char("��� ���� �� ������� ����� ������.\r\n", ch);
}

void do_hearing(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;

	if (!ch->desc)
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
	{
		send_to_char("�� ����� � ��� ����� ������ �� ��������.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("��� ������ ��������� ������ �������, ������� ��� � ����.\r\n", ch);
	if (GET_POS(ch) == POS_SLEEPING)
		send_to_char("������ �������� ��������� ������ ����� �� ������� � ������� �����������.\r\n", ch);
	else if (ch->get_skill(SKILL_HEARING))
	{
		if (check_moves(ch, HEARING_MOVES))
		{
			send_to_char("�� ������ �������������� ��������������.\r\n", ch);
			for (i = 0; i < NUM_OF_DIRS; i++)
				hear_in_direction(ch, i, 0);
			if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}
	}
	else
		send_to_char("������� ������� ��� ��� ������� ������.\r\n", ch);
}

void do_examine(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	CHAR_DATA *tmp_char;
	OBJ_DATA *tmp_object;
	char where[MAX_INPUT_LENGTH];
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM | FIND_OBJ_EXDESC;


	if (GET_POS(ch) < POS_SLEEPING)
	{
		send_to_char("������� ����� ��� �����������...\r\n", ch);
		return;
	}
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("�� ���������!\r\n", ch);
		return;
	}

	two_arguments(argument, arg, where);

	if (!*arg)
	{
		send_to_char("��� �� ������� ���������?\r\n", ch);
		return;
	}

	if (isname(where, "����� ������� room ground"))
		where_bits = FIND_OBJ_ROOM | FIND_CHAR_ROOM;
	else if (isname(where, "��������� inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(where, "���������� equipment"))
		where_bits = FIND_OBJ_EQUIP;

	skip_hide_on_look(ch);

	if (look_at_target(ch, argument, subcmd))
		return;

	if (isname(arg, "�����������") && world[ch->in_room]->portal_time && IS_SET(where_bits, FIND_OBJ_ROOM))
		return;

	if (isname(arg, "������") &&
			ch->get_skill(SKILL_TOWNPORTAL) &&
			(get_portal(GET_ROOM_VNUM(ch->in_room), NULL)) != NULL && IS_SET(where_bits, FIND_OBJ_ROOM))
		return;

	generic_find(arg, where_bits, ch, &tmp_char, &tmp_object);
	if (tmp_object)
	{
		if (GET_OBJ_TYPE(tmp_object) == OBJ_DATA::ITEM_DRINKCON
			|| GET_OBJ_TYPE(tmp_object) == OBJ_DATA::ITEM_FOUNTAIN
			|| GET_OBJ_TYPE(tmp_object) == OBJ_DATA::ITEM_CONTAINER)
		{
			look_in_obj(ch, argument);
		}
	}
}

void do_gold(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int count = 0;
	if (ch->get_gold() == 0)
		send_to_char("�� ��������!\r\n", ch);
	else if (ch->get_gold() == 1)
		send_to_char("� ��� ���� ����� ���� ���� ����.\r\n", ch);
	else
	{
		count += sprintf(buf, "� ��� ���� %ld %s.\r\n", ch->get_gold(), desc_count(ch->get_gold(), WHAT_MONEYa));
		send_to_char(buf, ch);
	}
}

/// �� pc_class_name
const char *class_name[] = { "������",
							 "������",
							 "����",
							 "��������",
							 "�������",
							 "���������",
							 "��������",
							 "���������",
							 "������������",
							 "������",
							 "�������",
							 "������",
							 "�����",
							 "�����",
							 "����",
							 "�����",
							 "�����",
							 "�������",
							 "�������",
							 "�������",
							 "������",
							 "��������",
							 "����",
							 "�����",
							 "������",
							 "�����",
							 "������",
							 "������",
							 "�������",
							 "�����",
							 "�����",
							 "�����",
							 "�������",
							 "����",
							 "����������",
							 "�������",
							 "���������",
							 "�����",
							 "�������",
							 "������",
							 "������",
							 "����"
						   };


const char *ac_text[] =
{
	"&W�� �������� ��� ���",	//  -30
	"&W�� �������� ��� ���",	//  -29
	"&W�� �������� ��� ���",	//  -28
	"&g�� �������� ����� ��� ���",	//  -27
	"&g�� �������� ����� ��� ���",	//  -26
	"&g�� �������� ����� ��� ���",	//  -25
	"&g��������� ������",	//  -24
	"&g��������� ������",	//  -23
	"&g��������� ������",	//  -22
	"&g������������ ������",	//  -21
	"&g������������ ������",	//  -20
	"&g������������ ������",	//  -19
	"&g�������� ������",	//  -18
	"&g�������� ������",	//  -17
	"&g�������� ������",	//  -16
	"&G����� ������� ������",	//  -15
	"&G����� ������� ������",	//  -14
	"&G����� ������� ������",	//  -13
	"&G������ ������� ������",	//  -12
	"&G������ ������� ������",	//  -11
	"&G������ ������� ������",	//  -10
	"&G������� ������",	//   -9
	"&G������� ������",	//   -8
	"&G������� ������",	//   -7
	"&G�������� ������",	//   -6
	"&G�������� ������",	//   -5
	"&G�������� ������",	//   -4
	"&Y������ ���� ���� ��������",	//   -3
	"&Y������ ���� ���� ��������",	//   -2
	"&Y������ ���� ���� ��������",	//   -1
	"&Y������� ������",	//    0
	"&Y������ ���� ���� ��������",
	"&Y������ ������",
	"&R������ ������",
	"&R����� ������ ������",
	"&R�� ������� ��������",	// 5
	"&R�� ������ ������� ��������",
	"&r�� ����-���� ��������",
	"&r�� ����� �������",
	"&r�� ����� ��������� �������",
	"&r�� ��������� �������",	// 10
};

void print_do_score_all(CHAR_DATA *ch)
{
	int ac, max_dam = 0, hr = 0, resist, modi = 0, timer_room_label;
	ESkill skill = SKILL_BOTHHANDS;

	std::string sum = std::string("�� ") + std::string(ch->get_name()) + std::string(", ")
		+ std::string(class_name[(int)GET_CLASS(ch) + 14 * GET_KIN(ch)]) + std::string(".");

	sprintf(buf,
		" %s-------------------------------------------------------------------------------------\r\n"
		" || %s%-80s%s||\r\n"
		" -------------------------------------------------------------------------------------\r\n",
		CCCYN(ch, C_NRM),
		CCNRM(ch, C_NRM), sum.substr(0, 80).c_str(), CCCYN(ch, C_NRM));

	sprintf(buf + strlen(buf),
		" || %s�����: %-11s %s|"
		" %s����:        %-3d(%-3d) %s|"
		" %s�����:       %4d %s|"
		" %s�������������: %s||\r\n",
		CCNRM(ch, C_NRM),
		std::string(PlayerRace::GetKinNameByNum(GET_KIN(ch), GET_SEX(ch))).substr(0, 14).c_str(),
		CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), GET_ARMOUR(ch), CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), CCCYN(ch, C_NRM));

	ac = compute_armor_class(ch) / 10;
	if (ac < 5) {
		const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
		ac = ac + mod > 5 ? 5 : ac + mod;
	}
	resist = MIN(GET_RESIST(ch, FIRE_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
		" || %s���: %-13s %s|"
		" %s���:         %3d(%3d) %s|"
		" %s������:       %3d %s|"
		" %s����:      %3d %s||\r\n",
		CCNRM(ch, C_NRM),
		std::string(PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), GET_SEX(ch))).substr(0, 14).c_str(),
		CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), ac, CCCYN(ch, C_NRM),
		CCIRED(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, AIR_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
		" || %s����: %-13s%s|"
		" %s������:      %3d(%3d) %s|"
		" %s����������:   %3d %s|"
		" %s�������:   %3d %s||\r\n",
		CCNRM(ch, C_NRM),
		std::string(religion_name[GET_RELIGION(ch)][(int)GET_SEX(ch)]).substr(0, 13).c_str(),
		CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_SIZE(ch), GET_REAL_SIZE(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), GET_ABSORBE(ch), CCCYN(ch, C_NRM),
		CCWHT(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	if (can_use_feat(ch, SHOT_FINESSE_FEAT)) //������ ������� ���� �� �����
		max_dam = GET_REAL_DR(ch) + str_bonus(GET_REAL_DEX(ch), STR_TO_DAM);
	else
		max_dam = GET_REAL_DR(ch) + str_bonus(GET_REAL_STR(ch), STR_TO_DAM);

	if (can_use_feat(ch, BULLY_FEAT))
	{
		modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? MIN(GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)), 18) : 0));
		//modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)) : 0));
		modi = MAX(100, modi);
		max_dam += modi * max_dam / 50;
		max_dam += MAX(0, GET_REAL_STR(ch) - 25);
	}
	else
	{
		max_dam += 6 + 2 * GET_LEVEL(ch) / 3;
	}

	OBJ_DATA* weapon = GET_EQ(ch, WEAR_BOTHS);
	if (weapon)
	{
		if (GET_OBJ_TYPE(weapon) == OBJ_DATA::ITEM_WEAPON)
		{
			max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1);
			skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
			if (ch->get_skill(skill) == SKILL_INVALID)
			{
				hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
				max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			}
			else
			{
				apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
			}
		}
	}
	else
	{
		weapon = GET_EQ(ch, WEAR_WIELD);
		if (weapon)
		{
			if (GET_OBJ_TYPE(weapon) == OBJ_DATA::ITEM_WEAPON)
			{
				max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1) / 2;
				skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
				if (ch->get_skill(skill) == SKILL_INVALID)
				{
					hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
					max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
				}
				else
				{
					apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
				}
			}
		}

		weapon = GET_EQ(ch, WEAR_HOLD);
		if (weapon)
		{
			if (GET_OBJ_TYPE(weapon) == OBJ_DATA::ITEM_WEAPON)
			{
				max_dam += GET_OBJ_VAL(weapon, 1) * (GET_OBJ_VAL(weapon, 2) + 1) / 2;
				skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
				if (ch->get_skill(skill) == SKILL_INVALID)
				{
					hr -= (50 - MIN(50, GET_REAL_INT(ch))) / 3;
					max_dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
				}
				else
				{
					apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
				}
			}
		}
	}

	if (can_use_feat(ch, WEAPON_FINESSE_FEAT))
	{
		if (weapon && GET_OBJ_WEIGHT(weapon) > 20)
		{
			hr += str_bonus(GET_REAL_STR(ch), STR_TO_HIT);
		}
		else
		{
			hr += str_bonus(GET_REAL_DEX(ch), STR_TO_HIT);
		}
	}
	else
	{
		hr += str_bonus(GET_REAL_STR(ch), STR_TO_HIT);
	}
	hr += GET_REAL_HR(ch) - thaco((int)GET_CLASS(ch), (int)GET_LEVEL(ch));
	if (PRF_FLAGGED(ch, PRF_POWERATTACK)) {
		hr -= 2;
		max_dam += 5;
	}
	if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK)) {
		hr -= 4;
		max_dam += 10;
	}
	if (PRF_FLAGGED(ch, PRF_AIMINGATTACK)) {
		hr += 2;
		max_dam -= 5;
	}
	if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK)) {
		hr += 4;
		max_dam -= 10;
	}

	max_dam += ch->obj_bonus().calc_phys_dmg(max_dam);
	max_dam = MAX(0, max_dam);
	max_dam *= ch->get_cond_penalty(P_DAMROLL);

	if (hr)
		hr *= ch->get_cond_penalty(P_HITROLL);

	resist = MIN(GET_RESIST(ch, WATER_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
		" || %s�������: %s%-2d        %s|"
		" %s����:          %2d(%2d) %s|"
		" %s�����:        %3d %s|"
		" %s����:      %3d %s||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_LEVEL(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), ch->get_str(), GET_REAL_STR(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), hr - (on_horse(ch) ? (10 - GET_SKILL(ch, SKILL_HORSE) / 20) : 0), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, EARTH_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
		" || %s��������������: %s%-2d %s|"
		" %s��������:      %2d(%2d) %s|"
		" %s����:        %4d %s|"
		" %s�����:     %3d %s||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_REMORT(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), ch->get_dex(), GET_REAL_DEX(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), int(max_dam * (on_horse(ch) ? ((GET_SKILL(ch, SKILL_HORSE) > 100) ? (1 + (GET_SKILL(ch, SKILL_HORSE) - 100) / 500.0) : 1) : 1)), CCCYN(ch, C_NRM),
		CCYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	resist = GET_RESIST(ch, DARK_RESISTANCE);
	sprintf(buf + strlen(buf),
		" || %s�������: %s%-3d       %s|"
		" %s������������:  %2d(%2d) %s|-------------------| &K����:      %3d&c ||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_AGE(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), ch->get_con(), GET_REAL_CON(ch), CCCYN(ch, C_NRM),
		resist);
	resist = MIN(GET_RESIST(ch, VITALITY_RESISTANCE), 75);
	const int rcast = GET_CAST_SUCCESS(ch) * ch->get_cond_penalty(P_CAST);
	sprintf(buf + strlen(buf),
		" || %s����: %s%-10ld   %s|"
		" %s��������:      %2d(%2d) %s|"
		" %s����������:   %3d %s|"
		"&c----------------||\r\n",
		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), GET_EXP(ch), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), ch->get_wis(), GET_REAL_WIS(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), rcast, CCCYN(ch, C_NRM));

	resist = MIN(GET_RESIST(ch, VITALITY_RESISTANCE), 75);

	if (IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), " || %s���: %s1%s             |",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf + strlen(buf),
			" || %s���: %s%-10ld    %s|",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch), CCCYN(ch, C_NRM));
	int itmp = GET_MANAREG(ch);
	itmp *= ch->get_cond_penalty(P_CAST);
	sprintf(buf + strlen(buf),
		" %s��:            %2d(%2d) %s|"
		" %s�����������: %4d %s|"
		" %s���������: %3d %s||\r\n",

		CCICYN(ch, C_NRM), ch->get_int(), GET_REAL_INT(ch), CCCYN(ch, C_NRM),
		CCIGRN(ch, C_NRM), itmp, CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));
	resist = MIN(GET_RESIST(ch, MIND_RESISTANCE), 75);

	sprintf(buf + strlen(buf),
		" || %s�����: %s%-8ld    %s|"
		" %s�������:       %2d(%2d) %s|-------------------|"
		" %s�����:     %3d %s||\r\n",

		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), ch->get_gold(), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), ch->get_cha(), GET_REAL_CHA(ch), CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));
	resist = MIN(GET_RESIST(ch, IMMUNITY_RESISTANCE), 75);
	sprintf(buf + strlen(buf),
		" || %s�� �����: %s%-8ld %s|"
		" %s�����:     %4d(%4d) %s|"
		" %s����:         %3d%s |"
		" %s���������: %3d %s||\r\n",

		CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), ch->get_bank(), CCCYN(ch, C_NRM),
		CCICYN(ch, C_NRM), GET_HIT(ch), GET_REAL_MAX_HIT(ch), CCCYN(ch, C_NRM),
		CCGRN(ch, C_NRM), GET_REAL_SAVING_WILL(ch), CCCYN(ch, C_NRM),
		CCIYEL(ch, C_NRM), resist, CCCYN(ch, C_NRM));

	if (!on_horse(ch))
		switch (GET_POS(ch))
		{
		case POS_DEAD:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCIRED(ch, C_NRM), std::string("�� ������!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_MORTALLYW:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCIRED(ch, C_NRM), std::string("�� ��������!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_INCAP:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCRED(ch, C_NRM), std::string("�� ��� ��������.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_STUNNED:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCIYEL(ch, C_NRM), std::string("�� � ��������!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_SLEEPING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCIGRN(ch, C_NRM), std::string("�� �����.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_RESTING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCGRN(ch, C_NRM), std::string("�� ���������.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_SITTING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCIGRN(ch, C_NRM), std::string("�� ������.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_FIGHTING:
			if (ch->get_fighting())
				sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCIRED(ch, C_NRM), std::string("�� ����������!").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			else
				sprintf(buf + strlen(buf), " || %s%-19s%s|",
					CCRED(ch, C_NRM), std::string("�� ������ ��������.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		case POS_STANDING:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCNRM(ch, C_NRM), std::string("�� ������.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		default:
			sprintf(buf + strlen(buf), " || %s%-19s%s|",
				CCNRM(ch, C_NRM), std::string("You are floating..").substr(0, 19).c_str(), CCCYN(ch, C_NRM));
			break;
		}
	else
		sprintf(buf + strlen(buf), " || %s%-19s%s|",
			CCNRM(ch, C_NRM), std::string("�� ������ ������.").substr(0, 19).c_str(), CCCYN(ch, C_NRM));

	sprintf(buf + strlen(buf),
		" %s������.:     %3d(%3d) %s|"
		" %s��������:     %3d %s|"
		"----------------||\r\n",

		CCICYN(ch, C_NRM), GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), CCCYN(ch, C_NRM),
		CCGRN(ch, C_NRM), GET_REAL_SAVING_CRITICAL(ch), CCCYN(ch, C_NRM));

	if (GET_COND(ch, FULL) > NORM_COND_VALUE)
		sprintf(buf + strlen(buf), " || %s�������: %s��� :(%s    |", CCNRM(ch, C_NRM), CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf + strlen(buf), " || %s�������: %s���%s       |", CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (IS_MANA_CASTER(ch))
		sprintf(buf + strlen(buf),
			" %s���. ����: %4d(%4d) %s|",
			CCICYN(ch, C_NRM), GET_MANA_STORED(ch), GET_MAX_MANA(ch), CCCYN(ch, C_NRM));
	else
		strcat(buf, "                       |");

	sprintf(buf + strlen(buf),
		" %s���������:    %3d %s|"
		" &r�����. �����:  &c||\r\n",
		CCGRN(ch, C_NRM), GET_REAL_SAVING_STABILITY(ch), CCCYN(ch, C_NRM));

	if (GET_COND_M(ch, THIRST))
		sprintf(buf + strlen(buf),
			" || %s�����: %s�������!%s    |",
			CCNRM(ch, C_NRM), CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	else
		sprintf(buf + strlen(buf),
			" || %s�����: %s���%s         |",
			CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (IS_MANA_CASTER(ch))
		sprintf(buf + strlen(buf),
			" %s�������.:    %3d ���. %s|",
			CCICYN(ch, C_NRM), mana_gain(ch), CCCYN(ch, C_NRM));
	else
		strcat(buf, "                       |");

	sprintf(buf + strlen(buf),
		" %s�������:      %3d %s|"
		" %s  %+4d%% (%+4d) %s||\r\n",
		CCGRN(ch, C_NRM), GET_REAL_SAVING_REFLEX(ch), CCCYN(ch, C_NRM),
		CCRED(ch, C_NRM), GET_HITREG(ch), hit_gain(ch), CCCYN(ch, C_NRM));

	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
	{
		if (affected_by_spell(ch, SPELL_ABSTINENT))
			sprintf(buf + strlen(buf),
				" || %s��������.          %s|                       |",
				CCIYEL(ch, C_NRM), CCCYN(ch, C_NRM));
		else
			sprintf(buf + strlen(buf),
				" || %s�� �����.          %s|                       |",
				CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));
	}
	else
	{
		strcat(buf, " ||                    |                       |");
	}
	sprintf(buf + strlen(buf),
		" %s�����:       %4d %s|"
		" &r�����. ���:    &c||\r\n",
		CCGRN(ch, C_NRM), ch->calc_morale(), CCCYN(ch, C_NRM));

	const unsigned wdex = PlayerSystem::weight_dex_penalty(ch);
	if (wdex == 0)
	{
		strcat(buf, " ||                    |                       |");
	}
	else
	{
		sprintf(buf + strlen(buf),
			" || %s��������!%s          |                       |",
			wdex == 1 ? CCIYEL(ch, C_NRM) : CCIRED(ch, C_NRM),
			CCCYN(ch, C_NRM));
	}
	sprintf(buf + strlen(buf),
		" %s����������:  %4d %s|"
		" %s  %+4d%% (%+4d) %s||\r\n"
		" -------------------------------------------------------------------------------------\r\n",
		CCGRN(ch, C_NRM), calc_initiative(ch, false), CCCYN(ch, C_NRM),
		CCRED(ch, C_NRM), GET_MOVEREG(ch), move_gain(ch), CCCYN(ch, C_NRM));

	if (has_horse(ch, FALSE))
	{
		if (on_horse(ch))
			sprintf(buf + strlen(buf),
				" %s|| %s�� ������ �� %-67s%s||\r\n"
				" -------------------------------------------------------------------------------------\r\n",
				CCCYN(ch, C_NRM), CCIGRN(ch, C_NRM),
				(std::string(GET_PAD(get_horse(ch), 5)) + std::string(".")).substr(0, 67).c_str(), CCCYN(ch, C_NRM));
		else
			sprintf(buf + strlen(buf),
				" %s|| %s� ��� ���� %-69s%s||\r\n"
				" -------------------------------------------------------------------------------------\r\n",
				CCCYN(ch, C_NRM), CCIGRN(ch, C_NRM),
				(std::string(GET_NAME(get_horse(ch))) + std::string(".")).substr(0, 69).c_str(), CCCYN(ch, C_NRM));
	}

	//���������� � �����, ���� ��� ����.
	ROOM_DATA *label_room = RoomSpells::find_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);
	if (label_room)
	{
		timer_room_label = RoomSpells::timer_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);
		sprintf(buf + strlen(buf),
			" %s|| &G&q�� ��������� ������ ����� � ������� %s%s||\r\n",
			CCCYN(ch, C_NRM),
			colored_name(std::string(std::string("'") + label_room->name + std::string("&n&Q'.")).c_str(), 44),
			CCCYN(ch, C_NRM));
		if (timer_room_label > 0)
		{
			*buf2 = '\0';
			(timer_room_label + 1) / SECS_PER_MUD_HOUR ? sprintf(buf2, "%d %s.", (timer_room_label + 1) / SECS_PER_MUD_HOUR + 1, desc_count((timer_room_label + 1) / SECS_PER_MUD_HOUR + 1, WHAT_HOUR)) : sprintf(buf2, "����� ����.");
			sprintf(buf + strlen(buf),
				" || ����� ����������� ��� %-58s||\r\n", buf2);
			*buf2 = '\0';
		}
	}

	int glory = Glory::get_glory(GET_UNIQUE(ch));
	if (glory)
		sprintf(buf + strlen(buf),
			" %s|| %s�� ��������� %5d %-61s%s||\r\n",
			CCCYN(ch, C_NRM), CCWHT(ch, C_NRM), glory,
			(std::string(desc_count(glory, WHAT_POINT)) + std::string(" ����� ��� ���������� ��������� �������������.")).substr(0, 61).c_str(),
			CCCYN(ch, C_NRM));
	glory = GloryConst::get_glory(GET_UNIQUE(ch));
	if (glory)
		sprintf(buf + strlen(buf),
			" %s|| %s�� ��������� %5d %-61s%s||\r\n",
			CCCYN(ch, C_NRM), CCWHT(ch, C_NRM), glory,
			(std::string(desc_count(glory, WHAT_POINT)) + std::string(" ���������� �����.")).substr(0, 61).c_str(),
			CCCYN(ch, C_NRM));

	if (GET_GOD_FLAG(ch, GF_REMORT) && CLAN(ch))
	{
		sprintf(buf + strlen(buf),
			" || �� �������������� ������� ���� ���������� ���� ����� �������.                   ||\r\n");
	}

	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		sprintf(buf + strlen(buf),
			" || �� ������ ���� ��������.                                                        ||\r\n");
	else
		sprintf(buf + strlen(buf),
			" || �� �������� �� �������.                                                         ||\r\n");
	if (PRF_FLAGGED(ch, PRF_BLIND))
		sprintf(buf + strlen(buf),
			" || ����� ������� ������ �������.                                                   ||\r\n");
	if (Bonus::is_bonus(0))
		sprintf(buf + strlen(buf),
			" || %-79s ||\r\n || %-79s ||\r\n", Bonus::str_type_bonus().c_str(), Bonus::bonus_end().c_str());

	if (!NAME_GOD(ch) && GET_LEVEL(ch) <= NAME_LEVEL)
	{
		sprintf(buf + strlen(buf),
			" &c|| &R��������!&n ���� ��� �� ������� ����� �� �����!&c                                   ||\r\n");
		sprintf(buf + strlen(buf),
			" || &nC���� �� ���������� �������� ����, ���������� � ����� ��� ��������� �����.      &c||\r\n");
	}
	else if (NAME_BAD(ch))
	{
		sprintf(buf + strlen(buf),
			" || &R��������!&n ���� ��� ��������� ������. ����� ����� �� ���������� �������� ����.   &c||\r\n");
	}

	if (GET_LEVEL(ch) < LVL_IMMORT)
		sprintf(buf + strlen(buf),
			" || %s�� ������ �������� � ������ � ������������ ��������                             %s||\r\n"
			" || %s� %2d %-75s%s||\r\n",
			CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
			grouping[(int)GET_CLASS(ch)][(int)GET_REMORT(ch)],
			(std::string(desc_count(grouping[(int)GET_CLASS(ch)][(int)GET_REMORT(ch)], WHAT_LEVEL))
				+ std::string(" ��� ������ ��� �����.")).substr(0, 76).c_str(), CCCYN(ch, C_NRM));

	if (RENTABLE(ch))
	{
		time_t rent_time = RENTABLE(ch) - time(0);
		int minutes = rent_time > 60 ? rent_time / 60 : 0;
		sprintf(buf + strlen(buf),
			" || %s� ����� � ������� ���������� �� �� ������ ���� �� ������ ��� %-18s%s ||\r\n",
			CCIRED(ch, C_NRM),
			minutes ? (std::to_string(minutes) + std::string(" ") + std::string(desc_count(minutes, WHAT_MINu)) + std::string(".")).substr(0, 18).c_str()
			: (std::to_string(rent_time) + std::string(" ") + std::string(desc_count(rent_time, WHAT_SEC)) + std::string(".")).substr(0, 18).c_str(),
			CCCYN(ch, C_NRM));
	}
	else if ((ch->in_room != NOWHERE) && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER))
		sprintf(buf + strlen(buf),
			" || %s��� �� ���������� ���� � ������������.                                          %s||\r\n",
			CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (ROOM_FLAGGED(ch->in_room, ROOM_SMITH) && (ch->get_skill(SKILL_INSERTGEM) || ch->get_skill(SKILL_REPAIR) || ch->get_skill(SKILL_TRANSFORMWEAPON)))
		sprintf(buf + strlen(buf),
			" || %s��� ����� ������� �������� ��� ������� ��������� �����.                         %s||\r\n",
			CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (mail::has_mail(ch->get_uid()))
		sprintf(buf + strlen(buf),
			" || %s��� ������� ����� ������, ������� �� �����.                                     %s||\r\n",
			CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (Parcel::has_parcel(ch))
		sprintf(buf + strlen(buf),
			" || %s��� ������� �������, ������� �� �����.                                          %s||\r\n",
			CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));

	if (ch->get_protecting())
		sprintf(buf + strlen(buf),
			" || %s�� ����������� %-65s%s||\r\n",
			CCIGRN(ch, C_NRM), std::string(GET_PAD(ch->get_protecting(), 3) + std::string(" �� ���������.")).substr(0, 65).c_str(),
			CCCYN(ch, C_NRM));

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch))
	{
		int hrs = (GCURSE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((GCURSE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %s�� �������� ������ �� %3d %-5s %2d %-45s%s||\r\n",
			CCRED(ch, C_NRM), hrs, std::string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (std::string(desc_count(mins, WHAT_MINu)) + std::string(".")).substr(0, 45).c_str(), CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_HELLED) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(NULL))
	{
		int hrs = (HELL_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((HELL_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %s��� ��������� �������� � ������� ��� %6d %-5s %2d %-27s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, std::string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (std::string(desc_count(mins, WHAT_MINu)) + std::string(".")).substr(0, 27).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(std::string(HELL_REASON(ch) ? HELL_REASON(ch) : "-") + std::string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_MUTE) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(NULL))
	{
		int hrs = (MUTE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((MUTE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %s�� �� ������� ������� ��� %6d %-5s %2d %-38s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, std::string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (std::string(desc_count(mins, WHAT_MINu)) + std::string(".")).substr(0, 38).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(std::string(MUTE_REASON(ch) ? MUTE_REASON(ch) : "-") + std::string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
	}

	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(NULL))
	{
		int hrs = (UNREG_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((UNREG_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %s�� �� ������� ������� � ������ IP ��� %6d %-5s %2d %-26s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, std::string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (std::string(desc_count(mins, WHAT_MINu)) + std::string(".")).substr(0, 38).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(std::string(UNREG_REASON(ch) ? UNREG_REASON(ch) : "-") + std::string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_DUMB) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(NULL))
	{
		int hrs = (DUMB_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((DUMB_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %s�� ������ ������� ��� %6d %-5s %2d %-42s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, std::string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (std::string(desc_count(mins, WHAT_MINu)) + std::string(".")).substr(0, 42).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(std::string(DUMB_REASON(ch) ? DUMB_REASON(ch) : "-") + std::string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
	}

	if (PLR_FLAGGED(ch, PLR_FROZEN) && FREEZE_DURATION(ch) != 0 && FREEZE_DURATION(ch) > time(NULL))
	{
		int hrs = (FREEZE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((FREEZE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf + strlen(buf),
			" || %s�� ������ ���������� ��� %6d %-5s %2d %-39s%s||\r\n"
			" || %s[%-79s%s||\r\n",
			CCRED(ch, C_NRM), hrs, std::string(desc_count(hrs, WHAT_HOUR)).substr(0, 5).c_str(),
			mins, (std::string(desc_count(mins, WHAT_MINu)) + std::string(".")).substr(0, 42).c_str(),
			CCCYN(ch, C_NRM), CCRED(ch, C_NRM),
			(std::string(FREEZE_REASON(ch) ? FREEZE_REASON(ch) : "-") + std::string("].")).substr(0, 79).c_str(),
			CCCYN(ch, C_NRM));
	}

	if (ch->is_morphed())
	{
		sprintf(buf + strlen(buf),
			" || %s�� ���������� � �������� ����� - %-47s%s||\r\n",
			CCYEL(ch, C_NRM),
			ch->get_morph_desc().substr(0, 47).c_str(),
			CCCYN(ch, C_NRM));
	}
	strcat(buf, " ||                                                                                 ||\r\n");
	strcat(buf, " -------------------------------------------------------------------------------------\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	//	test_self_hitroll(ch);
}

void do_score(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int ac, ac_t;

	skip_spaces(&argument);

	if (IS_NPC(ch))
		return;

	//��������� ������� "���� ���", ������� Adept. ������ ������� - 85 �������� + ������.
	if (is_abbrev(argument, "���") || is_abbrev(argument, "all"))
	{
		print_do_score_all(ch);
		return;
	}

	sprintf(buf, "�� %s (%s, %s, %s, %s %d ������).\r\n",
		ch->only_title().c_str(),
		std::string(PlayerRace::GetKinNameByNum(GET_KIN(ch), GET_SEX(ch))).c_str(),
		std::string(PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), GET_SEX(ch))).c_str(),
		religion_name[GET_RELIGION(ch)][(int)GET_SEX(ch)],
		class_name[(int)GET_CLASS(ch) + 14 * GET_KIN(ch)], GET_LEVEL(ch));

	if (!NAME_GOD(ch) && GET_LEVEL(ch) <= NAME_LEVEL)
	{
		sprintf(buf + strlen(buf), "\r\n&R��������!&n ���� ��� �� ������� ����� �� �����!\r\n");
		sprintf(buf + strlen(buf), "����� ����� �� ���������� �������� ����,\r\n");
		sprintf(buf + strlen(buf), "���������� � ����� ��� ��������� �����.\r\n\r\n");
	}
	else if (NAME_BAD(ch))
	{
		sprintf(buf + strlen(buf), "\r\n&R��������!&n ���� ��� ��������� ������.\r\n");
		sprintf(buf + strlen(buf), "����� ����� �� ���������� �������� ����.\r\n\r\n");
	}

	sprintf(buf + strlen(buf), "������ ��� %d %s. ", GET_REAL_AGE(ch), desc_count(GET_REAL_AGE(ch), WHAT_YEAR));

	if (age(ch)->month == 0 && age(ch)->day == 0)
	{
		sprintf(buf2, "%s� ��� ������� ���� �������!%s\r\n", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	}
	else
		strcat(buf, "\r\n");

	sprintf(buf + strlen(buf),
		"�� ������ ��������� %d(%d) %s �����������, � ������ %d(%d) %s �� ������ ���������.\r\n",
		GET_HIT(ch), GET_REAL_MAX_HIT(ch), desc_count(GET_HIT(ch),
			WHAT_ONEu),
		GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), desc_count(GET_MOVE(ch), WHAT_MOVEu));

	if (IS_MANA_CASTER(ch))
	{
		sprintf(buf + strlen(buf),
			"���� ���������� ������� %d(%d) � �� ���������������� %d � ���.\r\n",
			GET_MANA_STORED(ch), GET_MAX_MANA(ch), mana_gain(ch));
	}

	sprintf(buf + strlen(buf),
		"%s���� �������������� :\r\n"
		"  ���� : %2d(%2d)"
		"  ���� : %2d(%2d)"
		"  ���� : %2d(%2d)"
		"  ���� : %2d(%2d)"
		"  ��   : %2d(%2d)"
		"  �����: %2d(%2d)\r\n"
		"  ������ %3d(%3d)"
		"  ����   %3d(%3d)"
		"  ���    %3d(%3d)%s\r\n",
		CCICYN(ch, C_NRM), ch->get_str(), GET_REAL_STR(ch),
		ch->get_dex(), GET_REAL_DEX(ch),
		ch->get_con(), GET_REAL_CON(ch),
		ch->get_wis(), GET_REAL_WIS(ch),
		ch->get_int(), GET_REAL_INT(ch),
		ch->get_cha(), GET_REAL_CHA(ch),
		GET_SIZE(ch), GET_REAL_SIZE(ch),
		GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), CCNRM(ch, C_NRM));

	if (IS_IMMORTAL(ch))
	{
		sprintf(buf + strlen(buf),
			"%s���� ������ �������� :\r\n"
			"  AC   : %4d(%4d)"
			"  DR   : %4d(%4d)%s\r\n",
			CCIGRN(ch, C_NRM), GET_AC(ch), compute_armor_class(ch),
			GET_DR(ch), GET_REAL_DR(ch), CCNRM(ch, C_NRM));
	}
	else
	{
		ac = compute_armor_class(ch) / 10;

		if (ac < 5)
		{
			const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
			ac = ac + mod > 5 ? 5 : ac + mod;
		}

		ac_t = MAX(MIN(ac + 30, 40), 0);
		sprintf(buf + strlen(buf), "&G���� ������ �������� :\r\n"
			"  ������  (AC)     : %4d - %s&G\r\n"
			"  �����/���������� : %4d/%d&n\r\n",
			ac, ac_text[ac_t], GET_ARMOUR(ch), GET_ABSORBE(ch));
	}
	sprintf(buf + strlen(buf), "��� ���� - %ld %s, � ��� �� ����� %ld %s",
		GET_EXP(ch), desc_count(GET_EXP(ch), WHAT_POINT), ch->get_gold(), desc_count(ch->get_gold(), WHAT_MONEYa));
	if (ch->get_bank() > 0)
		sprintf(buf + strlen(buf), "(� ��� %ld %s ���������� � �����).\r\n",
			ch->get_bank(), desc_count(ch->get_bank(), WHAT_MONEYa));
	else
		strcat(buf, ".\r\n");

	if (GET_LEVEL(ch) < LVL_IMMORT)
		sprintf(buf + strlen(buf),
			"��� �������� ������� %ld %s �� ���������� ������.\r\n",
			level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch),
			desc_count(level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch), WHAT_POINT));
	if (GET_LEVEL(ch) < LVL_IMMORT)
		sprintf(buf + strlen(buf),
			"�� ������ �������� � ������ � ������������ �������� � %d %s ��� ������ ��� �����.\r\n",
			grouping[(int)GET_CLASS(ch)][(int)GET_REMORT(ch)],
			desc_count(grouping[(int)GET_CLASS(ch)][(int)GET_REMORT(ch)], WHAT_LEVEL));

	//���������� � �����, ���� ��� ����.
	ROOM_DATA *label_room = RoomSpells::find_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);
	if (label_room)
	{
		sprintf(buf + strlen(buf),
			"&G&q�� ��������� ������ ����� � ������� '%s'.&Q&n\r\n",
			std::string(label_room->name).c_str());
	}

	int glory = Glory::get_glory(GET_UNIQUE(ch));
	if (glory)
	{
		sprintf(buf + strlen(buf), "�� ��������� %d %s �����.\r\n",
			glory, desc_count(glory, WHAT_POINT));
	}
	glory = GloryConst::get_glory(GET_UNIQUE(ch));
	if (glory)
	{
		sprintf(buf + strlen(buf), "�� ��������� %d %s ���������� �����.\r\n",
			glory, desc_count(glory, WHAT_POINT));
	}

	TIME_INFO_DATA playing_time = *real_time_passed((time(0) - ch->player_data.time.logon) + ch->player_data.time.played, 0);
	sprintf(buf + strlen(buf), "�� ������� %d %s %d %s ��������� �������.\r\n",
		playing_time.day, desc_count(playing_time.day, WHAT_DAY),
		playing_time.hours, desc_count(playing_time.hours, WHAT_HOUR));

	if (!on_horse(ch))
		switch (GET_POS(ch))
		{
		case POS_DEAD:
			strcat(buf, "�� ������!\r\n");
			break;
		case POS_MORTALLYW:
			strcat(buf, "�� ���������� ������ � ���������� � ������!\r\n");
			break;
		case POS_INCAP:
			strcat(buf, "�� ��� �������� � �������� ��������...\r\n");
			break;
		case POS_STUNNED:
			strcat(buf, "�� � ��������!\r\n");
			break;
		case POS_SLEEPING:
			strcat(buf, "�� �����.\r\n");
			break;
		case POS_RESTING:
			strcat(buf, "�� ���������.\r\n");
			break;
		case POS_SITTING:
			strcat(buf, "�� ������.\r\n");
			break;
		case POS_FIGHTING:
			if (ch->get_fighting())
				sprintf(buf + strlen(buf), "�� ���������� � %s.\r\n", GET_PAD(ch->get_fighting(), 4));
			else
				strcat(buf, "�� ������ �������� �� �������.\r\n");
			break;
		case POS_STANDING:
			strcat(buf, "�� ������.\r\n");
			break;
		default:
			strcat(buf, "You are floating.\r\n");
			break;
		}
	send_to_char(buf, ch);

	strcpy(buf, CCIGRN(ch, C_NRM));
	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
	{
		if (affected_by_spell(ch, SPELL_ABSTINENT))
			strcat(buf, "������ � �������� ������!\r\n");
		else
			strcat(buf, "�� �����.\r\n");
	}
	if (GET_COND_M(ch, FULL))
		strcat(buf, "�� �������.\r\n");
	if (GET_COND_M(ch, THIRST))
		strcat(buf, "��� ������ �����.\r\n");
	/*
	   strcat(buf, CCICYN(ch, C_NRM));
	   strcat(buf,"������� :\r\n");
	   (ch)->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, "\r\n");
	   strcat(buf,buf2);
	 */
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		strcat(buf, "�� ������ ���� ��������.\r\n");

	if (has_horse(ch, FALSE))
	{
		if (on_horse(ch))
			sprintf(buf + strlen(buf), "�� ������ �� %s.\r\n", GET_PAD(get_horse(ch), 5));
		else
			sprintf(buf + strlen(buf), "� ��� ���� %s.\r\n", GET_NAME(get_horse(ch)));
	}
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	if (RENTABLE(ch))
	{
		sprintf(buf,
			"%s� ����� � ������� ���������� �� �� ������ ���� �� ������.%s\r\n",
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}
	else if ((ch->in_room != NOWHERE) && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER))
	{
		sprintf(buf, "%s��� �� ���������� ���� � ������������.%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_SMITH) && (ch->get_skill(SKILL_INSERTGEM) || ch->get_skill(SKILL_REPAIR) || ch->get_skill(SKILL_TRANSFORMWEAPON)))
	{
		sprintf(buf, "%s��� ����� ������� �������� ��� ������� ��������� �����.%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (mail::has_mail(ch->get_uid()))
	{
		sprintf(buf, "%s��� ������� ����� ������, ������� �� �����!%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (Parcel::has_parcel(ch))
	{
		sprintf(buf, "%s��� ������� �������, ������� �� �����!%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (PLR_FLAGGED(ch, PLR_HELLED) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(NULL))
	{
		int hrs = (HELL_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((HELL_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf,
			"��� ��������� �������� � ������� ��� %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins,
				WHAT_MINu),
			HELL_REASON(ch) ? HELL_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_MUTE) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(NULL))
	{
		int hrs = (MUTE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((MUTE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "�� �� ������� ������� ��� %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR),
			mins, desc_count(mins, WHAT_MINu), MUTE_REASON(ch) ? MUTE_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_DUMB) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(NULL))
	{
		int hrs = (DUMB_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((DUMB_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "�� ������ ������� ��� %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR),
			mins, desc_count(mins, WHAT_MINu), DUMB_REASON(ch) ? DUMB_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_FROZEN) && FREEZE_DURATION(ch) != 0 && FREEZE_DURATION(ch) > time(NULL))
	{
		int hrs = (FREEZE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((FREEZE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "�� ������ ���������� ��� %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR),
			mins, desc_count(mins, WHAT_MINu), FREEZE_REASON(ch) ? FREEZE_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(NULL))
	{
		int hrs = (UNREG_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((UNREG_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "�� �� ������� �������� � ������ IP ��� %d %s %d %s [%s].\r\n",
			hrs, desc_count(hrs, WHAT_HOUR),
			mins, desc_count(mins, WHAT_MINu), UNREG_REASON(ch) ? UNREG_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch))
	{
		int hrs = (GCURSE_DURATION(ch) - time(NULL)) / 3600;
		int mins = ((GCURSE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
		sprintf(buf, "�� �������� ������ �� %d %s %d %s.\r\n",
			hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins, WHAT_MINu));
		send_to_char(buf, ch);
	}

	if (ch->is_morphed())
	{
		sprintf(buf, "�� ���������� � �������� ����� - %s.\r\n", ch->get_morph_desc().c_str());
		send_to_char(buf, ch);
	}
	if (can_use_feat(ch, COLLECTORSOULS_FEAT))
	{
		int souls = ch->get_souls();
		if (souls == 0)
		{
			sprintf(buf, "�� �� ������ ����� ���.\r\n");
			send_to_char(buf, ch);
		}
		else
		{
			if (souls == 1)
			{
				sprintf(buf, "�� ������ ����� ���� ���� � ������.\r\n");
				send_to_char(buf, ch);
			}
			if (souls > 1 && souls < 5)
			{
				sprintf(buf, "�� ������ %d ���� � ������.\r\n", souls);
				send_to_char(buf, ch);
			}
			if (souls >= 5)
			{
				sprintf(buf, "�� ������ %d ����� ��� � ������.\r\n", souls);
				send_to_char(buf, ch);
			}
		}
	}
	if (ch->get_ice_currency() > 0)
	{
		if (ch->get_ice_currency() == 1)
		{
			sprintf(buf, "� ��� � ������� ���� ���� ������ ��������� ��������.\r\n");
			send_to_char(buf, ch);
		}
		else if (ch->get_ice_currency() < 5)
		{
			sprintf(buf, "� ��� � ������� ���� ������ %d ��������� ��������.\r\n", ch->get_ice_currency());
			send_to_char(buf, ch);
		}
		else
		{
			sprintf(buf, "� ��� � ������� ���� %d ��������� ��������.\r\n", ch->get_ice_currency());
			send_to_char(buf, ch);
		}
	}
}

//29.11.09 ����������� ���������� ����� (�) ��������
// edited by WorM 2011.05.21
void do_mystat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	skip_spaces(&argument);
	if (is_abbrev(argument, "��������") || is_abbrev(argument, "clear"))
	{
		GET_RIP_MOBTHIS(ch) = GET_EXP_MOBTHIS(ch) = GET_RIP_MOB(ch) = GET_EXP_MOB(ch) =
		GET_RIP_PKTHIS(ch) = GET_EXP_PKTHIS(ch) = GET_RIP_PK(ch) = GET_EXP_PK(ch) =
		GET_RIP_DTTHIS(ch) = GET_EXP_DTTHIS (ch) = GET_RIP_DT(ch) = GET_EXP_DT(ch) =
		GET_RIP_OTHERTHIS(ch) = GET_EXP_OTHERTHIS(ch) = GET_RIP_OTHER(ch) = GET_EXP_OTHER(ch) =
		GET_WIN_ARENA(ch) = GET_RIP_ARENA(ch) = GET_EXP_ARENA(ch) = 0;
		send_to_char("���������� �������.\r\n", ch);
	}
	else
	{
		sprintf(buf,    " &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n   ���������� ����� �������   &C|&n         &W�������&n         &C|&n                         &C||&n\r\n"
				" &C||&n (����������, �������� �����) &C|&n      &W��������������&n     &C|&n           &K�����&n         &C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n    � �������� ��� � �������: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C||&n    � �������� ��� � �������: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C||&n             � ������ ������: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C||&n   �� �������� �������������: &C|&n &W%4d (%16llu)&n &C|&n &K%4d (%16llu)&n &C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n                       &y�����:&n &C|&n &W%4d (%16llu)&n &C| &K%4d (%16llu)&n &n&C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				" &C||&n &W�� ����� (�����):                                                                &n&C||&n\r\n"
				" &C||&n   &w����� �������:&n&r%4d&n     &w�������:&n&r%4d&n           &w�������� �����:&n &r%16llu&n &C||&n\r\n"
				" &C--------------------------------------------------------------------------------------&n\r\n"
				,
				GET_RIP_MOBTHIS(ch),GET_EXP_MOBTHIS(ch), GET_RIP_MOB(ch), GET_EXP_MOB(ch),
				GET_RIP_PKTHIS(ch),GET_EXP_PKTHIS(ch), GET_RIP_PK(ch), GET_EXP_PK(ch),
				GET_RIP_DTTHIS(ch),GET_EXP_DTTHIS (ch), GET_RIP_DT(ch), GET_EXP_DT(ch),
				GET_RIP_OTHERTHIS(ch),GET_EXP_OTHERTHIS(ch), GET_RIP_OTHER(ch), GET_EXP_OTHER(ch),
				GET_RIP_MOBTHIS(ch)+GET_RIP_PKTHIS(ch)+GET_RIP_DTTHIS(ch)+GET_RIP_OTHERTHIS(ch),
				GET_EXP_MOBTHIS(ch)+GET_EXP_PKTHIS(ch)+GET_EXP_DTTHIS(ch)+GET_EXP_OTHERTHIS(ch)+GET_EXP_ARENA(ch),
				GET_RIP_MOB(ch)+GET_RIP_PK(ch)+GET_RIP_DT(ch)+GET_RIP_OTHER(ch),
				GET_EXP_MOB(ch)+GET_EXP_PK(ch)+GET_EXP_DT(ch)+GET_EXP_OTHER(ch)+GET_EXP_ARENA(ch),
				GET_WIN_ARENA(ch),GET_RIP_ARENA(ch), GET_EXP_ARENA(ch));
		send_to_char(buf, ch);
	}
}
// end by WorM
// ����� ������ (�) ��������

void do_inventory(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	send_to_char("�� ������:\r\n", ch);
	list_obj_to_char(ch->carrying, ch, 1, 2);
}

void do_equipment(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int i, found = 0;
	skip_spaces(&argument);

	send_to_char("�� ��� ������:\r\n", ch);
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
			{
				send_to_char(where[i], ch);
				show_obj_to_char(GET_EQ(ch, i), ch, 1, TRUE, 1);
				found = TRUE;
			}
			else
			{
				send_to_char(where[i], ch);
				send_to_char("���-��.\r\n", ch);
				found = TRUE;
			}
		}
		else		// added by Pereplut
		{
			if (is_abbrev(argument, "���") || is_abbrev(argument, "all"))
			{
			    if (GET_EQ(ch, 18))
				if ((i==16) || (i==17))
				    continue;
                            if ((i==19)&&(GET_EQ(ch, WEAR_BOTHS)))
                            {
                                if (!(((GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS))) == OBJ_DATA::ITEM_WEAPON) && (GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS )))
                                    continue;
                            }
                            else if (i==19)
				continue;
 			    if (GET_EQ(ch, 16) || GET_EQ(ch, 17))
				if (i==18)
				    continue;
			    if (GET_EQ(ch, 11))
				{
					if ((i==17) || (i==18))
						continue;
				}
				send_to_char(where[i], ch);
				sprintf(buf, "%s[ ������ ]%s\r\n", CCINRM(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				found = TRUE;
			}
		}
	}
	if (!found)
	{
		if (IS_FEMALE(ch))
			send_to_char("������ ��� ��� ����� ���� :)\r\n", ch);
		else
			send_to_char(" �� ����, ��� �����.\r\n", ch);
	}
}

void do_time(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int day, month, days_go;
	if (IS_NPC(ch))
		return;
	sprintf(buf, "������ ");
	switch (time_info.hours % 24)
	{
	case 0:
		sprintf(buf + strlen(buf), "�������, ");
		break;
	case 1:
		sprintf(buf + strlen(buf), "1 ��� ����, ");
		break;
	case 2:
	case 3:
	case 4:
		sprintf(buf + strlen(buf), "%d ���� ����, ", time_info.hours);
		break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		sprintf(buf + strlen(buf), "%d ����� ����, ", time_info.hours);
		break;
	case 12:
		sprintf(buf + strlen(buf), "�������, ");
		break;
	case 13:
		sprintf(buf + strlen(buf), "1 ��� ���������, ");
		break;
	case 14:
	case 15:
	case 16:
		sprintf(buf + strlen(buf), "%d ���� ���������, ", time_info.hours - 12);
		break;
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
		sprintf(buf + strlen(buf), "%d ����� ������, ", time_info.hours - 12);
		break;
	}

	if (GET_RELIGION(ch) == RELIGION_POLY)
		strcat(buf, weekdays_poly[weather_info.week_day_poly]);
	else
		strcat(buf, weekdays[weather_info.week_day_mono]);
	switch (weather_info.sunlight)
	{
	case SUN_DARK:
		strcat(buf, ", ����");
		break;
	case SUN_SET:
		strcat(buf, ", �����");
		break;
	case SUN_LIGHT:
		strcat(buf, ", ����");
		break;
	case SUN_RISE:
		strcat(buf, ", �������");
		break;
	}
	strcat(buf, ".\r\n");
	send_to_char(buf, ch);

	day = time_info.day + 1;	// day in [1..30]
	*buf = '\0';
	if (GET_RELIGION(ch) == RELIGION_POLY || IS_IMMORTAL(ch))
	{
		days_go = time_info.month * DAYS_PER_MONTH + time_info.day;
		month = days_go / 40;
		days_go = (days_go % 40) + 1;
		sprintf(buf + strlen(buf), "%s, %d� ����, ��� %d%s",
				month_name_poly[month], days_go, time_info.year, IS_IMMORTAL(ch) ? ".\r\n" : "");
	}
	if (GET_RELIGION(ch) == RELIGION_MONO || IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), "%s, %d� ����, ��� %d",
				month_name[(int) time_info.month], day, time_info.year);
	if (IS_IMMORTAL(ch))
		sprintf(buf + strlen(buf), "\r\n%d.%d.%d, ���� � ������ ����: %d", day, time_info.month+1, time_info.year, (time_info.month *DAYS_PER_MONTH) + day);
	switch (weather_info.season)
	{
	case SEASON_WINTER:
		strcat(buf, ", ����");
		break;
	case SEASON_SPRING:
		strcat(buf, ", �����");
		break;
	case SEASON_SUMMER:
		strcat(buf, ", ����");
		break;
	case SEASON_AUTUMN:
		strcat(buf, ", �����");
		break;
	}
	strcat(buf, ".\r\n");
	send_to_char(buf, ch);
	gods_day_now(ch);
}

int get_moon(int sky)
{
	if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT || sky == SKY_RAINING)
		return (0);
	else if (weather_info.moon_day <= NEWMOONSTOP || weather_info.moon_day >= NEWMOONSTART)
		return (1);
	else if (weather_info.moon_day < HALFMOONSTART)
		return (2);
	else if (weather_info.moon_day < FULLMOONSTART)
		return (3);
	else if (weather_info.moon_day <= FULLMOONSTOP)
		return (4);
	else if (weather_info.moon_day < LASTHALFMOONSTART)
		return (5);
	else
		return (6);
	return (0);
}

void do_weather(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int sky = weather_info.sky, weather_type = weather_info.weather_type;
	const char *sky_look[] = { "��������",
							   "���������",
							   "������� �������� ������",
							   "�����"
							 };
	const char *moon_look[] = { "���������.",
								"�������� ���� ����.",
								"�������� ����.",
								"����������.",
								"��������� ����.",
								"��������� ���� ����."
							  };

	if (OUTSIDE(ch))
	{
		*buf = '\0';
		if (world[ch->in_room]->weather.duration > 0)
		{
			sky = world[ch->in_room]->weather.sky;
			weather_type = world[ch->in_room]->weather.weather_type;
		}
		sprintf(buf + strlen(buf),
				"���� %s. %s\r\n%s\r\n", sky_look[sky],
				get_moon(sky) ? moon_look[get_moon(sky) - 1] : "",
				(weather_info.change >=
				 0 ? "����������� �������� ����������." : "����������� �������� ����������."));
		sprintf(buf + strlen(buf), "�� ����� %d %s.\r\n",
				weather_info.temperature, desc_count(weather_info.temperature, WHAT_DEGREE));

		if (IS_SET(weather_info.weather_type, WEATHER_BIGWIND))
			strcat(buf, "������� �����.\r\n");
		else if (IS_SET(weather_info.weather_type, WEATHER_MEDIUMWIND))
			strcat(buf, "��������� �����.\r\n");
		else if (IS_SET(weather_info.weather_type, WEATHER_LIGHTWIND))
			strcat(buf, "������ �������.\r\n");

		if (IS_SET(weather_type, WEATHER_BIGSNOW))
			strcat(buf, "����� ����.\r\n");
		else if (IS_SET(weather_type, WEATHER_MEDIUMSNOW))
			strcat(buf, "��������.\r\n");
		else if (IS_SET(weather_type, WEATHER_LIGHTSNOW))
			strcat(buf, "������ ������.\r\n");

		if (IS_SET(weather_type, WEATHER_GRAD))
			strcat(buf, "����� � ������.\r\n");
		else if (IS_SET(weather_type, WEATHER_BIGRAIN))
			strcat(buf, "����, ��� �� �����.\r\n");
		else if (IS_SET(weather_type, WEATHER_MEDIUMRAIN))
			strcat(buf, "���� �����.\r\n");
		else if (IS_SET(weather_type, WEATHER_LIGHTRAIN))
			strcat(buf, "������� ������.\r\n");

		send_to_char(buf, ch);
	}
	else
		send_to_char("�� ������ �� ������ ������� � ������ �������.\r\n", ch);
	if (IS_GOD(ch))
	{
		sprintf(buf, "����: %d �����: %s ���: %d ���� = %d\r\n"
				"����������� =%-5d, �� ���� = %-8d, �� ������ = %-8d\r\n"
				"��������    =%-5d, �� ���� = %-8d, �� ������ = %-8d\r\n"
				"������ ����� = %d(%d), ����� = %d(%d). ��� = %d(%d). ������ = %08x(%08x).\r\n",
				time_info.day, month_name[time_info.month], time_info.hours,
				weather_info.hours_go, weather_info.temperature,
				weather_info.temp_last_day, weather_info.temp_last_week,
				weather_info.pressure, weather_info.press_last_day,
				weather_info.press_last_week, weather_info.rainlevel,
				world[ch->in_room]->weather.rainlevel, weather_info.snowlevel,
				world[ch->in_room]->weather.snowlevel, weather_info.icelevel,
				world[ch->in_room]->weather.icelevel,
				weather_info.weather_type, world[ch->in_room]->weather.weather_type);
		send_to_char(buf, ch);
	}
}

namespace
{

const char* IMM_WHO_FORMAT =
"������: ��� [�������[-��������]] [-n ���] [-c ��������] [-s] [-r] [-z] [-h] [-b|-�]\r\n";

const char* MORT_WHO_FORMAT = "������: ��� [���] [-?]\r\n";

} // namespace

void do_who(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char name_search[MAX_INPUT_LENGTH];
	name_search[0] = '\0';

	// ����� ��� �����
	int low = 0, high = LVL_IMPL;
	int showclass = 0, num_can_see = 0;
	int imms_num = 0, morts_num = 0, demigods_num = 0;
	bool localwho = false, short_list = false;
	bool who_room = false, showname = false;

	skip_spaces(&argument);
	strcpy(buf, argument);

	// �������� ���������� ������� "���"
	while (*buf)
	{
		half_chop(buf, arg, buf1);
		if (!str_cmp(arg, "����") && strlen(arg) == 4)
		{
			low = LVL_IMMORT;
			high = LVL_IMPL;
			strcpy(buf, buf1);
		}
		else if (a_isdigit(*arg))
		{
			if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
				sscanf(arg, "%d-%d", &low, &high);
			strcpy(buf, buf1);
		}
		else if (*arg == '-')
		{
			const char mode = *(arg + 1);	// just in case; we destroy arg in the switch
			switch (mode)
			{
			case 'b':
			case '�':
				if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_DEMIGOD) || PRF_FLAGGED(ch, PRF_CODERINFO))
					showname = true;
				strcpy(buf, buf1);
				break;
			case 'z':
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					localwho = true;
				strcpy(buf, buf1);
				break;
			case 's':
				if (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					short_list = true;
				strcpy(buf, buf1);
				break;
			case 'l':
				half_chop(buf1, arg, buf);
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				half_chop(buf1, name_search, buf);
				break;
			case 'r':
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					who_room = true;
				strcpy(buf, buf1);
				break;
			case 'c':
				half_chop(buf1, arg, buf);
				if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
				{
					const size_t len = strlen(arg);
					for (size_t i = 0; i < len; i++)
					{
						showclass |= find_class_bitvector(arg[i]);
					}
				}
				break;
			case 'h':
			case '?':
			default:
				if (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
					send_to_char(IMM_WHO_FORMAT, ch);
				else
					send_to_char(MORT_WHO_FORMAT, ch);
				return;
			}	// end of switch
		}
		else  	// endif
		{
			strcpy(name_search, arg);
			strcpy(buf, buf1);

		}
	}			// end while (parser)

	if (who_spamcontrol(ch, strlen(name_search) ? WHO_LISTNAME : WHO_LISTALL))
		return;

	// ������ ���������� �����
	sprintf(buf, "%s����%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
	std::string imms(buf);

	sprintf(buf, "%s�����������������%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	std::string demigods(buf);

	sprintf(buf, "%s������%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	std::string morts(buf);

	int all = 0;

	for (const auto& tch: character_list)
	{
		if (IS_NPC(tch))
			continue;

		if (!HERE(tch))
			continue;

		if (!*argument && GET_LEVEL(tch) < LVL_IMMORT)
			++all;

		if (*name_search && !(isname(name_search, GET_NAME(tch))))
			continue;

		if (!CAN_SEE_CHAR(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
			continue;
		if (localwho && world[ch->in_room]->zone != world[tch->in_room]->zone)
			continue;
		if (who_room && (tch->in_room != ch->in_room))
			continue;
		if (showclass && !(showclass & (1 << GET_CLASS(tch))))
			continue;
		if (showname && !(!NAME_GOD(tch) && GET_LEVEL(tch) <= NAME_LEVEL))
			continue;
		if (PLR_FLAGGED(tch, PLR_NAMED) && NAME_DURATION(tch) && !IS_IMMORTAL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO) && ch != tch.get())
			continue;

		*buf = '\0';
		num_can_see++;

		if (short_list)
		{
			char tmp[MAX_INPUT_LENGTH];
			snprintf(tmp, sizeof(tmp), "%s%s%s", CCPK(ch, C_NRM, tch), GET_NAME(tch), CCNRM(ch, C_NRM));
			if (IS_IMPL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				sprintf(buf, "%s[%2d %s %s] %-30s%s",
					IS_GOD(tch) ? CCWHT(ch, C_SPR) : "",
					GET_LEVEL(tch), KIN_ABBR(tch), CLASS_ABBR(tch),
					tmp, IS_GOD(tch) ? CCNRM(ch, C_SPR) : "");
			}
			else
			{
				sprintf(buf, "%s%-30s%s",
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "",
					tmp, IS_IMMORTAL(tch) ? CCNRM(ch, C_SPR) : "");
			}
		}
		else
		{
			if (IS_IMPL(ch)
				|| PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				sprintf(buf, "%s[%2d %2d %s(%5d)] %s%s%s%s",
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "",
					GET_LEVEL(tch),
					GET_REMORT(tch),
					CLASS_ABBR(tch),
					tch->get_pfilepos(),
					CCPK(ch, C_NRM, tch),
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", tch->race_or_title().c_str(), CCNRM(ch, C_NRM));
			}
			else
			{
				sprintf(buf, "%s %s%s%s",
					CCPK(ch, C_NRM, tch),
					IS_IMMORTAL(tch) ? CCWHT(ch, C_SPR) : "", tch->race_or_title().c_str(), CCNRM(ch, C_NRM));
			}

			if (GET_INVIS_LEV(tch))
				sprintf(buf + strlen(buf), " (i%d)", GET_INVIS_LEV(tch));
			else if (AFF_FLAGGED(tch, EAffectFlag::AFF_INVISIBLE))
				sprintf(buf + strlen(buf), " (�������%s)", GET_CH_SUF_6(tch));
			if (AFF_FLAGGED(tch, EAffectFlag::AFF_HIDE))
				strcat(buf, " (��������)");
			if (AFF_FLAGGED(tch, EAffectFlag::AFF_CAMOUFLAGE))
				strcat(buf, " (�����������)");

			if (PLR_FLAGGED(tch, PLR_MAILING))
				strcat(buf, " (���������� ������)");
			else if (PLR_FLAGGED(tch, PLR_WRITING))
				strcat(buf, " (�����)");

			if (PRF_FLAGGED(tch, PRF_NOHOLLER))
				sprintf(buf + strlen(buf), " (����%s)", GET_CH_SUF_1(tch));
			if (PRF_FLAGGED(tch, PRF_NOTELL))
				sprintf(buf + strlen(buf), " (�����%s)", GET_CH_SUF_6(tch));
			if (PLR_FLAGGED(tch, PLR_MUTE))
				sprintf(buf + strlen(buf), " (������)");
			if (PLR_FLAGGED(tch, PLR_DUMB))
				sprintf(buf + strlen(buf), " (���%s)", GET_CH_SUF_6(tch));
			if (PLR_FLAGGED(tch, PLR_KILLER) == PLR_KILLER)
				sprintf(buf + strlen(buf), "&R (�������)&n");
			if ( (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_DEMIGOD)) &&  !NAME_GOD(tch)
					&& GET_LEVEL(tch) <= NAME_LEVEL)
			{
				sprintf(buf + strlen(buf), " &W!�� ��������!&n");
				if (showname)
				{
					sprintf(buf + strlen(buf),
							"\r\n������: %s/%s/%s/%s/%s/%s Email: &S%s&s ���: %s",
							GET_PAD(tch, 0), GET_PAD(tch, 1), GET_PAD(tch, 2),
							GET_PAD(tch, 3), GET_PAD(tch, 4), GET_PAD(tch, 5), 
							GET_GOD_FLAG(ch, GF_DEMIGOD) ? "������" : GET_EMAIL(tch),
							genders[(int)GET_SEX(tch)]);
				}
			}
			if ((GET_LEVEL(ch) == LVL_IMPL) && (RENTABLE(tch)))
			    sprintf(buf + strlen(buf), " &R(� �����)&n");
			else if ((IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO)) && NAME_BAD(tch))
			{
				sprintf(buf + strlen(buf), " &W������ %s!&n", get_name_by_id(NAME_ID_GOD(tch)));
			}
			if (IS_GOD(ch) && (GET_GOD_FLAG(tch, GF_TESTER) || PRF_FLAGGED(tch, PRF_TESTER)))
				sprintf(buf + strlen(buf), " &G(������!)&n");
			if (IS_IMMORTAL(tch))
				strcat(buf, CCNRM(ch, C_SPR));
		}		// endif shortlist

		if (IS_IMMORTAL(tch))
		{
			imms_num++;
			imms += buf;
			if (!short_list || !(imms_num % 4))
			{
				imms += "\r\n";
			}
		}
		else if (GET_GOD_FLAG(tch, GF_DEMIGOD)
			&& (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO) || GET_GOD_FLAG(tch, GF_DEMIGOD)))
		{
			demigods_num++;
			demigods += buf;
			if (!short_list || !(demigods_num % 4))
			{
				demigods += "\r\n";
			}
		}
		else
		{
			morts_num++;
			morts += buf;
			if (!short_list || !(morts_num % 4))
				morts += "\r\n";
		}
	}			// end of for

	if (morts_num + imms_num + demigods_num == 0)
	{
		send_to_char("\r\n�� ������ �� ������.\r\n", ch);
		// !!!
		return;
	}

	std::string out;

	if (imms_num > 0)
	{
		out += imms;
	}
	if (demigods_num > 0)
	{
		if (short_list)
		{
			out += "\r\n";
		}
		out += demigods;
	}
	if (morts_num > 0)
	{
		if (short_list)
		{
			out += "\r\n";
		}
		out += morts;
	}

	out += "\r\n�����:";
	if (imms_num)
	{
		sprintf(buf, " ����������� %d", imms_num);
		out += buf;
	}
	if (demigods_num)
	{
		sprintf(buf, " ����������������� %d", demigods_num);
		out += buf;
	}
	if (all && morts_num)
	{
		sprintf(buf, " �������� %d (������� %d)", all, morts_num);
		out += buf;
	}
	else if (morts_num)
	{
		sprintf(buf, " �������� %d", morts_num);
		out += buf;
	}

	out += ".\r\n";
	page_string(ch->desc, out);
}

std::string print_server_uptime()
{
	const auto boot_time = shutdown_parameters.get_boot_time();
	time_t diff = time(0) - boot_time;
	const int d = diff / 86400;
	const int h = (diff / 3600) % 24;
	const int m = (diff / 60) % 60;
	const int s = diff % 60;
	return boost::str(boost::format("������� � ������������: %d� %02d:%02d:%02d\r\n") % d % h % m % s);
}

void do_statistic(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int proff[NUM_PLAYER_CLASSES][2];
	int ptot[NUM_PLAYER_CLASSES];
	int i, clan = 0, noclan = 0, hilvl = 0, lowlvl = 0, all = 0, rem = 0, norem = 0, pk = 0, nopk = 0;

	for (i = 0; i < NUM_PLAYER_CLASSES; i++)
	{
		proff[i][0] = 0;
		proff[i][1] = 0;
		ptot[i] = 0;
	}

	for (const auto& tch : character_list)
	{
		if (IS_NPC(tch) || GET_LEVEL(tch) >= LVL_IMMORT || !HERE(tch))
			continue;

		if (CLAN(tch))
			clan++;
		else
			noclan++;
		if (GET_LEVEL(tch) >= 25)
			hilvl++;
		else
			lowlvl++;
		if (GET_REMORT(tch) >= 1)
			rem++;
		else
			norem++;
		all++;
		if (pk_count(tch.get()) >= 1)
		{
			pk++;
		}
		else
		{
			nopk++;
		}

		if (GET_LEVEL(tch) >= 25)
			proff[(int)GET_CLASS(tch)][0]++;
		else
			proff[(int)GET_CLASS(tch)][1]++;
		ptot[(int)GET_CLASS(tch)]++;
	}
	sprintf(buf, "%s���������� �� �������, ����������� � ���� (����� / 25 � ���� / ���� 25):%s\r\n", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "������        %s[%s%2d/%2d/%2d%s]%s       ",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_CLERIC], proff[CLASS_CLERIC][0], proff[CLASS_CLERIC][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "�������     %s[%s%2d/%2d/%2d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_BATTLEMAGE], proff[CLASS_BATTLEMAGE][0],
			proff[CLASS_BATTLEMAGE][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "����          %s[%s%2d/%2d/%2d%s]%s       ",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_THIEF], proff[CLASS_THIEF][0], proff[CLASS_THIEF][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "��������    %s[%s%2d/%2d/%2d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_WARRIOR], proff[CLASS_WARRIOR][0], proff[CLASS_WARRIOR][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "��������      %s[%s%2d/%2d/%2d%s]%s       ",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_ASSASINE], proff[CLASS_ASSASINE][0],
			proff[CLASS_ASSASINE][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "����������  %s[%s%2d/%2d/%2d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_GUARD], proff[CLASS_GUARD][0], proff[CLASS_GUARD][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "���������     %s[%s%2d/%2d/%2d%s]%s       ",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_CHARMMAGE], proff[CLASS_CHARMMAGE][0],
			proff[CLASS_CHARMMAGE][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "����������  %s[%s%2d/%2d/%2d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM),
			ptot[CLASS_DEFENDERMAGE], proff[CLASS_DEFENDERMAGE][0], proff[CLASS_DEFENDERMAGE][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "������������� %s[%s%2d/%2d/%2d%s]%s       ",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_NECROMANCER], proff[CLASS_NECROMANCER][0],
			proff[CLASS_NECROMANCER][1], CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "������      %s[%s%2d/%2d/%2d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_PALADINE], proff[CLASS_PALADINE][0],
			proff[CLASS_PALADINE][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "��������      %s[%s%2d/%2d/%2d%s]%s       ",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_RANGER], proff[CLASS_RANGER][0], proff[CLASS_RANGER][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "�������     %s[%s%2d/%2d/%2d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_SMITH], proff[CLASS_SMITH][0], proff[CLASS_SMITH][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "�����         %s[%s%2d/%2d/%2d%s]%s       ",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_MERCHANT], proff[CLASS_MERCHANT][0],
			proff[CLASS_MERCHANT][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "������      %s[%s%2d/%2d/%2d%s]%s\r\n\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), ptot[CLASS_DRUID], proff[CLASS_DRUID][0], proff[CLASS_DRUID][1],
			CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
			"������� ����|���� 25 ������     %s[%s%*d%s|%s%*d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, hilvl, CCIRED(ch,
					C_NRM),
			CCICYN(ch, C_NRM), 3, lowlvl, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
			"������� � ����������������|���  %s[%s%*d%s|%s%*d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, rem, CCIRED(ch, C_NRM),
			CCICYN(ch, C_NRM), 3, norem, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
			"��������|����������� �������    %s[%s%*d%s|%s%*d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, clan, CCIRED(ch,
					C_NRM),
			CCICYN(ch, C_NRM), 3, noclan, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf),
			"������� � ������� ��|��� ��     %s[%s%*d%s|%s%*d%s]%s\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, pk, CCIRED(ch,
					C_NRM),
			CCICYN(ch, C_NRM), 3, nopk, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(buf + strlen(buf), "����� ������� %s[%s%*d%s]%s\r\n\r\n",
			CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), 3, all, CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	char buf_[MAX_INPUT_LENGTH];
	std::string out;

	out += print_server_uptime();
	snprintf(buf_, sizeof(buf_),
		"������ (��� ��) | ������ �����  %s[%s%3d%s|%s %2d%s]%s\r\n",
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), char_stat::pkilled,
		CCIRED(ch, C_NRM), CCICYN(ch, C_NRM), char_stat::mkilled,
		CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	out += buf_;
	out += char_stat::print_class_exp(ch);

	send_to_char(out, ch);
}


#define USERS_FORMAT \
"������: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"
#define MAX_LIST_LEN 200
void do_users(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	const char *format = "%3d %-7s %-12s %-14s %-3s %-8s ";
	char line[200], line2[220], idletime[10], classname[20];
	char state[30] = "\0", *timeptr, mode;
	char name_search[MAX_INPUT_LENGTH] = "\0", host_search[MAX_INPUT_LENGTH];
// ����
	char host_by_name[MAX_INPUT_LENGTH] = "\0";
	DESCRIPTOR_DATA *list_players[MAX_LIST_LEN];
	DESCRIPTOR_DATA *d_tmp;
	int count_pl;
	int cycle_i, is, flag_change;
	unsigned long a1, a2;
	int showremorts = 0, showemail = 0, locating = 0;
	char sorting = '!';
	DESCRIPTOR_DATA *d;
	int low = 0, high = LVL_IMPL, num_can_see = 0;
	int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

	host_search[0] = name_search[0] = '\0';

	strcpy(buf, argument);
	while (*buf)
	{
		half_chop(buf, arg, buf1);
		if (*arg == '-')
		{
			mode = *(arg + 1);	// just in case; we destroy arg in the switch
			switch (mode)
			{
			case 'o':
			case 'k':
				outlaws = 1;
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'p':
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'd':
				deadweight = 1;
				strcpy(buf, buf1);
				break;
			case 'l':
				if (!IS_GOD(ch))
					return;
				playing = 1;
				half_chop(buf1, arg, buf);
				sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				playing = 1;
				half_chop(buf1, name_search, buf);
				break;
			case 'h':
				playing = 1;
				half_chop(buf1, host_search, buf);
				break;
			case 'u':
				playing = 1;
				half_chop(buf1, host_by_name, buf);
				break;
			case 'w':
				if (!IS_GRGOD(ch))
					return;
				playing = 1;
				locating = 1;
				strcpy(buf, buf1);
				break;
			case 'c':
			{
				playing = 1;
				half_chop(buf1, arg, buf);
				const size_t len = strlen(arg);
				for (size_t i = 0; i < len; i++)
				{
					showclass |= find_class_bitvector(arg[i]);
				}
				break;
			}
			case 'e':
				showemail = 1;
				strcpy(buf, buf1);
				break;
			case 'r':
				showremorts = 1;
				strcpy(buf, buf1);
				break;

			case 's':
				//sorting = 'i';
				sorting = *(arg + 2);
				strcpy(buf, buf1);
				break;
			default:
				send_to_char(USERS_FORMAT, ch);
				return;
			}	// end of switch

		}
		else  	// endif
		{
			strcpy(name_search, arg);
			strcpy(buf, buf1);
		}
	}			// end while (parser)
	if (showemail)
	{
		strcpy(line, "��� �������       ���         ���������       Idl �����    ����       E-mail\r\n");
	}
	else
	{
		strcpy(line, "��� �������       ���         ���������       Idl �����    ����\r\n");
	}
	strcat(line, "--- ---------- ------------ ----------------- --- -------- ----------------------------\r\n");
	send_to_char(line, ch);

	one_argument(argument, arg);

// ����
	if (strlen(host_by_name) != 0)
	{
		strcpy(host_search, "!");
	}

	for (d = descriptor_list, count_pl = 0; d && count_pl < MAX_LIST_LEN; d = d->next, count_pl++)
	{
		list_players[count_pl] = d;

		const auto character = d->get_character();
		if (!character)
		{
			continue;
		}

		if (isname(host_by_name, GET_NAME(character)))
		{
			strcpy(host_search, d->host);
		}
	}

	if (sorting != '!')
	{
		is = 1;
		while (is)
		{
			is = 0;
			for (cycle_i = 1; cycle_i < count_pl; cycle_i++)
			{
				flag_change = 0;
				d = list_players[cycle_i - 1];

				const auto t = d->get_character();

				d_tmp = list_players[cycle_i];

				const auto t_tmp = d_tmp->get_character();

				switch (sorting)
				{
				case 'n':
					if (0 < strcoll(t ? t->get_pc_name().c_str() : "", t_tmp ? t_tmp->get_pc_name().c_str() : ""))
					{
						flag_change = 1;
					}
					break;

				case 'e':
					if (strcoll(t ? GET_EMAIL(t) : "", t_tmp ? GET_EMAIL(t_tmp) : "") > 0)
						flag_change = 1;
					break;

				default:
					a1 = get_ip((const char *) d->host);
					a2 = get_ip((const char *) d_tmp->host);
					if (a1 > a2)
						flag_change = 1;
				}
				if (flag_change)
				{
					list_players[cycle_i - 1] = d_tmp;
					list_players[cycle_i] = d;
					is = 1;
				}
			}
		}
	}

	for (cycle_i = 0; cycle_i < count_pl; cycle_i++)
	{
		d = (DESCRIPTOR_DATA *) list_players[cycle_i];
// ---
		if (STATE(d) != CON_PLAYING && playing)
			continue;
		if (STATE(d) == CON_PLAYING && deadweight)
			continue;
		if (STATE(d) == CON_PLAYING)
		{
			const auto character = d->get_character();
			if (!character)
			{
				continue;
			}

			if (*host_search && !strstr(d->host, host_search))
				continue;
			if (*name_search && !isname(name_search, GET_NAME(character)))
				continue;
			if (!CAN_SEE(ch, character) || GET_LEVEL(character) < low || GET_LEVEL(character) > high)
				continue;
			if (outlaws && !PLR_FLAGGED((ch), PLR_KILLER))
				continue;
			if (showclass && !(showclass & (1 << GET_CLASS(character))))
				continue;
			if (GET_INVIS_LEV(character) > GET_LEVEL(ch))
				continue;

			if (d->original)
				if (showremorts)
					sprintf(classname, "[%2d %2d %s %s]", GET_LEVEL(d->original), GET_REMORT(d->original), KIN_ABBR(d->original), CLASS_ABBR(d->original));
				else
					sprintf(classname, "[%2d %s %s]   ", GET_LEVEL(d->original), KIN_ABBR(d->original), CLASS_ABBR(d->original));
			else
				if (showremorts)
					sprintf(classname, "[%2d %2d %s %s]", GET_LEVEL(d->character), GET_REMORT(d->character), KIN_ABBR(d->character), CLASS_ABBR(d->character));
				else
					sprintf(classname, "[%2d %s %s]   ", GET_LEVEL(d->character), KIN_ABBR(d->character), CLASS_ABBR(d->character));
		}
		else
		{
			strcpy(classname, "      -      ");
		}

		if (GET_LEVEL(ch) < LVL_IMPL && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			strcpy(classname, "      -      ");
		}

		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (STATE(d) == CON_PLAYING && d->original)
			strcpy(state, "Switched");
		else
			sprinttype(STATE(d), connected_types, state);

		if (d->character
			&& STATE(d) == CON_PLAYING
			&& !IS_GOD(d->character))
		{
			sprintf(idletime, "%3d", d->character->char_specials.timer *
				SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
		}
		else
		{
			strcpy(idletime, "");
		}

		if (d->character
			&& d->character->get_pc_name().c_str())
		{
			if (d->original)
			{
				sprintf(line, format, d->desc_num, classname, d->original->get_pc_name().c_str(), state, idletime, timeptr);
			}
			else
			{
				sprintf(line, format, d->desc_num, classname, d->character->get_pc_name().c_str(), state, idletime, timeptr);
			}
		}
		else
		{
			sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED", state, idletime, timeptr);
		}

// ����
		if (d && *d->host)
		{
			sprintf(line2, "[%s]", d->host);
			strcat(line, line2);
		}
		else
		{
			strcat(line, "[����������� ����]");
		}

		if (showemail)
		{
			sprintf(line2, "[&S%s&s]",
					d->original ? GET_EMAIL(d->original) : d->character ? GET_EMAIL(d->character) : "");
			strcat(line, line2);
		}

		if (locating && (*name_search || *host_by_name))
		{
			if (STATE(d) == CON_PLAYING)
			{
				const auto ci = d->get_character();
				if (ci
					&& CAN_SEE(ch, ci)
					&& ci->in_room != NOWHERE)
				{
					if (d->original && d->character)
					{
						sprintf(line2, " [%5d] %s (in %s)",
							GET_ROOM_VNUM(IN_ROOM(d->character)),
							world[d->character->in_room]->name, GET_NAME(d->character));
					}
					else
					{
						sprintf(line2, " [%5d] %s",
							GET_ROOM_VNUM(IN_ROOM(ci)), world[ci->in_room]->name);
					}
				}

				strcat(line, line2);
			}
		}

//--
		strcat(line, "\r\n");
		if (STATE(d) != CON_PLAYING)
		{
			sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
			strcpy(line, line2);
		}

		if (STATE(d) != CON_PLAYING || (STATE(d) == CON_PLAYING && d->character && CAN_SEE(ch, d->character)))
		{
			send_to_char(line, ch);
			num_can_see++;
		}
	}

	sprintf(line, "\r\n%d ������� ����������.\r\n", num_can_see);
	page_string(ch->desc, line, TRUE);
}

// Generic page_string function for displaying text
void do_gen_ps(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int subcmd)
{
	//DESCRIPTOR_DATA *d;
	switch (subcmd)
	{
	case SCMD_CREDITS:
		page_string(ch->desc, credits, 0);
		break;
	case SCMD_INFO:
		page_string(ch->desc, info, 0);
		break;
	case SCMD_IMMLIST:
		page_string(ch->desc, immlist, 0);
		break;
	case SCMD_HANDBOOK:
		page_string(ch->desc, handbook, 0);
		break;
	case SCMD_POLICIES:
		page_string(ch->desc, policies, 0);
		break;
	case SCMD_MOTD:
		page_string(ch->desc, motd, 0);
		break;
	case SCMD_RULES:
		page_string(ch->desc, rules, 0);
		break;
	case SCMD_CLEAR:
		send_to_char("\033[H\033[J", ch);
		break;
	case SCMD_VERSION:
		show_code_date(ch);
		break;
	case SCMD_WHOAMI:
	{
		//���������. ������.
		sprintf(buf, "�������� : %s\r\n", GET_NAME(ch));
		sprintf(buf + strlen(buf),
				"������ : &W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n\r\n",
				ch->get_name().c_str(), GET_PAD(ch, 1), GET_PAD(ch, 2),
				GET_PAD(ch, 3), GET_PAD(ch, 4), GET_PAD(ch, 5));

		sprintf(buf + strlen(buf), "��� e-mail : &S%s&s\r\n", GET_EMAIL(ch));
		time_t birt = ch->player_data.time.birth;
		sprintf(buf + strlen(buf), "���� ������ �������� : %s\r\n", rustime(localtime(&birt)));
		sprintf(buf + strlen(buf), "��� IP-����� : %s\r\n", ch->desc ? ch->desc->host : "Unknown");
//               GET_LASTIP (ch));
		send_to_char(buf, ch);
		if (!NAME_GOD(ch))
		{
			sprintf(buf, "��� ����� �� ��������!\r\n");
			send_to_char(buf, ch);
		}
		else
		{
			const int god_level = NAME_GOD(ch) > 1000 ? NAME_GOD(ch) - 1000 : NAME_GOD(ch);
			sprintf(buf1, "%s", get_name_by_id(NAME_ID_GOD(ch)));
			*buf1 = UPPER(*buf1);
			if (NAME_GOD(ch) < 1000)
				sprintf(buf, "&R��� ��������� %s %s&n\r\n", print_god_or_player(god_level), buf1);
			else
				sprintf(buf, "&W��� �������� %s %s&n\r\n", print_god_or_player(god_level), buf1);
			send_to_char(buf, ch);
		}
		sprintf(buf, "��������������: %d\r\n", GET_REMORT(ch));
		send_to_char(buf, ch);
		//����� ���������. ������.
		Clan::CheckPkList(ch);
		break;
	}
	default:
		log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
		return;
	}
}

void perform_mortal_where(CHAR_DATA * ch, char *arg)
{
	DESCRIPTOR_DATA *d;

	send_to_char("��� ����� �����, ��� ����� ����.\r\n", ch);
	return;

	if (!*arg)
	{
		send_to_char("������, ����������� � ����\r\n--------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next)
		{
			if (STATE(d) != CON_PLAYING
				|| d->character.get() == ch)
			{
				continue;
			}

			const auto i = d->get_character();
			if (!i)
			{
				continue;
			}

			if (i->in_room == NOWHERE
				|| !CAN_SEE(ch, i))
			{
				continue;
			}

			if (world[ch->in_room]->zone != world[i->in_room]->zone)
			{
				continue;
			}

			sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			send_to_char(buf, ch);
		}
	}
	else  		// print only FIRST char, not all.
	{
		for (const auto& i : character_list)
		{
			if (i->in_room == NOWHERE
				|| i.get() == ch)
			{
				continue;
			}

			if (!CAN_SEE(ch, i)
				|| world[i->in_room]->zone != world[ch->in_room]->zone)
			{
				continue;
			}

			if (!isname(arg, i->get_pc_name()))
			{
				continue;
			}

			sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			send_to_char(buf, ch);
			return;
		}
		send_to_char("������ �������� � ���� ������ ���.\r\n", ch);
	}
}

void print_object_location(int num, const OBJ_DATA * obj, CHAR_DATA * ch, int recur)
{
	if (num > 0)
	{
		sprintf(buf, "O%3d. %-25s - ", num, obj->get_short_description().c_str());
	}
	else
	{
		sprintf(buf, "%34s", " - ");
	}

	if (obj->get_in_room() > NOWHERE)
	{
		sprintf(buf + strlen(buf), "[%5d] %s", GET_ROOM_VNUM(obj->get_in_room()), world[obj->get_in_room()]->name);
		if (IS_GRGOD(ch))
		{
			sprintf(buf2, " Vnum ��������: %d", GET_OBJ_VNUM(obj));
			strcat(buf, buf2);
		}
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else if (obj->get_carried_by())
	{
		sprintf(buf + strlen(buf), "�������� %s[%d] � ������� [%d]",
			PERS(obj->get_carried_by(), ch, 4),
			GET_MOB_VNUM(obj->get_carried_by()),
			world[obj->get_carried_by()->in_room]->number);
			if (IS_GRGOD(ch))
			{
				sprintf(buf2, " Vnum ��������: %d", GET_OBJ_VNUM(obj));
				strcat(buf, buf2);
			}
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else if (obj->get_worn_by())
	{
		sprintf(buf + strlen(buf), "����� �� %s[%d] � ������� [%d]",
			PERS(obj->get_worn_by(), ch, 3),
			GET_MOB_VNUM(obj->get_worn_by()),
			world[obj->get_worn_by()->in_room]->number);
			if (IS_GRGOD(ch))
			{
				sprintf(buf2, " Vnum ��������: %d", GET_OBJ_VNUM(obj));
				strcat(buf, buf2);
			}
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else if (obj->get_in_obj())
	{
		if (Clan::is_clan_chest(obj->get_in_obj()))// || Clan::is_ingr_chest(obj->get_in_obj())) ������� ��������� �����
		{
			return; // ��� �� �������� ������ �� �����/������� - �� ������ �������� ���� ��������
		}
		else
		{
			sprintf(buf + strlen(buf), "����� � %s%s\r\n",
				obj->get_in_obj()->get_PName(5).c_str(),
				(recur ? ", ������� ��������� " : " "));
/*			if (IS_GRGOD(ch))
			{
				sprintf(buf2, " Vnum ��������: %d", GET_OBJ_VNUM(obj));
				strcat(buf, buf2);
			}
			strcat(buf, "\r\n");*/
			send_to_char(buf, ch);
			if (recur)
			{
				print_object_location(0, obj->get_in_obj(), ch, recur);
			}
		}
	}
	else
	{
		sprintf(buf + strlen(buf), "��������� ���-�� ���, ������-������.");
		if (IS_GRGOD(ch))
		{
			sprintf(buf2, " Vnum ��������: %d", GET_OBJ_VNUM(obj));
			strcat(buf, buf2);
		}
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
}

/**
* ������� ����� ������ �� '���' � �������� ��� �� ����������� ������, ���
* � �� ������� �������� � �����.
*/
bool print_imm_where_obj(CHAR_DATA *ch, char *arg, int num)
{
	bool found = false;

	world_objects.foreach([&](const OBJ_DATA::shared_ptr object)	/* maybe it is possible to create some index instead of linear search */
	{
		if (isname(arg, object->get_aliases()))
		{
			found = true;
			print_object_location(num++, object.get(), ch, TRUE);
		}
	});

	int tmp_num = num;
	if (IS_GOD(ch)
		|| PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		tmp_num = Clan::print_spell_locate_object(ch, tmp_num, arg);
		tmp_num = Depot::print_imm_where_obj(ch, arg, tmp_num);
		tmp_num = Parcel::print_imm_where_obj(ch, arg, tmp_num);
	}

	if (!found
		&& tmp_num == num)
	{
		return false;
	}
	else
	{
		num = tmp_num;
		return true;
	}
}

void perform_immort_where(CHAR_DATA * ch, char *arg)
{
	DESCRIPTOR_DATA *d;
	int num = 1, found = 0;

	if (!*arg)
	{
		if (GET_LEVEL(ch) < LVL_IMPL && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("��� ��� ���������?", ch);
		}
		else
		{
			send_to_char("������\r\n------\r\n", ch);
			for (d = descriptor_list; d; d = d->next)
			{
				if (STATE(d) == CON_PLAYING)
				{
					const auto i = d->get_character();
					if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE))
					{
						if (d->original)
						{
							sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n",
								GET_NAME(i),
								GET_ROOM_VNUM(IN_ROOM(d->character)),
								world[d->character->in_room]->name,
								GET_NAME(d->character));
						}
						else
						{
							sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i),
								GET_ROOM_VNUM(IN_ROOM(i)), world[i->in_room]->name);
						}
						send_to_char(buf, ch);
					}
				}
			}
		}
	}
	else
	{
		for (const auto& i : character_list)
		{
			if (CAN_SEE(ch, i)
				&& i->in_room != NOWHERE
				&& isname(arg, i->get_pc_name()))
			{
			    zone_data *zone = &zone_table[world[i->in_room]->zone];
				found = 1;
				sprintf(buf, "%s%3d. %-25s - [%5d] %s. �������� ����: '%s'\r\n", IS_NPC(i)? "���:  ":"�����:", num++, GET_NAME(i),
						GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)]->name, zone->name);
				send_to_char(buf, ch);
			}
		}

		if (!print_imm_where_obj(ch, arg, num)
			&& !found)
		{
			send_to_char("��� ������ ��������.\r\n", ch);
		}
	}
}

void do_where(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	one_argument(argument, arg);

	if (IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
		perform_immort_where(ch, arg);
	else
		perform_mortal_where(ch, arg);
}

void do_levels(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	int i;

	if (IS_NPC(ch))
	{
		send_to_char("���� ��� ��������� ��� �������.\r\n", ch);
		return;
	}
	*buf = '\0';

	sprintf(buf, "�������          ����            ���� �� ���.\r\n");
	for (i = 1; i < LVL_IMMORT; i++)
		sprintf(buf + strlen(buf), "[%2d] %13s-%-13s %-13s\r\n", i, thousands_sep(level_exp(ch, i)).c_str(),
			thousands_sep(level_exp(ch, i + 1) - 1).c_str(),
			thousands_sep((int) (level_exp(ch, i + 1) - level_exp(ch, i)) / (10 + GET_REMORT(ch))).c_str());

	sprintf(buf + strlen(buf), "[%2d] %13s               (����������)\r\n", LVL_IMMORT, thousands_sep(level_exp(ch, LVL_IMMORT)).c_str());
	page_string(ch->desc, buf, 1);
}

void do_consider(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	int diff;

	one_argument(argument, buf);

	if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
	{
		send_to_char("���� �� ������ �������?\r\n", ch);
		return;
	}
	if (victim == ch)
	{
		send_to_char("�����! �������� �������� <������� ��������>!\r\n", ch);
		return;
	}
	if (!IS_NPC(victim))
	{
		send_to_char("���������� ������� ���� - ��� � �� ��������.\r\n", ch);
		return;
	}
	diff = (GET_LEVEL(victim) - GET_LEVEL(ch) - GET_REMORT(ch));

	if (diff <= -10)
		send_to_char("���-����, ��� ��������.\r\n", ch);
	else if (diff <= -5)
		send_to_char("\"������� ��� ���� � ����!\"\r\n", ch);
	else if (diff <= -2)
		send_to_char("�����.\r\n", ch);
	else if (diff <= -1)
		send_to_char("������������ �����.\r\n", ch);
	else if (diff == 0)
		send_to_char("������ ��������!\r\n", ch);
	else if (diff <= 1)
		send_to_char("��� ����������� ������� �����!\r\n", ch);
	else if (diff <= 2)
		send_to_char("��� ����������� �������!\r\n", ch);
	else if (diff <= 3)
		send_to_char("����� � ������� ���������� ��� ������ ����������!\r\n", ch);
	else if (diff <= 5)
		send_to_char("�� ������ �� ���� ������� �����.\r\n", ch);
	else if (diff <= 10)
		send_to_char("�����, ������� ��� ���.\r\n", ch);
	else if (diff <= 100)
		send_to_char("������ � ��������� - �� ��������� ������ �������!\r\n", ch);

}

void do_diagnose(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;

	one_argument(argument, buf);

	if (*buf)
	{
		if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
			send_to_char(NOPERSON, ch);
		else
			diag_char_to_char(vict, ch);
	}
	else
	{
		if (ch->get_fighting())
			diag_char_to_char(ch->get_fighting(), ch);
		else
			send_to_char("�� ���� �� ������ ���������?\r\n", ch);
	}
}

const char *ctypes[] = { "��������", "�������", "�������", "������", "\n" };

void do_color(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int tp;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		sprintf(buf, "%s %s��������%s �����.\r\n", ctypes[COLOR_LEV(ch)], CCRED(ch, C_SPR), CCNRM(ch, C_OFF));
		send_to_char(CAP(buf), ch);
		return;
	}
	if ((tp = search_block(arg, ctypes, FALSE)) == -1)
	{
		send_to_char("������: [�����] ���� { ���� | ������� | ������� | ������ }\r\n", ch);
		return;
	}
	PRF_FLAGS(ch).unset(PRF_COLOR_1);
	PRF_FLAGS(ch).unset(PRF_COLOR_2);

	if (0 != (1 & tp))	// 1 or 3 (simple/full)
	{
		PRF_FLAGS(ch).set(PRF_COLOR_1);
	}
	if (0 != (2 & tp))	// 2 or 3 (normal/full)
	{
		PRF_FLAGS(ch).set(PRF_COLOR_2);
	}

	sprintf(buf, "%s %s��������%s �����.\r\n", ctypes[tp], CCRED(ch, C_SPR), CCNRM(ch, C_OFF));
	send_to_char(CAP(buf), ch);
}

void do_toggle(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch))
		return;
	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf2, "���");
	else
		sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

	if (GET_LEVEL(ch) >= LVL_IMMORT || PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		sprintf(buf,
				" ��� �����     : %-3s     "
				" ������������  : %-3s     "
				" ����� ������  : %-3s \r\n"
				" ������� ����� : %-3s     "
				" ����������    : %-3s     "
				" �����         : %-3s \r\n"
				" ��������      : %-3s \r\n",
				ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
				ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
				ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)),
				ONOFF(PRF_FLAGGED(ch, PRF_NOWIZ)),
				ONOFF(nameserver_is_slow),
				ONOFF(PRF_FLAGGED(ch, PRF_CODERINFO)),
				ONOFF(PRF_FLAGGED(ch, PRF_MISPRINT)));
		send_to_char(buf, ch);
	}

	sprintf(buf,
			" ����������    : %-3s     "
			" ������� ����� : %-3s     "
			" ������ �����  : %-3s \r\n"
			" ������ ������ : %-3s     "
			" ���������     : %-3s     "
			" ����          : %-8s\r\n"
			" ���-��        : %-6s  "
			" �������       : %-3s     "
			" �����         : %-3s \r\n"
			" �������       : %-3s     "
			" �����         : %-3s     "
			" ��������������: %-3s \r\n"
			" ������        : %-3s     "
			" ��������������: %-3s     "
			" ������ (���)  : %-7s \r\n"
			" ��� ��������� : %-3s     "
			" ���������     : %-3s     "
			" ����������    : %-7s \r\n"
			" ����� ����    : %-3s     "
			" �����         : %-3s     "
			" ��������      : %-3s \r\n"
			" ������ ������ : %-3d     "
			" ������ ������ : %-3d     "
			" ������        : %-6s \r\n"
			" ������� (���) : %-5s   "
			" �����         : %-3s     "
			" ���������     : %-10s\r\n"
			" ������        : %-3s     "
			" ��������      : %-3s     "
			" ��������      : %-10s\r\n"
			" ����������    : %-3s     "
			" ������        : %-3s     "
			" ������ �����  : %-3s \r\n"
			" �����������   : %-3s     "
			" ���������     : %-3u     ",
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
			ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
			ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
			YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOTELL)),
			ctypes[COLOR_LEV(ch)],
			PRF_FLAGGED(ch, PRF_NOINVISTELL) ? "������" : "�����",
			ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOHOLLER)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOEXCHANGE)),
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),
			ONOFF(PRF_FLAGGED(ch, PRF_SUMMONABLE)),
			ONOFF(PRF_FLAGGED(ch, PRF_GOAHEAD)),
			PRF_FLAGGED(ch, PRF_SHOWGROUP) ? "������" : "�������",
			ONOFF(PRF_FLAGGED(ch, PRF_NOCLONES)),
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
			PRF_FLAGGED(ch, PRF_AUTOLOOT) ? PRF_FLAGGED(ch, PRF_NOINGR_LOOT) ? "NO-INGR" : "ALL    " : "OFF    ",
			ONOFF(PRF_FLAGGED(ch, PRF_AUTOMONEY)),
			ONOFF(!PRF_FLAGGED(ch, PRF_NOARENA)),
			buf2,
			STRING_LENGTH(ch),
			STRING_WIDTH(ch),
#if defined(HAVE_ZLIB)
			ch->desc->deflate == NULL ? "���" : (ch->desc->mccp_version == 2 ? "MCCPv2" : "MCCPv1"),
#else
			"N/A",
#endif
			PRF_FLAGGED(ch, PRF_NEWS_MODE) ? "�����" : "�����",
			ONOFF(PRF_FLAGGED(ch, PRF_BOARD_MODE)),
			GetChestMode(ch).c_str(),
			ONOFF(PRF_FLAGGED(ch, PRF_PKL_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_POLIT_MODE)),
			PRF_FLAGGED(ch, PRF_PKFORMAT_MODE) ? "�������" : "������",
			ONOFF(PRF_FLAGGED(ch, PRF_WORKMATE_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_OFFTOP_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_ANTIDC_MODE)),
			ONOFF(PRF_FLAGGED(ch, PRF_NOINGR_MODE)),
			ch->remember_get_num());
	send_to_char(buf, ch);
	if (NOTIFY_EXCH_PRICE(ch) > 0)
	{
		sprintf(buf,  " �����������   : %-3ld \r\n", NOTIFY_EXCH_PRICE(ch));
	}
	else
	{
		sprintf(buf,  " �����������   : %-3s \r\n", "���");
	}
	send_to_char(buf, ch);

	sprintf(buf,
		" �����         : %-3s     "
		" ���� � ����   : %-3s     "
		" ������� (���) : %s\r\n"
		" ����������    : %-3s     "
		" ������        : %-3s     "
		" �������� IP   : %-3s",
		ONOFF(PRF_FLAGGED(ch, PRF_DRAW_MAP)),
		ONOFF(PRF_FLAGGED(ch, PRF_ENTER_ZONE)),
		(PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS) ? "�������" : "������"),
		ONOFF(PRF_FLAGGED(ch, PRF_AUTO_NOSUMMON)),
		ONOFF(PRF_FLAGGED(ch, PRF_MAPPER)),
		ONOFF(PRF_FLAGGED(ch, PRF_IPCONTROL)));
	send_to_char(buf, ch);
	if (GET_GOD_FLAG(ch, GF_TESTER))
		sprintf(buf, " ������        : %-3s\r\n", ONOFF(PRF_FLAGGED(ch, PRF_TESTER)));
	else
		sprintf(buf, "\r\n");
	send_to_char(buf, ch);
}

void do_zone(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (ch->desc
		&& !(IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !can_use_feat(ch, DARK_READING_FEAT))
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		MapSystem::print_map(ch);
	}

	print_zone_info(ch);

	if ((IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
		&& zone_table[world[ch->in_room]->zone].comment)
	{
		send_to_char(ch, "�����������: %s.\r\n",
			zone_table[world[ch->in_room]->zone].comment);
	}
}


struct sort_struct
{
	int sort_pos;
	byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
	int a, b, tmp;

	num_of_cmds = 0;

	 // first, count commands (num_of_commands is actually one greater than the
	 // number of commands; it inclues the '\n'.

	while (*cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	// create data array
	CREATE(cmd_sort_info, num_of_cmds);

	// initialize it
	for (a = 1; a < num_of_cmds; a++)
	{
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social = FALSE;
	}

	// the infernal special case
	cmd_sort_info[find_command("insult")].is_social = TRUE;

	// Sort.  'a' starts at 1, not 0, to remove 'RESERVED'
	for (a = 1; a < num_of_cmds - 1; a++)
		for (b = a + 1; b < num_of_cmds; b++)
			if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
					   cmd_info[cmd_sort_info[b].sort_pos].command) > 0)
			{
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
}

void do_commands(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	int no, i, cmd_num, num_of;
	int wizhelp = 0, socials = 0;
	CHAR_DATA *vict = ch;

	one_argument(argument, arg);

	if (subcmd == SCMD_SOCIALS)
		socials = 1;
	else if (subcmd == SCMD_WIZHELP)
		wizhelp = 1;

	sprintf(buf, "��������� %s%s �������� %s:\r\n",
			wizhelp ? "����������������� " : "",
			socials ? "�������" : "�������", vict == ch ? "���" : GET_PAD(vict, 2));

	if (socials)
		num_of = number_of_social_commands;
	else
		num_of = num_of_cmds - 1;

	// cmd_num starts at 1, not 0, to remove 'RESERVED'
	for (no = 1, cmd_num = socials ? 0 : 1; cmd_num < num_of; cmd_num++)
		if (socials)
		{
			sprintf(buf + strlen(buf), "%-19s", soc_keys_list[cmd_num].keyword);
			if (!(no % 4))
				strcat(buf, "\r\n");
			no++;
		}
		else
		{
			i = cmd_sort_info[cmd_num].sort_pos;
			if (cmd_info[i].minimum_level >= 0
					&& (Privilege::can_do_priv(vict, std::string(cmd_info[i].command), i, 0))
					&& (cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp
					&& (wizhelp || socials == cmd_sort_info[i].is_social))
			{
				sprintf(buf + strlen(buf), "%-15s", cmd_info[i].command);
				if (!(no % 5))
					strcat(buf, "\r\n");
				no++;
			}
		}

	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}

void do_affects(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	char sp_name[MAX_STRING_LENGTH];
	const auto hide_affs = make_array<EAffectFlag>(EAffectFlag::AFF_SNEAK, EAffectFlag::AFF_HIDE, EAffectFlag::AFF_CAMOUFLAGE);

	for (const auto& j : hide_affs)
	{
		AFF_FLAGS(ch).unset(j);
	}

	ch->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, ",");

	sprintf(buf, "�������: %s%s%s\r\n", CCIYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	for (const auto& j : hide_affs)
	{
		const uint32_t i = to_underlying(j);
		if (ch->char_specials.saved.affected_by.get(i))
		{
			AFF_FLAGS(ch).set(j);
		}
	}

	// Routine to show what spells a char is affected by
	if (!ch->affected.empty())
	{
		for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end(); ++affect_i)
		{
			const auto aff = *affect_i;

			if (aff->type == SPELL_SOLOBONUS)
			{
				continue;
			}

			*buf2 = '\0';
			strcpy(sp_name, spell_name(aff->type));
			int mod = 0;
			if (aff->battleflag == AF_PULSEDEC)
			{
				mod = aff->duration / 51; //���� � ������� �������� � ����� 25.5 � ��� 2 ������
			}
			else
			{
				mod = aff->duration;
			}
			(mod + 1) / SECS_PER_MUD_HOUR
				? sprintf(buf2, "(%d %s)", (mod + 1) / SECS_PER_MUD_HOUR + 1, desc_count((mod + 1) / SECS_PER_MUD_HOUR + 1, WHAT_HOUR))
				: sprintf(buf2, "(����� ����)");
			sprintf(buf, "%s%s%-21s %-12s%s ",
					*sp_name == '!' ? "���������  : " : "���������� : ",
					CCICYN(ch, C_NRM), sp_name, buf2, CCNRM(ch, C_NRM));
			*buf2 = '\0';
			if (!IS_IMMORTAL(ch))
			{
				auto next_affect_i = affect_i;
				++next_affect_i;
				if (next_affect_i != ch->affected.end())
				{
					const auto& next_affect = *next_affect_i;
					if (aff->type == next_affect->type)
					{
						continue;
					}
				}
			}
			else
			{
				if (aff->modifier)
				{
					sprintf(buf2, "%-3d � ���������: %s", aff->modifier, apply_types[(int) aff->location]);
					strcat(buf, buf2);
				}
				if (aff->bitvector)
				{
					if (*buf2)
					{
						strcat(buf, ", ������������� ");
					}
					else
					{
						strcat(buf, "������������� ");
					}
					strcat(buf, CCIRED(ch, C_NRM));
					sprintbit(aff->bitvector, affected_bits, buf2);
					strcat(buf, buf2);
					strcat(buf, CCNRM(ch, C_NRM));
				}
			}
			send_to_char(strcat(buf, "\r\n"), ch);
		}
// ����������� ������
		for (const auto& aff : ch->affected)
		{
		    if (aff->type == SPELL_SOLOBONUS)
		    {
				int mod;
				if (aff->battleflag == AF_PULSEDEC)
				{
					mod = aff->duration / 51; //���� � ������� �������� � �����	25.5 � ��� 2 ������
				}
				else
				{
					mod = aff->duration;
				}
				(mod + 1) / SECS_PER_MUD_HOUR
					? sprintf(buf2, "(%d %s)", (mod + 1) / SECS_PER_MUD_HOUR + 1, desc_count((mod + 1) / SECS_PER_MUD_HOUR + 1, WHAT_HOUR))
					: sprintf(buf2, "(����� ����)");
			    sprintf(buf, "���������� : %s%-21s %-12s%s ", CCICYN(ch, C_NRM),  "�������",  buf2, CCNRM(ch, C_NRM));
			    *buf2 = '\0';
			    if (aff->modifier)
			    {	
				    sprintf(buf2, "%s%-3d � ���������: %s%s%s",(aff->modifier > 0)? "+": "",  aff->modifier, CCIRED(ch, C_NRM), apply_types[(int) aff->location], CCNRM(ch, C_NRM));
				    strcat(buf, buf2);
			    }
			    send_to_char(strcat(buf, "\r\n"), ch);
		    }
		}
	}

	if (ch->is_morphed())
	{
		*buf2 = '\0';
		send_to_char("����������� �������� �����: " , ch);
		const IMorph::affects_list_t& affs = ch->GetMorphAffects();
		for (auto it = affs.begin(); it != affs.end();)
		{
			sprintbit(to_underlying(*it), affected_bits, buf2);
			send_to_char(std::string(CCIYEL(ch, C_NRM))+ std::string(buf2)+ std::string(CCNRM(ch, C_NRM)), ch);
			if (++it != affs.end())
			{
				send_to_char(", ", ch);
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
