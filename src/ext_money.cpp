// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "ext_money.hpp"

#include "conf.h"
#include "screen.h"
#include "db.h"
#include "logger.hpp"
#include "interpreter.h"
#include "pugixml.hpp"
#include "room.hpp"
#include "parse.hpp"
#include "utils.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <sstream>

using namespace ExtMoney;
using namespace Remort;

namespace
{

void message_low_torc(CHAR_DATA *ch, unsigned type, int amount, const char *add_text);

} // namespace

namespace Remort
{

const char *CONFIG_FILE = LIB_MISC"remort.xml";
std::string WHERE_TO_REMORT_STR;

int calc_torc_daily(int rmrt);

} // namespace Remort

namespace ExtMoney
{

// �� ��� ��� ���������� �������� init()
int TORC_EXCH_RATE = 999;
std::map<std::string, std::string> plural_name_currency_map = {
	{ "����" , "�����" },
	{ "�����" , "�����" },
	{ "���" , "����" },
};

std::string name_currency_plural(const std::string& name)
{	
	const auto it = plural_name_currency_map.find(name);
	if (it != plural_name_currency_map.end())
	{
		return (*it).second;
	}
	return "����������� ������";
}

struct type_node
{
	type_node() : MORT_REQ(99), MORT_REQ_ADD_PER_MORT(99), MORT_NUM(99),
		DROP_LVL(99), DROP_AMOUNT(0), DROP_AMOUNT_ADD_PER_LVL(0), MINIMUM_DAYS(99),
		DESC_MESSAGE_NUM(0), DESC_MESSAGE_U_NUM(0) {};
	// ������� ������ ��������� �� ��������������� ����� �����
    int MORT_REQ;
	// ������� ��������� � ����������� �� ������ ���� ������
    int MORT_REQ_ADD_PER_MORT;
	// � ������ ����� ��������� ����� ������
    int MORT_NUM;
	// � ������ ��� ������ ���. �������� ������
    int DROP_LVL;
    // ������� ������� � �������� ��. ������
    int DROP_AMOUNT;
	// ������� ���������� �� ������ ������� ���� ��������
    int DROP_AMOUNT_ADD_PER_LVL;
	// �������� ���������� ������, ���������� ������� ���� ������� �����������
	// ��� ������ ������������ �� ���� ���-�� ������. �������� ���� ��������
	// ����� 7, � ������ ��� ����. ����� ����� 70 �������, �� �� ����� ���������
	// ��������� �� ����� 10 ������� ������ ��� �� �����������
    int MINIMUM_DAYS;
    // ��� ��������� ����� desc_count
    int DESC_MESSAGE_NUM;
    int DESC_MESSAGE_U_NUM;
};

// ������ ����� ������ �� ����� �� �����������
std::array<type_node, TOTAL_TYPES> type_list;

struct TorcReq
{
	TorcReq(int rmrt);
	// ��� �����
	unsigned type;
	// ���-��
	int amount;
};

TorcReq::TorcReq(int rmrt)
{
	// type
	if (rmrt >= type_list[TORC_GOLD].MORT_NUM)
	{
		type = TORC_GOLD;
	}
	else if (rmrt >= type_list[TORC_SILVER].MORT_NUM)
	{
		type = TORC_SILVER;
	}
	else if (rmrt >= type_list[TORC_BRONZE].MORT_NUM)
	{
		type = TORC_BRONZE;
	}
	else
	{
		type = TOTAL_TYPES;
	}
	// amount
	if (type != TOTAL_TYPES)
	{
		amount = type_list[type].MORT_REQ +
			(rmrt - type_list[type].MORT_NUM) * type_list[type].MORT_REQ_ADD_PER_MORT;
	}
	else
	{
		amount = 0;
	}
}

} // namespace ExtMoney

namespace ExtMoney
{

// ���������� ���� ������ ������ ('������' � ��������)
void torc_exch_menu(CHAR_DATA *ch)
{
	boost::format menu("   %s%d) %s%-17s%s -> %s%-17s%s [%d -> %d]\r\n");
	std::stringstream out;

	out << "\r\n"
		"   ����� ������ ������:\r\n"
		"   " << TORC_EXCH_RATE << "  ��������� <-> 1 ����������\r\n"
		"   " << TORC_EXCH_RATE << " ���������� <-> 1 �������\r\n\r\n";

	out << "   ������� ������: "
		<< CCIYEL(ch, C_NRM) << ch->desc->ext_money[TORC_GOLD] << "� "
		<< CCWHT(ch, C_NRM) << ch->desc->ext_money[TORC_SILVER] << "� "
		<< CCYEL(ch, C_NRM) << ch->desc->ext_money[TORC_BRONZE] << "�\r\n\r\n";

	out << menu
		% CCGRN(ch, C_NRM) % 1
		% CCYEL(ch, C_NRM) % "��������� ������" % CCNRM(ch, C_NRM)
		% CCWHT(ch, C_NRM) % "���������� ������" % CCNRM(ch, C_NRM)
		% TORC_EXCH_RATE % 1;
	out << menu
		% CCGRN(ch, C_NRM) % 2
		% CCWHT(ch, C_NRM) % "���������� ������" % CCNRM(ch, C_NRM)
		% CCIYEL(ch, C_NRM) % "������� ������" % CCNRM(ch, C_NRM)
		% TORC_EXCH_RATE % 1;
	out << "\r\n"
		<< menu
		% CCGRN(ch, C_NRM) % 3
		% CCIYEL(ch, C_NRM) % "������� ������" % CCNRM(ch, C_NRM)
		% CCWHT(ch, C_NRM) % "���������� ������" % CCNRM(ch, C_NRM)
		% 1 % TORC_EXCH_RATE;
	out << menu
		% CCGRN(ch, C_NRM) % 4
		% CCWHT(ch, C_NRM) % "���������� ������" % CCNRM(ch, C_NRM)
		% CCYEL(ch, C_NRM) % "��������� ������" % CCNRM(ch, C_NRM)
		% 1 % TORC_EXCH_RATE;

	out << "\r\n"
		"   <����� ��������> - ���� ����������� ����� ���������� ����\r\n"
		"   <����� ��������> <����� �> - ����� � ��������� ������\r\n\r\n";

	out << CCGRN(ch, C_NRM) << "   5)"
		<< CCNRM(ch, C_NRM) << " �������� ����� � �����\r\n"
		<< CCGRN(ch, C_NRM) << "   6)"
		<< CCNRM(ch, C_NRM) << " ����������� ����� � �����\r\n\r\n"
		<< "   ��� �����:";

	send_to_char(out.str(), ch);
}

// ����� � ������� ������� ������
void parse_inc_exch(CHAR_DATA *ch, int amount, int num)
{
	int torc_from = TORC_BRONZE;
	int torc_to = TORC_SILVER;
	const int torc_rate = TORC_EXCH_RATE;

	if (num == 2)
	{
		torc_from = TORC_SILVER;
		torc_to = TORC_GOLD;
	}

	if (ch->desc->ext_money[torc_from] < amount)
	{
		if (ch->desc->ext_money[torc_from] < torc_rate)
		{
			send_to_char("��� ������������ ���������� ������!\r\n", ch);
		}
		else
		{
			amount = ch->desc->ext_money[torc_from] / torc_rate * torc_rate;
			send_to_char(ch, "���������� �������� ������ ��������� �� %d.\r\n", amount);
			ch->desc->ext_money[torc_from] -= amount;
			ch->desc->ext_money[torc_to] += amount / torc_rate;
			send_to_char(ch, "���������� �����: %d -> %d.\r\n", amount, amount / torc_rate);
		}
	}
	else
	{
		const int real_amount = amount / torc_rate * torc_rate;
		if (real_amount != amount)
		{
			send_to_char(ch, "���������� �������� ������ ��������� �� %d.\r\n", real_amount);
		}
		ch->desc->ext_money[torc_from] -= real_amount;
		ch->desc->ext_money[torc_to] += real_amount / torc_rate;
		send_to_char(ch, "���������� �����: %d -> %d.\r\n", real_amount, real_amount / torc_rate);
	}
}

// ����� � ������� ������� ������
void parse_dec_exch(CHAR_DATA *ch, int amount, int num)
{
	int torc_from = TORC_GOLD;
	int torc_to = TORC_SILVER;
	const int torc_rate = TORC_EXCH_RATE;

	if (num == 4)
	{
		torc_from = TORC_SILVER;
		torc_to = TORC_BRONZE;
	}

	if (ch->desc->ext_money[torc_from] < amount)
	{
		if (ch->desc->ext_money[torc_from] < 1)
		{
			send_to_char("��� ������������ ���������� ������!\r\n", ch);
		}
		else
		{
			amount = ch->desc->ext_money[torc_from];
			send_to_char(ch, "���������� �������� ������ ��������� �� %d.\r\n", amount);

			ch->desc->ext_money[torc_from] -= amount;
			ch->desc->ext_money[torc_to] += amount * torc_rate;
			send_to_char(ch, "���������� �����: %d -> %d.\r\n", amount, amount * torc_rate);
		}
	}
	else
	{
		ch->desc->ext_money[torc_from] -= amount;
		ch->desc->ext_money[torc_to] += amount * torc_rate;
		send_to_char(ch, "���������� �����: %d -> %d.\r\n", amount, amount * torc_rate);
	}
}

// ���-�� �������� ������
int check_input_amount(CHAR_DATA* /*ch*/, int num1, int num2)
{
	if ((num1 == 1 || num1 == 2) && num2 < TORC_EXCH_RATE)
	{
		return TORC_EXCH_RATE;
	}
	else if ((num1 == 3 || num1 == 4) && num2 < 1)
	{
		return 1;
	}
	return 0;
}

// �������� ����� ������, ��� ������ ������� �� ��������� ��������
bool check_equal_exch(CHAR_DATA *ch)
{
	int before = 0, after = 0;
	for (unsigned i = 0; i < TOTAL_TYPES; ++i)
	{
		if (i == TORC_BRONZE)
		{
			before += ch->get_ext_money(i);
			after += ch->desc->ext_money[i];
		}
		if (i == TORC_SILVER)
		{
			before += ch->get_ext_money(i) * TORC_EXCH_RATE;
			after += ch->desc->ext_money[i] * TORC_EXCH_RATE;
		}
		else if (i == TORC_GOLD)
		{
			before += ch->get_ext_money(i) * TORC_EXCH_RATE * TORC_EXCH_RATE;
			after += ch->desc->ext_money[i] * TORC_EXCH_RATE * TORC_EXCH_RATE;
		}
	}
	if (before != after)
	{
		sprintf(buf, "SYSERROR: Torc exch by %s not equal: %d -> %d",
			GET_NAME(ch), before, after);
		mudlog(buf, DEF, LVL_IMMORT, SYSLOG, TRUE);
		return false;
	}
	return true;
}

// ���� ����� ��� ������ ������
void torc_exch_parse(CHAR_DATA *ch, const char *arg)
{
	if (!*arg || !a_isdigit(*arg))
	{
		send_to_char("�������� �����!\r\n", ch);
		torc_exch_menu(ch);
		return;
	}

	std::string param2(arg), param1;
	GetOneParam(param2, param1);
	boost::trim(param2);

	int num1 = 0, num2 = 0;

	try
	{
		num1 = std::stoi(param1, nullptr, 10);
		if (!param2.empty())
		{
			num2 = std::stoi(param2, nullptr, 10);
		}
	}
	catch (const std::invalid_argument&)
	{
		log("SYSERROR: invalid_argument arg=%s (%s %s %d)",
			arg, __FILE__, __func__, __LINE__);

		send_to_char("�������� �����!\r\n", ch);
		torc_exch_menu(ch);
		return;
	}

	int amount = num2;
	if (!param2.empty())
	{
		// ����� ��� ����� - �������� ����������������� ����� ���-�� ������
		const int min_amount = check_input_amount(ch, num1, amount);
		if (min_amount > 0)
		{
			send_to_char(ch, "����������� ���������� ������ ��� ������� ������: %d.", min_amount);
			torc_exch_menu(ch);
			return;
		}
	}
	else
	{
		// ����� ���� ����� - ����������� ����������� �����
		if (num1 == 1 || num1 == 2)
		{
			amount = TORC_EXCH_RATE;
		}
		else if (num1 == 3 || num1 == 4)
		{
			amount = 1;
		}
	}
	// ������ �����
	if (num1 == 1 || num1 == 2)
	{
		parse_inc_exch(ch, amount, num1);
	}
	else if (num1 == 3 || num1 == 4)
	{
		parse_dec_exch(ch, amount, num1);
	}
	else if (num1 == 5)
	{
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("����� �������.\r\n", ch);
		return;
	}
	else if (num1 == 6)
	{
		if (!check_equal_exch(ch))
		{
			send_to_char("����� ������� �� ����������� ��������, ���������� � �����.\r\n", ch);
		}
		else
		{
			for (unsigned i = 0; i < TOTAL_TYPES; ++i)
			{
				ch->set_ext_money(i, ch->desc->ext_money[i]);
			}
			STATE(ch->desc) = CON_PLAYING;
			send_to_char("����� ����������.\r\n", ch);
		}
		return;
	}
	else
	{
		send_to_char("�������� �����!\r\n", ch);
	}
	torc_exch_menu(ch);
}

// ������������ ��������� � ������� �������� ��� ������ �����
// ����� � ���� ������ � ���������� ����� ������, ���� ������� ����
std::string create_message(CHAR_DATA *ch, int gold, int silver, int bronze)
{
	std::stringstream out;
	int cnt = 0;

	if (gold > 0)
	{
		out << CCIYEL(ch, C_NRM) << gold << " "
			<< desc_count(gold, type_list[TORC_GOLD].DESC_MESSAGE_U_NUM);
		if (silver <= 0 && bronze <= 0)
		{
			out << " " << desc_count(gold, WHAT_TORCu);
		}
		out << CCNRM(ch, C_NRM);
		++cnt;
	}
	if (silver > 0)
	{
		if (cnt > 0)
		{
			if (bronze > 0)
			{
				out << ", " << CCWHT(ch, C_NRM) << silver << " "
					<< desc_count(silver, type_list[TORC_SILVER].DESC_MESSAGE_U_NUM)
					<< CCNRM(ch, C_NRM) << " � ";
			}
			else
			{
				out << " � " << CCWHT(ch, C_NRM) << silver << " "
					<< desc_count(silver, type_list[TORC_SILVER].DESC_MESSAGE_U_NUM)
					<< " " << desc_count(silver, WHAT_TORCu)
					<< CCNRM(ch, C_NRM);
			}
		}
		else
		{
			out << CCWHT(ch, C_NRM) << silver << " "
				<< desc_count(silver, type_list[TORC_SILVER].DESC_MESSAGE_U_NUM);
			if (bronze > 0)
			{
				out << CCNRM(ch, C_NRM) << " � ";
			}
			else
			{
				out << " " << desc_count(silver, WHAT_TORCu) << CCNRM(ch, C_NRM);
			}
		}
	}
	if (bronze > 0)
	{
		out << CCYEL(ch, C_NRM) << bronze << " "
			<< desc_count(bronze, type_list[TORC_BRONZE].DESC_MESSAGE_U_NUM)
			<< " " << desc_count(bronze, WHAT_TORCu)
			<< CCNRM(ch, C_NRM);
	}

	return out.str();
}

// �������� �� ������ ���������� ���������� ������,
// ������� ��������� �������� ����� �������, ��������������� ������ ���� ������
bool has_connected_bosses(CHAR_DATA *ch)
{
	// ���� � ������� ���� ������ ����� �����
	for (const auto i : world[ch->in_room]->people)
	{
		if (i != ch
			&& IS_NPC(i)
			&& !IS_CHARMICE(i)
			&& i->get_role(MOB_ROLE_BOSS))
		{
			return true;
		}
	}
	// ���� � ������� ���� ���� ����� �������������-�����
	for (follow_type *i = ch->followers; i; i = i->next)
	{
		if (i->follower != ch
			&& IS_NPC(i->follower)
			&& !IS_CHARMICE(i->follower)
			&& i->follower->get_master() == ch
			&& i->follower->get_role(MOB_ROLE_BOSS))
		{
			return true;
		}
	}
	// ���� �� ��� ������� �� �����-�� ������
	if (ch->has_master() && ch->get_master()->get_role(MOB_ROLE_BOSS))
	{
		return true;
	}

	return false;
}

// �������� ����� ��� ������ ������� � ����
unsigned calc_type_by_zone_lvl(int zone_lvl)
{
	if (zone_lvl >= type_list[TORC_GOLD].DROP_LVL)
	{
		return TORC_GOLD;
	}
	else if (zone_lvl >= type_list[TORC_SILVER].DROP_LVL)
	{
		return TORC_SILVER;
	}
	else if (zone_lvl >= type_list[TORC_BRONZE].DROP_LVL)
	{
		return TORC_BRONZE;
	}
	return TOTAL_TYPES;
}

// ���������� ���-�� ������ � ����� ���� ������ zone_lvl, ����������
// � ������ ������ members � ������������� � ������
int calc_drop_torc(int zone_lvl, int members)
{
	const unsigned type = calc_type_by_zone_lvl(zone_lvl);
	if (type >= TOTAL_TYPES)
	{
		return 0;
	}
	const int add = zone_lvl - type_list[type].DROP_LVL;
	int drop = type_list[type].DROP_AMOUNT + add * type_list[type].DROP_AMOUNT_ADD_PER_LVL;

	// ������������� ���� � ������������ ����
	if (type == TORC_GOLD)
	{
		drop = drop * TORC_EXCH_RATE * TORC_EXCH_RATE;
	}
	else if (type == TORC_SILVER)
	{
		drop = drop * TORC_EXCH_RATE;
	}

	// ���� �� ������ ��� �������
	if (drop < members)
	{
		return 0;
	}

	// ����� ����� ��� ��������� �������� ������
	if (members > 1)
	{
		drop = drop / members;
	}

	return drop;
}

// �� ������� ��������� * �� ������ 1/5 �� ��������� ������ ������
// ���� imm_stat == true, �� ������ ��������� ���������� ����� ���/����
std::string draw_daily_limit(CHAR_DATA *ch, bool imm_stat)
{
	const int today_torc = ch->get_today_torc();
	const int torc_req_daily = calc_torc_daily(GET_REMORT(ch));

	TorcReq torc_req(GET_REMORT(ch));
	if (torc_req.type >= TOTAL_TYPES)
	{
		torc_req.type = TORC_BRONZE;
	}
	const int daily_torc_limit = torc_req_daily / type_list[torc_req.type].MINIMUM_DAYS;

	std::string out("[");
	if (!imm_stat)
	{
		for (int i = 1; i <= 5; ++i)
		{
			if (daily_torc_limit / 5 * i <= today_torc)
			{
				out += "*";
			}
			else
			{
				out += ".";
			}
		}
	}
	else
	{
		out += boost::str(boost::format("%d/%d") % today_torc % daily_torc_limit);
	}
	out += "]";

	return out;
}

// �������� ����� ������ �� �������� ������
int check_daily_limit(CHAR_DATA *ch, int drop)
{
	const int today_torc = ch->get_today_torc();
	const int torc_req_daily = calc_torc_daily(GET_REMORT(ch));

	// �� calc_torc_daily � ����� ������ ������� �����-�� ����� ������
	// ���� ���� ��� �� ����� ������ ��� ���������� ������
	TorcReq torc_req(GET_REMORT(ch));
	if (torc_req.type >= TOTAL_TYPES)
	{
		torc_req.type = TORC_BRONZE;
	}
	const int daily_torc_limit = torc_req_daily / type_list[torc_req.type].MINIMUM_DAYS;

	if (today_torc + drop > daily_torc_limit)
	{
		const int add = daily_torc_limit - today_torc;
		if (add > 0)
		{
			ch->add_today_torc(add);
			return add;
		}
		else
		{
			return 0;
		}
	}

	ch->add_today_torc(drop);
	return drop;
}

// ������� ����� ������ ����������� ����
void gain_torc(CHAR_DATA *ch, int drop)
{
	// �������� �� �������������� �������� ������ �����
	int bronze = check_daily_limit(ch, drop);
	if (bronze <= 0)
	{
		return;
	}
	int gold = 0, silver = 0;
	// � �������� ��� �������� ������� �� �����
	if (bronze >= TORC_EXCH_RATE * TORC_EXCH_RATE)
	{
		gold += bronze / (TORC_EXCH_RATE * TORC_EXCH_RATE);
		bronze -= gold * TORC_EXCH_RATE * TORC_EXCH_RATE;
		ch->set_ext_money(TORC_GOLD, gold + ch->get_ext_money(TORC_GOLD));
	}
	if (bronze >= TORC_EXCH_RATE)
	{
		silver += bronze / TORC_EXCH_RATE;
		bronze -= silver * TORC_EXCH_RATE;
		ch->set_ext_money(TORC_SILVER, silver + ch->get_ext_money(TORC_SILVER));
	}
	ch->set_ext_money(TORC_BRONZE, bronze + ch->get_ext_money(TORC_BRONZE));

	std::string out = create_message(ch, gold, silver, bronze);
	send_to_char(ch, "� ������� �� ���������� ������ �� �������� �� ����� %s.\r\n", out.c_str());

}

// ��������� �� ��������_���, � ����� ������� ���� �������, �����������
// � ��� �� �������, ������ �������� � ������������� ������, ���� ����
// ���� ����������� (���� GF_REMORT, �������� �� ��������� ������, �������� ��
// ��, ��� ��� ��������� � ������� � ����� �� ����� �������� ������� ��������)
void drop_torc(CHAR_DATA *mob)
{
	if (!mob->get_role(MOB_ROLE_BOSS)
		|| has_connected_bosses(mob))
	{
		return;
	}

	log("[Extract char] Checking %s for ExtMoney.", mob->get_name().c_str());

	const auto damager = mob->get_max_damager_in_room();
	DESCRIPTOR_DATA *d = 0;
	if (damager.first > 0)
	{
		d = DescByUID(damager.first);
	}
	if (!d)
	{
		return;
	}

	CHAR_DATA *leader = (d->character->has_master() && AFF_FLAGGED(d->character, EAffectFlag::AFF_GROUP))
		? d->character->get_master()
		: d->character.get();

	int members = 1;
	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
			&& f->follower->in_room == IN_ROOM(mob)
			&& !IS_NPC(f->follower))
		{
			++members;
		}
	}

	const int zone_lvl = zone_table[mob_index[GET_MOB_RNUM(mob)].zone].mob_level;
	const int drop = calc_drop_torc(zone_lvl, members);
	if (drop <= 0)
	{
		return;
	}

	if (IN_ROOM(leader) == IN_ROOM(mob)
		&& GET_GOD_FLAG(leader, GF_REMORT)
		&& (GET_UNIQUE(leader) == damager.first
			|| mob->get_attacker(leader, ATTACKER_ROUNDS) >= damager.second / 2))
	{
		gain_torc(leader, drop);
	}

	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
			&& f->follower->in_room == IN_ROOM(mob)
			&& !IS_NPC(f->follower)
			&& GET_GOD_FLAG(f->follower, GF_REMORT)
			&& mob->get_attacker(f->follower, ATTACKER_ROUNDS) >= damager.second / 2)
		{
			gain_torc(f->follower, drop);
		}
	}
}

void player_drop_log(CHAR_DATA *ch, unsigned type, int diff)
{
	int total_bronze = ch->get_ext_money(TORC_BRONZE);
	total_bronze += ch->get_ext_money(TORC_SILVER) * TORC_EXCH_RATE;
	total_bronze += ch->get_ext_money(TORC_GOLD) * TORC_EXCH_RATE * TORC_EXCH_RATE;

	log("ExtMoney: %s%s%d%s, sum=%d",
		ch->get_name().c_str(),
		(diff > 0 ? " +" : " "),
		diff,
		((type == TORC_GOLD) ? "g" : (type == TORC_SILVER) ? "s" : "b"),
		total_bronze);
}

} // namespace ExtMoney

namespace Remort
{

// ���������� ����� 'reload remort.xml'
void init()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	const auto main_node = doc.child("remort");
    if (!main_node)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<remort> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }

	WHERE_TO_REMORT_STR = Parse::child_value_str(main_node, "WHERE_TO_REMORT_STR");
	TORC_EXCH_RATE = Parse::child_value_int(main_node, "TORC_EXCH_RATE");

	type_list[TORC_GOLD].MORT_NUM = Parse::child_value_int(main_node, "GOLD_MORT_NUM");
	type_list[TORC_GOLD].MORT_REQ = Parse::child_value_int(main_node, "GOLD_MORT_REQ");
	type_list[TORC_GOLD].MORT_REQ_ADD_PER_MORT = Parse::child_value_int(main_node, "GOLD_MORT_REQ_ADD_PER_MORT");
	type_list[TORC_GOLD].DROP_LVL = Parse::child_value_int(main_node, "GOLD_DROP_LVL");
	type_list[TORC_GOLD].DROP_AMOUNT = Parse::child_value_int(main_node, "GOLD_DROP_AMOUNT");
	type_list[TORC_GOLD].DROP_AMOUNT_ADD_PER_LVL = Parse::child_value_int(main_node, "GOLD_DROP_AMOUNT_ADD_PER_LVL");
	type_list[TORC_GOLD].MINIMUM_DAYS = Parse::child_value_int(main_node, "GOLD_MINIMUM_DAYS");

	type_list[TORC_SILVER].MORT_NUM = Parse::child_value_int(main_node, "SILVER_MORT_NUM");
	type_list[TORC_SILVER].MORT_REQ = Parse::child_value_int(main_node, "SILVER_MORT_REQ");
	type_list[TORC_SILVER].MORT_REQ_ADD_PER_MORT = Parse::child_value_int(main_node, "SILVER_MORT_REQ_ADD_PER_MORT");
	type_list[TORC_SILVER].DROP_LVL = Parse::child_value_int(main_node, "SILVER_DROP_LVL");
	type_list[TORC_SILVER].DROP_AMOUNT = Parse::child_value_int(main_node, "SILVER_DROP_AMOUNT");
	type_list[TORC_SILVER].DROP_AMOUNT_ADD_PER_LVL = Parse::child_value_int(main_node, "SILVER_DROP_AMOUNT_ADD_PER_LVL");
	type_list[TORC_SILVER].MINIMUM_DAYS = Parse::child_value_int(main_node, "SILVER_MINIMUM_DAYS");

	type_list[TORC_BRONZE].MORT_NUM = Parse::child_value_int(main_node, "BRONZE_MORT_NUM");
	type_list[TORC_BRONZE].MORT_REQ = Parse::child_value_int(main_node, "BRONZE_MORT_REQ");
	type_list[TORC_BRONZE].MORT_REQ_ADD_PER_MORT = Parse::child_value_int(main_node, "BRONZE_MORT_REQ_ADD_PER_MORT");
	type_list[TORC_BRONZE].DROP_LVL = Parse::child_value_int(main_node, "BRONZE_DROP_LVL");
	type_list[TORC_BRONZE].DROP_AMOUNT = Parse::child_value_int(main_node, "BRONZE_DROP_AMOUNT");
	type_list[TORC_BRONZE].DROP_AMOUNT_ADD_PER_LVL = Parse::child_value_int(main_node, "BRONZE_DROP_AMOUNT_ADD_PER_LVL");
	type_list[TORC_BRONZE].MINIMUM_DAYS = Parse::child_value_int(main_node, "BRONZE_MINIMUM_DAYS");

	// �� �� �������, �� ������ ������ �� �����
	type_list[TORC_GOLD].DESC_MESSAGE_NUM = WHAT_TGOLD;
	type_list[TORC_SILVER].DESC_MESSAGE_NUM = WHAT_TSILVER;
	type_list[TORC_BRONZE].DESC_MESSAGE_NUM = WHAT_TBRONZE;
	type_list[TORC_GOLD].DESC_MESSAGE_U_NUM = WHAT_TGOLDu;
	type_list[TORC_SILVER].DESC_MESSAGE_U_NUM = WHAT_TSILVERu;
	type_list[TORC_BRONZE].DESC_MESSAGE_U_NUM = WHAT_TBRONZEu;
}

// ��������, ������ �� ���-�� ���� ���� � ������
bool can_remort_now(CHAR_DATA *ch)
{
	if (PRF_FLAGGED(ch, PRF_CAN_REMORT) || !need_torc(ch))
	{
		return true;
	}
	return false;
}

// ���������� ���������� �� �������
void show_config(CHAR_DATA *ch)
{
	std::stringstream out;
	out << "&S������� �������� �������� ����������:\r\n"
		<< "WHERE_TO_REMORT_STR = " << WHERE_TO_REMORT_STR << "\r\n"
		<< "TORC_EXCH_RATE = " << TORC_EXCH_RATE << "\r\n"

		<< "GOLD_MORT_NUM = " << type_list[TORC_GOLD].MORT_NUM << "\r\n"
		<< "GOLD_MORT_REQ = " << type_list[TORC_GOLD].MORT_REQ << "\r\n"
		<< "GOLD_MORT_REQ_ADD_PER_MORT = " << type_list[TORC_GOLD].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "GOLD_DROP_LVL = " << type_list[TORC_GOLD].DROP_LVL << "\r\n"
		<< "GOLD_DROP_AMOUNT = " << type_list[TORC_GOLD].DROP_AMOUNT << "\r\n"
		<< "GOLD_DROP_AMOUNT_ADD_PER_LVL = " << type_list[TORC_GOLD].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "GOLD_MINIMUM_DAYS = " << type_list[TORC_GOLD].MINIMUM_DAYS << "\r\n"

		<< "SILVER_MORT_NUM = " << type_list[TORC_SILVER].MORT_NUM << "\r\n"
		<< "SILVER_MORT_REQ = " << type_list[TORC_SILVER].MORT_REQ << "\r\n"
		<< "SILVER_MORT_REQ_ADD_PER_MORT = " << type_list[TORC_SILVER].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "SILVER_DROP_LVL = " << type_list[TORC_SILVER].DROP_LVL << "\r\n"
		<< "SILVER_DROP_AMOUNT = " << type_list[TORC_SILVER].DROP_AMOUNT << "\r\n"
		<< "SILVER_DROP_AMOUNT_ADD_PER_LVL = " << type_list[TORC_SILVER].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "SILVER_MINIMUM_DAYS = " << type_list[TORC_SILVER].MINIMUM_DAYS << "\r\n"

		<< "BRONZE_MORT_NUM = " << type_list[TORC_BRONZE].MORT_NUM << "\r\n"
		<< "BRONZE_MORT_REQ = " << type_list[TORC_BRONZE].MORT_REQ << "\r\n"
		<< "BRONZE_MORT_REQ_ADD_PER_MORT = " << type_list[TORC_BRONZE].MORT_REQ_ADD_PER_MORT << "\r\n"
		<< "BRONZE_DROP_LVL = " << type_list[TORC_BRONZE].DROP_LVL << "\r\n"
		<< "BRONZE_DROP_AMOUNT = " << type_list[TORC_BRONZE].DROP_AMOUNT << "\r\n"
		<< "BRONZE_DROP_AMOUNT_ADD_PER_LVL = " << type_list[TORC_BRONZE].DROP_AMOUNT_ADD_PER_LVL << "\r\n"
		<< "BRONZE_MINIMUM_DAYS = " << type_list[TORC_BRONZE].MINIMUM_DAYS << "\r\n";

	send_to_char(out.str(), ch);
}

// ���������� ���������� ������ �� ���� � ��������� �� ������
// ��� ���-������ ������� ���������� ������ �������� ������
int calc_torc_daily(int rmrt)
{
	const TorcReq torc_req(rmrt);
	int num = 0;

	if (torc_req.type < TOTAL_TYPES)
	{
		num = type_list[torc_req.type].MORT_REQ;

		if (torc_req.type == TORC_GOLD)
		{
			num = num * TORC_EXCH_RATE * TORC_EXCH_RATE;
		}
		else if (torc_req.type == TORC_SILVER)
		{
			num = num * TORC_EXCH_RATE;
		}
	}
	else
	{
		num = type_list[TORC_BRONZE].MORT_REQ;
	}

	return num;
}

// ��������, ��������� �� �� ���� ���������� ��� �������
bool need_torc(CHAR_DATA *ch)
{
	const TorcReq torc_req(GET_REMORT(ch));

	if (torc_req.type < TOTAL_TYPES && torc_req.amount > 0)
	{
		return true;
	}

	return false;
}

} // namespace Remort

namespace
{

// ����������� ������
void donat_torc(CHAR_DATA *ch, const std::string &mob_name, unsigned type, int amount)
{
	const int balance = ch->get_ext_money(type) - amount;
	ch->set_ext_money(type, balance);
	PRF_FLAGS(ch).set(PRF_CAN_REMORT);

	send_to_char(ch, "�� ������������ %d %s %s.\r\n",
		amount, desc_count(amount, type_list[type].DESC_MESSAGE_NUM),
		desc_count(amount, WHAT_TORC));

	std::string name = mob_name;
	name_convert(name);

	send_to_char(ch,
		"%s ������ ���� ������� ����� ������ � ������� ����� ������� � ������ ��� �����.\r\n"
		"�� ������������� ���� ����������� �������.\r\n", name.c_str());

	if (GET_GOD_FLAG(ch, GF_REMORT))
	{
		send_to_char(ch,
			"%s�����������, �� �������� ����� �� ��������������!%s\r\n",
			CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	}
	else
	{
		send_to_char(ch,
			"�� ����������� ���� ����� �� ��������� ��������������,\r\n"
			"��� ��� ���������� ��� ����� ������� ������������ ���������� �����.\r\n");
	}
}

// ��������� ��� ��� �������� ������, ��� � ��� ������� ������� ��� �������������
void message_low_torc(CHAR_DATA *ch, unsigned type, int amount, const char *add_text)
{
	if (type < TOTAL_TYPES)
	{
		const int money = ch->get_ext_money(type);
		send_to_char(ch,
			"��� ������������� ����� �� �������������� �� ������ ������������ %d %s %s.\r\n"
			"� ��� � ������ ������ %d %s %s%s\r\n",
			amount,
			desc_count(amount, type_list[type].DESC_MESSAGE_U_NUM),
			desc_count(amount, WHAT_TORC),
			money,
			desc_count(money, type_list[type].DESC_MESSAGE_NUM),
			desc_count(money, WHAT_TORC),
			add_text);
	}
}

} // namespace

// ��������
int torc(CHAR_DATA *ch, void *me, int cmd, char* /*argument*/)
{
	if (!ch->desc || IS_NPC(ch))
	{
		return 0;
	}
	if (CMD_IS("������") || CMD_IS("�����") || CMD_IS("��������"))
	{
		// ��� ��� ������ ������ � ��� �������
		STATE(ch->desc) = CON_TORC_EXCH;
		for (unsigned i = 0; i < TOTAL_TYPES; ++i)
		{
			ch->desc->ext_money[i] = ch->get_ext_money(i);
		}
		torc_exch_menu(ch);
		return 1;
	}
	if (CMD_IS("��������������") || CMD_IS("���������������"))
	{
		if (can_remort_now(ch))
		{
			// ��� ��� ��������� ��� �� ���� � �� �����������
			return 0;
		}
		else if (need_torc(ch))
		{
			const TorcReq torc_req(GET_REMORT(ch));
			// ��� ��� �� ��������� � �� ���� ���-�� ���������
			message_low_torc(ch, torc_req.type, torc_req.amount, " (������� '����������').");
			return 1;
		}
	}
	if (CMD_IS("����������"))
	{
		if (!need_torc(ch))
		{
			// �� ���� ��� ������� ������ �� ���������
			send_to_char(
				"��� �� ����� ������������ ���� ����� �� ��������������, ������ �������� '���������������'.\r\n", ch);
		}
		else if (PRF_FLAGGED(ch, PRF_CAN_REMORT))
		{
			// ��� �� ���� ����� ��� ��������� ����������� ���-�� ������
			if (GET_GOD_FLAG(ch, GF_REMORT))
			{
				send_to_char(
					"�� ��� ����������� ���� ����� �� ��������������, ������ �������� '���������������'.\r\n", ch);
			}
			else
			{
				send_to_char(
					"�� ��� ������������ ����������� ���������� ������.\r\n"
					"��� ���������� �������������� ��� ����� ������� ������������ ���������� �����.\r\n", ch);
			}
		}
		else
		{
			// ������� ������������
			const TorcReq torc_req(GET_REMORT(ch));
			if (ch->get_ext_money(torc_req.type) >= torc_req.amount)
			{
				const CHAR_DATA *mob = reinterpret_cast<CHAR_DATA *>(me);
				donat_torc(ch, mob->get_name_str(), torc_req.type, torc_req.amount);
			}
			else
			{
				message_low_torc(ch, torc_req.type, torc_req.amount, ". ���������� �����.");
			}
		}
		return 1;
	}
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
