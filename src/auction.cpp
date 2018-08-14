/**************************************************************************
*   File: Auction.cpp                                  Part of Bylins     *
*  Usage: Auction functions used by the MUD                               *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "obj.hpp"
#include "screen.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "auction.h"
#include "constants.h"
#include "char.hpp"
#include "room.hpp"
#include "named_stuff.hpp"
#include "pk.h"
#include "utils.h"
#include "structs.h"
#include "conf.h"
#include "sysdep.h"

// external functions
extern int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear);
extern char *diag_timer_to_char(const OBJ_DATA* obj);
extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
extern void obj_info(CHAR_DATA * ch, OBJ_DATA *obj, char buf[MAX_STRING_LENGTH]);
extern void imm_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch);
extern void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness);


AUCTION_DATA auction_lots[MAX_AUCTION_LOT] = { { -1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
	{ -1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
	{ -1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0}	/*,
	{-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
	{-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0}  */
};

const char *tact_message[] = { "���!",
							   "���!!",
							   "�-�-�-�-�!!!",
							   "������!!!!",
							   "����!!!!!",
							   "\n"
							 };

const char *auction_cmd[] = { "���������", "set",
							  "�����", "close",
							  "������", "value",
							  "�������", "sell",
							  "���������", "transport",
							  "�����������", "examine",
							  "��������������", "identify",
							  "\n"
							};

void showlots(CHAR_DATA * ch)
{
	char tmpbuf[MAX_INPUT_LENGTH];

	CHAR_DATA *sch;
	//CHAR_DATA *bch;
	OBJ_DATA *obj;

	for (int i = 0; i < MAX_AUCTION_LOT; i++)
	{
		sch = GET_LOT(i)->seller;
		//bch = GET_LOT(i)->buyer;
		obj = GET_LOT(i)->item;

		if (!sch || !obj)
		{
			send_to_char(ch, "������� : ��� %2d - ��������.\r\n", i);
			continue;
		}
		if (GET_LOT(i)->prefect && GET_LOT(i)->prefect != ch)
		{
			sprintf(tmpbuf, "������� : ��� %2d - %s%s%s (������� �����).\r\n",
					i, CCIYEL(ch, C_NRM), obj->get_PName(0).c_str(), CCNRM(ch, C_NRM));
			send_to_char(tmpbuf, ch);
			continue;
		}

		sprintf(tmpbuf, "������� : ��� %2d - %s%s%s - ������ %d %s, ������� %d, �������� %s.\r\n",
			i, CCIYEL(ch, C_NRM), obj->get_PName(0).c_str(), CCNRM(ch, C_NRM),
			GET_LOT(i)->cost, desc_count(GET_LOT(i)->cost, WHAT_MONEYa),
			GET_LOT(i)->tact < 0 ? 1 : GET_LOT(i)->tact + 1, GET_NAME(sch));

		if (GET_LOT(i)->prefect && GET_LOT(i)->prefect_unique == GET_UNIQUE(ch))
		{
			strcat(tmpbuf, "(���������� ��� ���).\r\n");
		}
		send_to_char(tmpbuf, ch);
	}
}

bool auction_drive(CHAR_DATA * ch, char *argument)
{
	int mode = -1, value = -1, lot = -1;
	CHAR_DATA *tch = NULL;
	AUCTION_DATA *lotis;
	OBJ_DATA *obj;
	char operation[MAX_INPUT_LENGTH], whom[MAX_INPUT_LENGTH];
	char tmpbuf[MAX_INPUT_LENGTH];

	if (!*argument)
	{
		showlots(ch);
		return false;
	}
	argument = one_argument(argument, operation);
	if ((mode = search_block(operation, auction_cmd, FALSE)) < 0)
	{
		send_to_char("������� �������� : ���������, �����, ������, �������, ���������, ��������������, �����������.\r\n", ch);
		return false;
	}
	mode >>= 1;
	switch (mode)
	{
	case 0:		// Set lot
		if (!(lotis = free_auction(&lot)))
		{
			send_to_char("��� ��������� ��������.\r\n", ch);
			return false;
		}
		*operation = '\0';
		*whom = '\0';
		if (!sscanf(argument, "%s %d %s", operation, &value, whom))
		{
			send_to_char("������: ������� ��������� ���� [���. ������] [��� ����]\r\n", ch);
			return false;
		}
		if (!*operation)
		{
			send_to_char("�� ������ �������.\r\n", ch);
			return false;
		}
		if (!(obj = get_obj_in_list_vis(ch, operation, ch->carrying)))
		{
			send_to_char("� ��� ����� ���.\r\n", ch);
			return false;
		}
		if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_BOOK)
		{
			if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NORENT)
					|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_NOSELL))
			{
				send_to_char("���� ������� �� ������������ ��� ��������.\r\n", ch);
				return false;
			}
		}
		if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_DECAY)
			|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_NODROP)
			|| GET_OBJ_COST(obj) <= 0
			|| obj->get_owner() > 0)
		{
			send_to_char("���� ������� �� ������������ ��� ��������.\r\n", ch);
			return false;
		}
		if (obj_on_auction(obj))
		{
			send_to_char("�� ��� ��������� �� ������� ���� �������.\r\n", ch);
			return false;
		}
		if (obj->get_contains())
		{
			sprintf(tmpbuf, "���������� %s ����� ��������.\r\n", obj->get_PName(3).c_str());
			send_to_char(tmpbuf, ch);
			return false;
		}
		if (IS_GOD(ch))
		{
			sprintf(tmpbuf, "&C����� ���-������ �������� ��� ����, ������� ��� ���� ������� ��� �����!\n\r\n");
			send_to_char(tmpbuf, ch);
			return false;
		}
		if (value <= 0)
		{
			value = MAX(1, GET_OBJ_COST(obj));
		};
		if (*whom)
		{
			if (!(tch = get_player_vis(ch, whom, FIND_CHAR_WORLD)))
			{
				send_to_char("�� �� ������ ����� ������.\r\n", ch);
				return false;
			}
			/*	  if (IS_NPC (tch))
				    {
				      send_to_char ("� ���� ��������� ����������� ����.\r\n", ch);
				      return false;
				    }*/
			if (ch == tch)
			{
				send_to_char("�� ��� �� ��!\r\n", ch);
				return false;
			}
		};
		lotis->item_id = obj->get_id();
		lotis->item = obj;
		lotis->cost = value;
		lotis->tact = -1;
		lotis->seller_unique = GET_UNIQUE(ch);
		lotis->seller = ch;
		lotis->buyer_unique = lotis->prefect_unique = -1;
		lotis->buyer = lotis->prefect = NULL;
		if (tch)
		{
			lotis->prefect_unique = GET_UNIQUE(tch);
			lotis->prefect = tch;
		}

		if (tch)
		{
			sprintf(tmpbuf, "�� ��������� �� ������� $O3 �� %d %s (��� %s)",
					value, desc_count(value, WHAT_MONEYu), GET_PAD(tch, 1));
		}
		else
		{
			sprintf(tmpbuf, "�� ��������� �� ������� $O3 �� %d %s", value, desc_count(value, WHAT_MONEYu));
		}
		act(tmpbuf, FALSE, ch, 0, obj, TO_CHAR);
		sprintf(tmpbuf,
				"������� : ����� ��� %d - %s - ��������� ������ %d %s. \r\n",
				lot, obj->get_PName(0).c_str(), value, desc_count(value, WHAT_MONEYa));
		message_auction(tmpbuf, NULL);
		set_wait(ch, 1, FALSE);
		return true;
		break;
	case 1:		// Close
		if (!sscanf(argument, "%d", &lot))
		{
			send_to_char("�� ������ ����� ����.\r\n", ch);
			return false;
		}
		if (lot < 0 || lot >= MAX_AUCTION_LOT)
		{
			send_to_char("�������� ����� ����.\r\n", ch);
			return false;
		}
		if (GET_LOT(lot)->seller != ch || GET_LOT(lot)->seller_unique != GET_UNIQUE(ch))
		{
			send_to_char("��� �� ��� ���.\r\n", ch);
			return false;
		}
		act("�� ����� $O3 � ��������.\r\n", FALSE, ch, 0, GET_LOT(lot)->item, TO_CHAR);
		sprintf(tmpbuf, "������� : ��� %d(%s) ����%s � �������� ����������.\r\n", lot,
			GET_LOT(lot)->item->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_LOT(lot)->item));
		clear_auction(lot);
		message_auction(tmpbuf, NULL);
		set_wait(ch, 1, FALSE);
		return true;
		break;
	case 2:		// Set
		if (sscanf(argument, "%d %d", &lot, &value) != 2)
		{
			send_to_char("������: ������� ������ ��� �����.����\r\n", ch);
			return false;
		}
		if (lot < 0 || lot >= MAX_AUCTION_LOT)
		{
			send_to_char("�������� ����� ����.\r\n", ch);
			return false;
		}
		if (!GET_LOT(lot)->item || GET_LOT(lot)->item_id <= 0 ||
				!GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0)
		{
			send_to_char("��� ����.\r\n", ch);
			return false;
		}
		if (GET_LOT(lot)->seller == ch || GET_LOT(lot)->seller_unique == GET_UNIQUE(ch))
		{
			send_to_char("�� ��� �� ��� ���!\r\n", ch);
			return false;
		}
		if (GET_LOT(lot)->prefect && GET_LOT(lot)->prefect_unique > 0 &&
				(GET_LOT(lot)->prefect != ch || GET_LOT(lot)->prefect_unique != GET_UNIQUE(ch)))
		{
			send_to_char("���� ��� ����� ������� ����������.\r\n", ch);
			return false;
		}
		if (GET_LOT(lot)->item->get_carried_by() != GET_LOT(lot)->seller)
		{
			send_to_char("���� ������� ����������.\r\n", ch);
			sprintf(tmpbuf, "������� : ��� %d (%s) ����, ����� ����� ���������.", lot,
				GET_LOT(lot)->item->get_PName(0).c_str());
			clear_auction(lot);
			message_auction(tmpbuf, NULL);
			return true;
		}
		if (value < GET_LOT(lot)->cost)
		{
			send_to_char("���� ������ ���� �������.\r\n", ch);
			return false;
		}
		if (GET_LOT(lot)->buyer && value < GET_LOT(lot)->cost + MAX(1, GET_LOT(lot)->cost / 20))
		{
			send_to_char("��������� ������ �� ���� 5% �������.\r\n", ch);
			return false;
		}
		if (value > ch->get_gold() + ch->get_bank())
		{
			send_to_char("� ��� ��� ����� �����.\r\n", ch);
			return false;
		}
		GET_LOT(lot)->cost = value;
		GET_LOT(lot)->tact = -1;
		GET_LOT(lot)->buyer = ch;
		GET_LOT(lot)->buyer_unique = GET_UNIQUE(ch);
		sprintf(tmpbuf, "������, �� �������� ��������� %d %s �� %s (��� %d).\r\n",
			value, desc_count(value, WHAT_MONEYu), GET_LOT(lot)->item->get_PName(3).c_str(), lot);
		send_to_char(tmpbuf, ch);
		sprintf(tmpbuf, "������� ������ %s �� ��� %d(%s) %d %s.\r\n",
			GET_PAD(ch, 1), lot, GET_LOT(lot)->item->get_PName(0).c_str(), value, desc_count(value, WHAT_MONEYa));
		send_to_char(tmpbuf, GET_LOT(lot)->seller);
		sprintf(tmpbuf, "������� : ��� %d(%s) - ����� ������ %d %s.", lot,
			GET_LOT(lot)->item->get_PName(0).c_str(), value, desc_count(value, WHAT_MONEYa));
		message_auction(tmpbuf, NULL);
		set_wait(ch, 1, FALSE);
		return true;
		break;

	case 3:		// Sell
		if (!sscanf(argument, "%d", &lot))
		{
			send_to_char("�� ������ ����� ����.\r\n", ch);
			return false;
		}
		if (lot < 0 || lot >= MAX_AUCTION_LOT)
		{
			send_to_char("�������� ����� ����.\r\n", ch);
			return false;
		}
		if (GET_LOT(lot)->seller != ch || GET_LOT(lot)->seller_unique != GET_UNIQUE(ch))
		{
			send_to_char("��� �� ��� ���.\r\n", ch);
			return false;
		}
		if (!GET_LOT(lot)->buyer)
		{
			send_to_char("���������� �� ��� ����� ���� ���.\r\n", ch);
			return false;
		}

		GET_LOT(lot)->prefect = GET_LOT(lot)->buyer;
		GET_LOT(lot)->prefect_unique = GET_LOT(lot)->buyer_unique;
		if (GET_LOT(lot)->tact < MAX_AUCTION_TACT_BUY)
		{
			sprintf(whom, "������� : ��� %d(%s) ������ � �������� �� %d %s.",
					lot, GET_LOT(lot)->item->get_PName(0).c_str(), GET_LOT(lot)->cost,
					desc_count(GET_LOT(lot)->cost, WHAT_MONEYu));
			GET_LOT(lot)->tact = MAX_AUCTION_TACT_BUY;
		}
		else
			*whom = '\0';
		sell_auction(lot);
		if (*whom)
		{
			strcpy(tmpbuf, whom);
			message_auction(tmpbuf, NULL);
			return true;
		}
		set_wait(ch, 1, FALSE);
		return false;
		break;
	case 4:		// Transport //
		if (!sscanf(argument, "%d", &lot))
		{
			send_to_char("�� ������ ����� ���� ��� ��������.\r\n", ch);
			return false;
		}
		if (lot < 0 || lot >= MAX_AUCTION_LOT)
		{
			send_to_char("�������� ����� ����.\r\n", ch);
			return false;
		}
		if (!GET_LOT(lot)->item ||
				GET_LOT(lot)->item_id <= 0 || !GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0)
		{
			send_to_char("��� ����.\r\n", ch);
			return false;
		}

		if (GET_LOT(lot)->seller == ch && GET_LOT(lot)->seller_unique == GET_UNIQUE(ch))
		{
			send_to_char("��� ����� ������� ������ ����������.\r\n", ch);
			return false;
		}

		if (GET_LOT(lot)->prefect != ch || GET_LOT(lot)->prefect_unique != GET_UNIQUE(ch))
		{
			send_to_char("�� ��� �� ������ ���� �������.\r\n", ch);
			return false;
		}

		if IS_IMMORTAL(ch)
		{
			send_to_char("�������, �� ����� ���� ���?.\r\n", ch);
			return false;
		}
		trans_auction(lot);
		return true;
		break;
	case 5:		//Info
		OBJ_DATA * obj;
		if (!sscanf(argument, "%d", &lot))
		{
			send_to_char("�� ������ ����� ���� ��� ��������.\r\n", ch);
			return false;
		}

		if (lot < 0 || lot >= MAX_AUCTION_LOT)
		{
			send_to_char("�������� ����� ����.\r\n", ch);
			return false;
		}

		if (!GET_LOT(lot)->item ||
				GET_LOT(lot)->item_id <= 0 || !GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0)
		{
			send_to_char("��� ����.\r\n", ch);
			return false;
		}
		if (GET_LEVEL(GET_LOT(lot)->seller) >= LVL_IMMORT)
		{
			send_to_char("������������ ���� ������������.\r\n", ch);
			return false;
		}
		obj = GET_LOT(lot)->item;
		sprintf(buf, "������� \"%s\", ", obj->get_short_description().c_str());
		if ((GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WAND)
			|| (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF))
		{
			if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
			{
				strcat(buf, "(�/�), ");
			}
		}
		strcat(buf, " ��� ");
		sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
		if (*buf2)
		{
			strcat(buf, buf2);
			strcat(buf, "\n");
		};
		strcat(buf, diag_weapon_to_char(obj, TRUE));
		strcat(buf, diag_timer_to_char(obj));
		obj_info(ch, obj, buf);
		strcat(buf, "\n");
		if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj) || NamedStuff::check_named(ch, obj, false))
		{
			sprintf(buf2, "��� ���� ��� ����������!");
			strcat(buf, buf2);
			strcat(buf, "\n");
		}
		if ((!IS_NPC(ch) && invalid_align(ch, obj))
				|| invalid_no_class(ch, obj))
		{
			sprintf(buf2, "�� �� ������� ������������ ���� �����.");
			strcat(buf, buf2);
			strcat(buf, "\n");
		}
		send_to_char(buf, ch);
		return true;
		break;
	case 6:		//Identify
		OBJ_DATA * iobj;
		if (!sscanf(argument, "%d", &lot))
		{
			send_to_char("�� ������ ����� ���� ��� ��������.\r\n", ch);
			return false;
		}

		if (lot < 0 || lot >= MAX_AUCTION_LOT)
		{
			send_to_char("�������� ����� ����.\r\n", ch);
			return false;
		}

		if (!GET_LOT(lot)->item || GET_LOT(lot)->item_id <= 0 ||
			!GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0)
		{
			send_to_char("��� ����.\r\n", ch);
			return false;
		}

		if (GET_LOT(lot)->seller == ch || GET_LOT(lot)->seller_unique == GET_UNIQUE(ch))
		{
			send_to_char("�� ��� �� ��� ���!\r\n", ch);
			return false;
		}

		if (GET_LEVEL(GET_LOT(lot)->seller) >= LVL_IMMORT)
		{
			send_to_char("������������ ���� ������������.\r\n", ch);
			return false;
		}

		iobj = GET_LOT(lot)->item;

		if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
		{
			send_to_char("�������, � ���� �������� �� ��� ������ ������.\r\n", ch);
			return false;
		}
		if ((ch->get_total_gold() < AUCTION_IDENT_PAY) && (GET_LEVEL(ch) < LVL_IMPL))
		{
			send_to_char("� ��� �� ������ �� ��� �����!\r\n", ch);
			return false;
		}
		if (GET_LEVEL(ch) < LVL_IMPL)
			mort_show_obj_values(iobj, ch, 200);	//200 - ������ ���������
		else
			imm_show_obj_values(iobj, ch);

		ch->remove_both_gold(AUCTION_IDENT_PAY);
		send_to_char(ch, "\r\n%s�� ���������� � �������� � ������ ����� ����� %d %s%s\r\n",
		CCIGRN(ch, C_NRM), AUCTION_IDENT_PAY, desc_count(AUCTION_IDENT_PAY, WHAT_MONEYu), CCNRM(ch, C_NRM));

		return true;
		break;
	}
	return false;
}

void message_auction(char *message, CHAR_DATA * ch)
{
	DESCRIPTOR_DATA *i;

	// now send all the strings out
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING &&
				(!ch || i != ch->desc) &&
				i->character &&
				!PRF_FLAGGED(i->character, PRF_NOAUCT) &&
				!PLR_FLAGGED(i->character, PLR_WRITING) &&
				!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && GET_POS(i->character) > POS_SLEEPING)
		{
			if (COLOR_LEV(i->character) >= C_NRM)
			{
				send_to_char("&Y&q", i->character.get());
			}

			act(message, FALSE, i->character.get(), 0, 0, TO_CHAR | TO_SLEEP);

			if (COLOR_LEV(i->character) >= C_NRM)
			{
				send_to_char("&Q&n", i->character.get());
			}
		}
	}
}

void clear_auction(int lot)
{
	if (lot < 0 || lot >= MAX_AUCTION_LOT)
		return;
	GET_LOT(lot)->seller = GET_LOT(lot)->buyer = GET_LOT(lot)->prefect = NULL;
	GET_LOT(lot)->seller_unique = GET_LOT(lot)->buyer_unique = GET_LOT(lot)->prefect_unique = -1;
	GET_LOT(lot)->item = NULL;
	GET_LOT(lot)->item_id = -1;
}

int check_sell(int lot)
{
	CHAR_DATA *ch, *tch;
	OBJ_DATA *obj;
	char tmpbuf[MAX_INPUT_LENGTH];

	if (lot < 0 || lot >= MAX_AUCTION_LOT || !(ch = GET_LOT(lot)->seller)
			|| GET_UNIQUE(ch) != GET_LOT(lot)->seller_unique || !(tch = GET_LOT(lot)->buyer)
			|| GET_UNIQUE(tch) != GET_LOT(lot)->buyer_unique || !(obj = GET_LOT(lot)->item)
			|| obj->get_id() != GET_LOT(lot)->item_id)
		return (FALSE);

	if (obj->get_carried_by() != ch)
	{
		sprintf(tmpbuf, "������� : ��� %d(%s) ����, ����� ����� ���������", lot, obj->get_PName(0).c_str());
		message_auction(tmpbuf, NULL);
		clear_auction(lot);
		return (FALSE);
	}

	if (obj->get_contains())
	{
		sprintf(tmpbuf, "���������� %s ����� ��������.\r\n", obj->get_PName(3).c_str());
		send_to_char(tmpbuf, ch);
		if (GET_LOT(lot)->tact >= MAX_AUCTION_TACT_PRESENT)
		{
			sprintf(tmpbuf, "������� : ��� %d(%s) ���� � �������� �������������� ������.", lot, obj->get_PName(0).c_str());
			message_auction(tmpbuf, NULL);
			clear_auction(lot);
			return (FALSE);
		}
	}

	if (tch->get_total_gold() < GET_LOT(lot)->cost)
	{
		sprintf(tmpbuf, "� ��� �� ������� ����� �� ������� %s.\r\n", obj->get_PName(1).c_str());
		send_to_char(tmpbuf, tch);
		sprintf(tmpbuf, "� ���������� %s �� ������� �����.\r\n", obj->get_PName(1).c_str());
		send_to_char(tmpbuf, ch);
		sprintf(tmpbuf, "������� : ��� %d(%s) ���� � �������� �������������� ������.", lot, obj->get_PName(0).c_str());
		message_auction(tmpbuf, NULL);
		clear_auction(lot);
		return (FALSE);
	}
	// ������� �����.
	return (TRUE);
}

void trans_auction(int lot)
{
	CHAR_DATA *ch, *tch;
	OBJ_DATA *obj;
	std::string tmpstr;
	char tmpbuff[MAX_INPUT_LENGTH];

	ch = GET_LOT(lot)->seller;
	tch = GET_LOT(lot)->prefect;
	obj = GET_LOT(lot)->item;

	if (!check_sell(lot))
		return;

	// ���� ����� ����� 1000 ���.
	if (GET_LOT(lot)->cost < 1000)
	{
		send_to_char("��������� �������� ������� ����, ����� ��� ��������.\r\n", tch);
		return;
	}
	// � ���������� ���� 10% ����� �� �����.
	if (tch->get_total_gold() < (GET_LOT(lot)->cost + GET_LOT(lot)->cost / 10))
	{
		send_to_char("� ��� �� ������� ����� �� �������� ��������.", tch);
		return;
	}

	if (ch->in_room == IN_ROOM(tch))
	{
		// �������� �� ���������� � ����� �������.
		tmpstr = "$n ����� ����� � ����.";
		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_VICT | TO_SLEEP);
		return;
	};
	// ��������� ������� �������� ��������.
	// ��� ���� � �����
	// ��� ���� ��� ��
	if (RENTABLE(ch))
	{
		tmpstr = "��������� ������ �������� ��� �������� " + obj->get_PName(1) + " $N2.\r\n";

		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_CHAR | TO_SLEEP);

		tmpstr = "$n2 ���������� ��������� ������ �������� ��� �������� " + obj->get_PName(1) + " ���.\r\n";

		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_VICT | TO_SLEEP);
		return;
	}

	if (RENTABLE(tch))
	{
		tmpstr = "��������� ������ �������� ��� ��������� ����� �� $n1.\r\n";

		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_VICT | TO_SLEEP);

		tmpstr = "$N2 ���������� ��������� ������ �������� ��� ��������� ����� �� ���.";
		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_CHAR | TO_SLEEP);
		return;
	}

	if (!bloody::handle_transfer(tch, ch, obj))
	{
		act("$N2 ����� ������� ����� ����� � ������.", FALSE, ch, 0, tch, TO_CHAR | TO_SLEEP);
		return;
	}

	if (!is_post(ch->in_room))
	{
		// �������� �� �� ��� �������� �� �����.
		tmpstr = "��� ���������� ������� � ��������� ��� ��� �������� " + obj->get_PName(1) + " $N2.\r\n";

		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_CHAR | TO_SLEEP);

		tmpstr = "$N2 ���������� ������� � ��������� ��� ��� �������� " + obj->get_PName(1) + " ���.\r\n";

		act(tmpstr.c_str(), FALSE, tch, 0, ch, TO_CHAR | TO_SLEEP);
		return;
	}

	if (!is_post(IN_ROOM(tch)))
	{
		// �������� �� �� ��� �������� �� �����.
		tmpstr = "��� ���������� ������� � ��������� ��� ��� �������� ����� $N2.\r\n";
		act(tmpstr.c_str(), FALSE, tch, 0, ch, TO_CHAR | TO_SLEEP);

		tmpstr = "$N2 ���������� ������� � ��������� ��� ��� �������� ����� ���.\r\n";
		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_CHAR | TO_SLEEP);
		return;
	}

	if (obj->get_contains())
	{
		sprintf(tmpbuff, "������� %s �� ��������.\r\n", obj->get_PName(3).c_str());
		send_to_char(tmpbuff, ch);
		sprintf(tmpbuff, "��������������� %s � ������ ������ �� ��������.\r\n", obj->get_PName(1).c_str());
		send_to_char(tmpbuff, tch);
		return;
	}

// - �������� ������� � ��������
	tmpstr = "����� ���� �������� ������ ������� ���� ������� �� �����-�����.";

	act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_CHAR);
	act(tmpstr.c_str(), FALSE, tch, 0, tch, TO_CHAR);

	tmpstr = "����� $n4 �������� ����-������� �� �����-�����.";

	act(tmpstr.c_str(), FALSE, ch, 0, ch, TO_ROOM);
	act(tmpstr.c_str(), FALSE, tch, 0, tch, TO_ROOM);

	act("����-������� ��� ��� ����� ���.", FALSE, ch, 0, ch, TO_CHAR);
	act("����-������� ��� ���� ��� $n2", FALSE, ch, 0, ch, TO_ROOM);

	tmpstr = "�� ������ " + obj->get_PName(3) + " �����-��������.";
	act(tmpstr.c_str(), FALSE, ch, 0, ch, TO_CHAR);

	tmpstr = "$n �����$g " + obj->get_PName(3) + " �����-��������.";
	act(tmpstr.c_str(), FALSE, ch, 0, ch, TO_ROOM);

	act("�� ���� ����� ��� �����-��������.", FALSE, tch, 0, tch, TO_CHAR);
	act("$n ���$g ���� ��� �����-��������.", FALSE, tch, 0, tch, TO_ROOM);


	tmpstr = "����-������� ����� " + obj->get_PName(3) + " ���.";
	act(tmpstr.c_str(), FALSE, tch, 0, tch, TO_CHAR);

	tmpstr = "����-������� ����� " + obj->get_PName(3) + " $n2.";
	act(tmpstr.c_str(), FALSE, tch, 0, tch, TO_ROOM);

	tmpstr = "����-������� ����� � ������ ����. �� ��� ���� �� �������� �������:\r\n";
	tmpstr += "'���� �� ����� - ������� ������ �����'.";

	act(tmpstr.c_str(), FALSE, ch, 0, ch, TO_CHAR);
	act(tmpstr.c_str(), FALSE, ch, 0, ch, TO_ROOM);

	act(tmpstr.c_str(), FALSE, tch, 0, tch, TO_CHAR);
	act(tmpstr.c_str(), FALSE, tch, 0, tch, TO_ROOM);

	// ������ ��������� ������������ �����.

	tmpstr = "�� ������� " + obj->get_PName(3) + " � ��������.\r\n";
	send_to_char(tmpstr.c_str(), ch);
	tmpstr = "�� ������ " + obj->get_PName(3) + " �� ��������.\r\n";
	send_to_char(tmpstr.c_str(), tch);

	obj_from_char(obj);
	obj_to_char(obj, tch);

	ch->add_bank(GET_LOT(lot)->cost);
	tch->remove_both_gold(GET_LOT(lot)->cost + (GET_LOT(lot)->cost / 10));

	clear_auction(lot);
	return;
}

void sell_auction(int lot)
{
	CHAR_DATA *ch, *tch;
	OBJ_DATA *obj;
	std::string tmpstr;
	char tmpbuff[MAX_INPUT_LENGTH];

	ch = GET_LOT(lot)->seller;
	tch = GET_LOT(lot)->buyer;
	obj = GET_LOT(lot)->item;

	if (!check_sell(lot))
		return;

	if (ch->in_room != IN_ROOM(tch)
			|| !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
	{
		if (GET_LOT(lot)->tact >= MAX_AUCTION_TACT_PRESENT)
		{
			sprintf(tmpbuff, "������� : ��� %d(%s) ���� � �������� �������������� ������.", lot, obj->get_PName(0).c_str());

			message_auction(tmpbuff, NULL);
			clear_auction(lot);
			return;
		}
		tmpstr = "��� ���������� ������� � ������� �������� � $n2 ��� ��������� " +
			obj->get_PName(1) + "\r\n��� ��������������� �������� ������.";

		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_VICT | TO_SLEEP);

		tmpstr = "��� ���������� ������� � ������� �������� � $N2 ��� ��������� ����� �� " + obj->get_PName(3) + ".";

		act(tmpstr.c_str(), FALSE, ch, 0, tch, TO_CHAR | TO_SLEEP);
		GET_LOT(lot)->tact = MAX(GET_LOT(lot)->tact, MAX_AUCTION_TACT_BUY);
		return;
	}

	if (obj->get_contains())
	{
		sprintf(tmpbuff, "������� %s �� ��������.\r\n", obj->get_PName(3).c_str());
		send_to_char(tmpbuff, ch);
		return;
	}

	tmpstr = "�� ������� " + obj->get_PName(3) + " � ��������.\r\n";
	send_to_char(tmpstr.c_str(), ch);

	tmpstr = "�� ������ " + obj->get_PName(3) + " �� ��������.\r\n";
	send_to_char(tmpstr.c_str(), tch);

	obj_from_char(obj);
	obj_to_char(obj, tch);

	ch->add_bank(GET_LOT(lot)->cost);
	tch->remove_both_gold(GET_LOT(lot)->cost);

	clear_auction(lot);
	return;
}

void check_auction(CHAR_DATA * ch, OBJ_DATA * obj)
{
	int i;
	char tmpbuf[MAX_INPUT_LENGTH];
	if (ch)
	{
		for (i = 0; i < MAX_AUCTION_LOT; i++)
		{
			if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
				continue;
			if (GET_LOT(i)->seller == ch || GET_LOT(i)->seller_unique == GET_UNIQUE(ch)
					|| GET_LOT(i)->buyer == ch || GET_LOT(i)->buyer_unique == GET_UNIQUE(ch)
					|| GET_LOT(i)->prefect == ch || GET_LOT(i)->prefect_unique == GET_UNIQUE(ch))
			{
				sprintf(tmpbuf, "������� : ��� %d(%s) ���� � �������� ��������������.",
					i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, ch);
				clear_auction(i);
			}
		}
	}
	else if (obj)
	{
		for (i = 0; i < MAX_AUCTION_LOT; i++)
		{
			if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
				continue;
			if (GET_LOT(i)->item == obj || GET_LOT(i)->item_id == obj->get_id())
			{
				sprintf(tmpbuf, "������� : ��� %d(%s) ���� � �������� ��������������.",
					i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, obj->get_carried_by());
				clear_auction(i);
			}
		}
	}
	else
	{
		for (i = 0; i < MAX_AUCTION_LOT; i++)
		{
			if (!GET_LOT(i)->seller
				|| !GET_LOT(i)->item)
			{
				continue;
			}
			if (GET_LOT(i)->item->get_carried_by() != GET_LOT(i)->seller
				|| (GET_LOT(i)->buyer
					&& (GET_LOT(i)->buyer->get_total_gold() < GET_LOT(i)->cost)))
			{
				sprintf(tmpbuf, "������� : ��� %d(%s) ���� � �������� ��������������.",
					i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, NULL);
				clear_auction(i);
			}
		}
	}
}

void tact_auction(void)
{
	int i;
	char tmpbuf[MAX_INPUT_LENGTH];

	check_auction(NULL, NULL);

	for (i = 0; i < MAX_AUCTION_LOT; i++)
	{
		if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
			continue;
		if (++GET_LOT(i)->tact < MAX_AUCTION_TACT_BUY)
		{
			sprintf(tmpbuf, "������� : ��� %d(%s), %d %s, %s", i,
				GET_LOT(i)->item->get_PName(0).c_str(), GET_LOT(i)->cost,
				desc_count(GET_LOT(i)->cost, WHAT_MONEYa), tact_message[GET_LOT(i)->tact]);
			message_auction(tmpbuf, NULL);
			continue;
		}
		else if (GET_LOT(i)->tact < MAX_AUCTION_TACT_PRESENT)
		{
			if (!GET_LOT(i)->buyer)
			{
				sprintf(tmpbuf, "������� : ��� %d(%s) ���� �������������� ����� ���������� ������.",
					i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, NULL);
				clear_auction(i);
				continue;
			}
			if (!GET_LOT(i)->prefect)
			{
				sprintf(tmpbuf, "������� : ��� %d(%s), %d %s - �������.",
					i, GET_LOT(i)->item->get_PName(0).c_str(), GET_LOT(i)->cost,
					desc_count(GET_LOT(i)->cost, WHAT_MONEYa));
				message_auction(tmpbuf, NULL);
				GET_LOT(i)->prefect = GET_LOT(i)->buyer;
				GET_LOT(i)->prefect_unique = GET_LOT(i)->buyer_unique;
			}
			sell_auction(i);
		}
		else
			sell_auction(i);
	}
}

AUCTION_DATA *free_auction(int *lotnum)
{
	int i;
	for (i = 0; i < MAX_AUCTION_LOT; i++)
	{
		if (!GET_LOT(i)->seller && !GET_LOT(i)->item)
		{
			*lotnum = i;
			return (GET_LOT(i));
		}
	}

	return (NULL);
}

int obj_on_auction(OBJ_DATA * obj)
{
	int i;
	for (i = 0; i < MAX_AUCTION_LOT; i++)
	{
		if (GET_LOT(i)->item == obj && GET_LOT(i)->item_id == obj->get_id())
			return (TRUE);
	}

	return (FALSE);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
